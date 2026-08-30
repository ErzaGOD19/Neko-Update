#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QAction>
#include <QIcon>
#include <QFileSystemWatcher>
#include <QTranslator>
#include <QLocale>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <malloc.h>
#include <unistd.h>

class NekoTray : public QObject {
    Q_OBJECT
public:
    NekoTray() : isWorking(false) {
        createActions();
        createTrayIcon();

        watcher = new QFileSystemWatcher(this);
        // Ensure file exists before watching
        QFile file("/tmp/neko_updates_count");
        if (!file.exists()) {
            if (file.open(QIODevice::WriteOnly)) {
                file.write("-1\n");
                file.close();
            }
        }
        watcher->addPath("/tmp/neko_updates_count");
        connect(watcher, &QFileSystemWatcher::fileChanged, this, &NekoTray::onFileChanged);
        
        updateStatus();
        trayIcon->show();
        malloc_trim(0); // Free startup memory
    }

private slots:
    void onFileChanged(const QString &path) {
        Q_UNUSED(path);
        // Re-add in case it was deleted and recreated
        if (!watcher->files().contains("/tmp/neko_updates_count")) {
            watcher->addPath("/tmp/neko_updates_count");
        }
        if (!isWorking) {
            updateStatus();
        }
    }

    void updateStatus() {
        QFile file("/tmp/neko_updates_count");
        int count = -1;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll().trimmed();
            if (!content.isEmpty()) {
                count = content.toInt();
            }
        }

        QString iconPath = "/usr/share/neko-void/";
        // Fallback for local testing
        if (!QFile::exists(iconPath + "normal.png")) {
            iconPath = "neko-updater-core/Data/";
        }

        if (isWorking) {
            trayIcon->setIcon(QIcon(iconPath + "working.png"));
            trayIcon->setToolTip(tr("Neko: Trabajando..."));
            statusAction->setText(tr("Estado: Trabajando..."));
        } else if (count > 0) {
            trayIcon->setIcon(QIcon(iconPath + "warnig.png"));
            trayIcon->setToolTip(tr("Neko: %1 actualizaciones disponibles").arg(count));
            statusAction->setText(tr("Actualizaciones: %1").arg(count));
        } else if (count == 0) {
            trayIcon->setIcon(QIcon(iconPath + "normal.png"));
            trayIcon->setToolTip(tr("Neko: Sistema actualizado"));
            statusAction->setText(tr("Sistema actualizado"));
        } else {
            trayIcon->setIcon(QIcon(iconPath + "normal.png"));
            trayIcon->setToolTip(tr("Neko: Estado desconocido"));
            statusAction->setText(tr("Estado desconocido"));
        }

        bool manual = false;
        QString freq = "1d"; // default
        QFile conf(QDir::homePath() + "/.config/NekoVoid/Updater.conf");
        if (conf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&conf);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("frequency=")) {
                    freq = line.split("=").last();
                    manual = (freq == "manual");
                }
            }
        }

        if (f12h && f1d && f3d && f1w && fman) {
            f12h->setChecked(freq == "12h");
            f12h->setEnabled(freq != "12h");
            
            f1d->setChecked(freq == "1d");
            f1d->setEnabled(freq != "1d");
            
            f3d->setChecked(freq == "3d");
            f3d->setEnabled(freq != "3d");
            
            f1w->setChecked(freq == "1w");
            f1w->setEnabled(freq != "1w");
            
            fman->setChecked(freq == "manual");
            fman->setEnabled(freq != "manual");
        }

        // Disable actions when working, or based on state
        checkAction->setEnabled(!isWorking && manual);
        applyAction->setEnabled(!isWorking && count > 0);
        cleanAction->setEnabled(!isWorking);
        malloc_trim(0); // Free memory back to OS
    }

    QString findTerminal() {
        // Terminales soportadas: alacritty, foot, mate-terminal, xfce4-terminal, st, kitty
        QStringList terms = {"alacritty", "foot", "kitty", "mate-terminal", "xfce4-terminal", "st", "xterm"};
        for (const QString &t : terms) {
            if (QFile::exists("/usr/bin/" + t) || QFile::exists("/usr/local/bin/" + t)) {
                return t;
            }
        }
        return "";
    }

    void launchInTerminal(const QString &script, const QString &fallbackArg) {
        QProcess *process = new QProcess(this);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process]() {
            isWorking = false;
            updateStatus();
            process->deleteLater();
        });

        QString termCmd = findTerminal();
        if (termCmd.isEmpty()) {
            // Fallback sin terminal: ejecutar directamente
            process->start("/usr/bin/neko-updater-core", {fallbackArg});
            return;
        }

        // Sintaxis por terminal:
        //  - alacritty/kitty/foot/st/xterm: -e seguido de argv separados
        //  - mate-terminal/xfce4-terminal: -e toma un UNICO string de comando
        if (termCmd == "mate-terminal" || termCmd == "xfce4-terminal") {
            process->start(termCmd, {"-e", "bash -lc \"" + script + "\""});
        } else {
            process->start(termCmd, {"-e", "bash", "-lc", script});
        }
    }

    void runCoreCommand(const QString &arg) {
        if (isWorking) return;
        isWorking = true;
        updateStatus();

        // Wrapper que eleva privilegios usando pkexec (Polkit), con doas/sudo como respaldo
        QString script = QString(
            "cmd='/usr/bin/neko-updater-core %1'; "
            "if command -v pkexec >/dev/null 2>&1; then "
            "    pkexec $cmd; "
            "elif command -v doas >/dev/null 2>&1 && [ -f /etc/doas.conf ]; then "
            "    doas $cmd; "
            "elif command -v sudo >/dev/null 2>&1 && [ -f /etc/sudoers ]; then "
            "    sudo $cmd; "
            "else echo '%2'; sleep 5; exit 1; fi; "
            "echo \"%3\"; read dummy"
        ).arg(arg)
         .arg(tr("No escalador de privilegios configurado"))
         .arg(tr("Presiona Enter para cerrar..."));

        launchInTerminal(script, arg);
    }

    void checkUpdates() {
        if (isWorking) return;
        isWorking = true;
        updateStatus();

        // La comprobación de actualizaciones no requiere privilegios de root,
        // así que no se usa sudo/doas/pkexec y no se pide clave.
        QString script = QString(
            "/usr/bin/neko-updater-core; "
            "echo \"%1\"; read dummy"
        ).arg(tr("Comprobación finalizada. Presiona Enter para cerrar..."));

        launchInTerminal(script, "");
    }

    void applyUpdates() {
        runCoreCommand("--apply");
    }

    void cleanSystem() {
        runCoreCommand("--clean");
    }

    void showAbout() {
        QMessageBox about;
        about.setWindowTitle(tr("Acerca de Neko Void Updater"));
        QString logoPath = "/usr/share/neko-void/logo.png";
        if (QFile::exists(logoPath)) {
            about.setIconPixmap(QPixmap(logoPath));
        }
        about.setTextFormat(Qt::RichText);
        about.setText(
            "<h3>Neko Void Updater</h3>"
            "<p><b>" + tr("Versión") + ":</b> 3.0.0</p>"
            "<p><b>" + tr("Gestor de actualizaciones para Void Linux") + "</b></p>"
            "<p>" + tr("Motor de actualizaciones XBPS y Flatpak con integración en la bandeja del sistema, "
            "actualizaciones automáticas y herramientas de mantenimiento.") + "</p>"
            "<p><b>" + tr("Repositorio") + ":</b> "
            "<a href=\"https://github.com/Neko-Void-Linux/Neko-Update\">github.com/Neko-Void-Linux/Neko-Update</a></p>"
        );
        about.exec();
    }

    void setFrequency(const QString &freq) {
        QString configDir = QDir::homePath() + "/.config/NekoVoid";
        QDir().mkpath(configDir);
        QFile file(configDir + "/Updater.conf");
        QString content = "[General]\nfrequency=" + freq + "\n";
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
        }
        updateStatus();
    }

private:
    void createActions() {
        statusAction = new QAction(tr("Estado"), this);
        statusAction->setEnabled(false);

        checkAction = new QAction(tr("Buscar actualizaciones"), this);
        connect(checkAction, &QAction::triggered, this, &NekoTray::checkUpdates);

        applyAction = new QAction(tr("Actualizar sistema"), this);
        connect(applyAction, &QAction::triggered, this, &NekoTray::applyUpdates);

        cleanAction = new QAction(tr("Limpiar sistema"), this);
        connect(cleanAction, &QAction::triggered, this, &NekoTray::cleanSystem);

        aboutAction = new QAction(tr("Acerca de"), this);
        connect(aboutAction, &QAction::triggered, this, &NekoTray::showAbout);

        quitAction = new QAction(tr("Salir"), this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    }

    void createTrayIcon() {
        trayMenu = new QMenu();
        // Mini-Matugen 3-color palette: Background #282a36, Text #f8f8f2, Accent #50fa7b
        trayMenu->setStyleSheet(
            "QMenu { background-color: #282a36; color: #f8f8f2; border: 1px solid #44475a; }"
            "QMenu::item:selected { background-color: #50fa7b; color: #282a36; }"
            "QMenu::separator { height: 1px; background: #44475a; margin: 4px; }"
        );
        
        trayMenu->addAction(statusAction);
        trayMenu->addSeparator();
        trayMenu->addAction(checkAction);
        trayMenu->addAction(applyAction);
        trayMenu->addAction(cleanAction);
        trayMenu->addSeparator();
        
        freqMenu = new QMenu(tr("Frecuencia"));
        QString style = "QMenu { background-color: #282a36; color: #f8f8f2; border: 1px solid #44475a; }"
                        "QMenu::item:selected { background-color: #50fa7b; color: #282a36; }";
        freqMenu->setStyleSheet(style);
        
        f12h = freqMenu->addAction(tr("Cada 12 horas"));
        f12h->setCheckable(true);
        connect(f12h, &QAction::triggered, this, [this]() { setFrequency("12h"); });
        
        f1d = freqMenu->addAction(tr("Diario"));
        f1d->setCheckable(true);
        connect(f1d, &QAction::triggered, this, [this]() { setFrequency("1d"); });
        
        f3d = freqMenu->addAction(tr("Cada 3 días"));
        f3d->setCheckable(true);
        connect(f3d, &QAction::triggered, this, [this]() { setFrequency("3d"); });
        
        f1w = freqMenu->addAction(tr("Semanal"));
        f1w->setCheckable(true);
        connect(f1w, &QAction::triggered, this, [this]() { setFrequency("1w"); });
        
        fman = freqMenu->addAction(tr("Manual"));
        fman->setCheckable(true);
        connect(fman, &QAction::triggered, this, [this]() { setFrequency("manual"); });
        
        trayMenu->addMenu(freqMenu);
        trayMenu->addSeparator();
        trayMenu->addAction(aboutAction);
        trayMenu->addAction(quitAction);

        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setContextMenu(trayMenu);
    }

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QMenu *freqMenu;
    QAction *statusAction;
    QAction *checkAction;
    QAction *applyAction;
    QAction *cleanAction;
    QAction *aboutAction;
    QAction *quitAction;
    QAction *f12h;
    QAction *f1d;
    QAction *f3d;
    QAction *f1w;
    QAction *fman;
    QFileSystemWatcher *watcher;
    bool isWorking;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Neko Void Updater");
    app.setQuitOnLastWindowClosed(false);

    // i18n: detect system locale and load matching translation automatically.
    // Supported: English (default), German, Portuguese, Italian, Spanish, Japanese.
    QString lang = QLocale::system().name().left(2).toLower();
    QStringList supported = {"de", "pt", "it", "es", "ja"};
    QTranslator translator;
    bool loaded = false;
    if (supported.contains(lang)) {
        QString qmName = QString("neko_%1.qm").arg(lang);
        QStringList qmCandidates = {
            QApplication::applicationDirPath() + "/../share/neko-void/translations/" + qmName,
            "/usr/share/neko-void/translations/" + qmName,
            QApplication::applicationDirPath() + "/translations/" + qmName,
            "neko-updater-tray/translations/" + qmName,
            "translations/" + qmName,
        };
        for (const QString &path : qmCandidates) {
            if (QFile::exists(path) && translator.load(path)) {
                loaded = true;
                break;
            }
        }
    }
    if (loaded) {
        app.installTranslator(&translator);
    }

    NekoTray tray;
    return app.exec();
}

#include "main.moc"

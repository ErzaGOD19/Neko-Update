/*
 * Neko Void Updater - Tray UI
 * Copyright (C) 2024 ErzaGOD19 - Trinity Community
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QUrl>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <limits>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTranslator>
#include <QLocale>

static constexpr auto kCountPath = "/tmp/neko_updates_count";
static constexpr auto kNotifyIcon = "system-software-update";

class NekoUpdaterBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int updateCount READ updateCount NOTIFY updateCountChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString frequency READ frequency NOTIFY frequencyChanged)
    Q_PROPERTY(bool hasUpdates READ hasUpdates NOTIFY hasUpdatesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool serviceActive READ serviceActive NOTIFY serviceActiveChanged)

public:
    explicit NekoUpdaterBackend(QObject *parent = nullptr)
        : QObject(parent)
        , m_checkProcess(new QProcess(this))
        , m_applyProcess(nullptr)
        , m_mainWindow(nullptr)
        , m_busy(false)
        , m_serviceActive(false)
    {
        // Dynamic core path detection
        m_corePath = "/usr/bin/neko-updater-core";
        if (!QFile::exists(m_corePath)) {
            // Try relative path for development
            QString localPath = QCoreApplication::applicationDirPath() + "/../neko-updater-core/target/release/neko-updater-core";
            if (QFile::exists(localPath)) {
                m_corePath = localPath;
            } else {
                // Try current directory as last resort
                if (QFile::exists("./neko-updater-core")) {
                    m_corePath = "./neko-updater-core";
                }
            }
        }
        qDebug() << "Núcleo detectado en:" << m_corePath;

        QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/NekoVoid/Updater.conf";
        QDir().mkpath(QFileInfo(configPath).path());
        m_settings = new QSettings(configPath, QSettings::IniFormat, this);

        m_frequency = m_settings->value("frequency", "1d").toString();
        m_lastCheck = m_settings->value("lastCheck", 0).toLongLong();

        connect(m_checkProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &NekoUpdaterBackend::onCheckFinished);
        connect(&m_timer, &QTimer::timeout, this, &NekoUpdaterBackend::checkNow);

        // Periodically check if the background service is running
        QTimer *serviceCheckTimer = new QTimer(this);
        connect(serviceCheckTimer, &QTimer::timeout, this, &NekoUpdaterBackend::checkServiceStatus);
        serviceCheckTimer->start(5000); // Every 5 seconds
        checkServiceStatus();

        // Schedule first check after a short delay (10s) to not freeze UI during init
        QTimer::singleShot(10000, this, [this](){ configureTimer(m_frequency); });
        updateStatusFromCountFile();
    }

    int updateCount() const { return m_updateCount; }
    QString statusText() const { return m_statusText; }
    QString frequency() const { return m_frequency; }
    bool hasUpdates() const { return m_updateCount > 0; }
    bool busy() const { return m_busy; }
    bool serviceActive() const { return m_serviceActive; }

    void setWindowObject(QObject *window)
    {
        m_mainWindow = window;
    }

    void checkServiceStatus()
    {
        QProcess p;
        p.start("pgrep", QStringList{"-f", m_corePath + " --sync-daemon"});
        p.waitForFinished();
        bool active = (p.exitCode() == 0);
        if (active != m_serviceActive) {
            m_serviceActive = active;
            emit serviceActiveChanged();
            configureTimer(m_frequency);
        }
    }

public slots:
    void checkNow()
    {
        if (m_checkProcess->state() != QProcess::NotRunning) {
            return;
        }

        m_busy = true;
        emit busyChanged();

        m_statusText = tr("Searching for updates...");
        emit statusTextChanged();
        qDebug() << "Executing check:" << m_corePath;

        m_checkProcess->start(m_corePath, QStringList());
        if (!m_checkProcess->waitForStarted(3000)) {
            m_busy = false;
            emit busyChanged();
            qDebug() << "Error: Failed to start check process.";
            notify(tr("Error checking updates"), tr("Failed to start check process."));
            m_checkProcess->kill();
            m_checkProcess->waitForFinished();
            m_statusText = tr("Check error");
            emit statusTextChanged();
        }
    }

    void applyUpdates()
    {
        if (m_applyProcess && m_applyProcess->state() != QProcess::NotRunning) {
            return;
        }

        if (!QFile::exists(m_corePath)) {
            qDebug() << "Error: Binary not found at" << m_corePath;
            notify(tr("Error"), tr("Update core not found."));
            return;
        }

        m_busy = true;
        emit busyChanged();
        m_statusText = tr("Starting update...");
        emit statusTextChanged();

        m_applyProcess = new QProcess(this);
        connect(m_applyProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &NekoUpdaterBackend::onApplyFinished);
        
        qDebug() << "Executing pkexec to apply updates";
        m_applyProcess->start("pkexec", QStringList{m_corePath, "--apply"});

        if (!m_applyProcess->waitForStarted(3000)) {
            m_busy = false;
            emit busyChanged();
            qDebug() << "Error: Failed to start pkexec";
            notify(tr("Error starting update"), tr("Failed to start pkexec."));
            m_applyProcess->deleteLater();
            m_applyProcess = nullptr;
            m_statusText = tr("Error starting pkexec");
            emit statusTextChanged();
            return;
        }

        notify(tr("Starting update..."), tr("Authorization requested to apply updates."), ":/icon-working.png");
    }

    void cleanSystem()
    {
        if ((m_applyProcess && m_applyProcess->state() != QProcess::NotRunning) || m_checkProcess->state() != QProcess::NotRunning) {
            return;
        }

        if (!QFile::exists(m_corePath)) {
            notify(tr("Error"), tr("Maintenance core not found."));
            return;
        }

        m_busy = true;
        emit busyChanged();
        m_statusText = tr("Cleaning system...");
        emit statusTextChanged();

        m_applyProcess = new QProcess(this);
        connect(m_applyProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &NekoUpdaterBackend::onApplyFinished);
        
        qDebug() << "Executing pkexec for system cleaning";
        m_applyProcess->start("pkexec", QStringList{m_corePath, "--clean"});

        if (!m_applyProcess->waitForStarted(3000)) {
            m_busy = false;
            emit busyChanged();
            notify(tr("Error"), tr("Failed to start cleaning."));
            m_applyProcess->deleteLater();
            m_applyProcess = nullptr;
            return;
        }

        notify(tr("Maintenance"), tr("Authorization requested to clean system."), ":/icon-working.png");
    }

    void setFrequency(const QString &frequency)
    {
        const QString normalized = frequency.trimmed();
        if (normalized.isEmpty() || normalized == m_frequency) {
            return;
        }

        m_frequency = normalized;
        m_settings->setValue("frequency", m_frequency);
        emit frequencyChanged();
        configureTimer(m_frequency);
    }

    void toggleWindowVisibility()
    {
        if (!m_mainWindow) {
            return;
        }

        const bool visible = m_mainWindow->property("visible").toBool();
        m_mainWindow->setProperty("visible", !visible);
    }

signals:
    void updateCountChanged();
    void statusTextChanged();
    void frequencyChanged();
    void hasUpdatesChanged();
    void busyChanged();
    void serviceActiveChanged();

private slots:
    void onCheckFinished(int exitCode, QProcess::ExitStatus status)
    {
        Q_UNUSED(status)
        m_busy = false;
        if (exitCode == 0) {
            m_lastCheck = QDateTime::currentSecsSinceEpoch();
            m_settings->setValue("lastCheck", m_lastCheck);
        }
        emit busyChanged();
        updateStatusFromCountFile();
        configureTimer(m_frequency); // Re-calculate next interval
    }

    void onApplyFinished(int exitCode, QProcess::ExitStatus status)
    {
        Q_UNUSED(status)
        m_busy = false;
        emit busyChanged();

        bool isClean = m_applyProcess->arguments().contains("--clean");

        if (exitCode == 0) {
            if (isClean) {
                notify(tr("System cleaned successfully."), tr("System maintenance completed."), ":/icon-normal.png");
            } else {
                notify(tr("System updated successfully."), tr("System update completed."), ":/icon-normal.png");
            }
        } else {
            if (isClean) {
                notify(tr("Cleaning failed."), tr("Could not clean the system."), ":/icon-warning.png");
            } else {
                notify(tr("Update cancelled or failed."), tr("Could not apply updates."), ":/icon-warning.png");
            }
        }

        if (m_applyProcess) {
            m_applyProcess->deleteLater();
            m_applyProcess = nullptr;
        }

        if (!isClean) {
            checkNow();
        }
    }

private:
    void configureTimer(const QString &frequency)
    {
        m_timer.stop();
        if (m_serviceActive) {
            qDebug() << "Background service active detected. Internal tray timer disabled.";
            return;
        }

        if (frequency == "manual") {
            m_statusText = tr("Manual check selected.");
            emit statusTextChanged();
            return;
        }

        const qint64 intervalSecs = frequency == "3d" ? 3LL * 24 * 60 * 60
                                    : frequency == "1w" ? 7LL * 24 * 60 * 60
                                    : 24LL * 60 * 60;
        
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        const qint64 elapsed = now - m_lastCheck;
        const qint64 remainingMs = qMax(1000LL, (intervalSecs - elapsed) * 1000);

        m_timer.setSingleShot(true); // Run once, then onCheckFinished will reschedule
        m_timer.start(static_cast<int>(qMin(remainingMs, static_cast<qint64>(std::numeric_limits<int>::max()))));
        
        qDebug() << "Next check in:" << (remainingMs / 1000) << "seconds";

        m_statusText = frequency == "3d" ? tr("Automatic check every 3 days.")
                         : frequency == "1w" ? tr("Automatic check every 1 week.")
                         : tr("Automatic check every 1 day.");
        emit statusTextChanged();
    }

    void updateStatusFromCountFile()
    {
        QFile countFile(QString::fromUtf8(kCountPath));
        int count = 0;

        if (countFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString text = QString::fromUtf8(countFile.readAll()).trimmed();
            bool ok = false;
            count = text.toInt(&ok);
            if (!ok) {
                count = 0;
            }
        }

        m_updateCount = count;
        emit updateCountChanged();
        emit hasUpdatesChanged();

        if (count <= 0) {
            m_statusText = tr("System up to date");
        } else {
            m_statusText = tr("%1 updates ready").arg(count);
            notify(tr("Updates available"), tr("Found %1 upgradable packages. Click tray icon to proceed.").arg(count), ":/icon-warning.png");
        }

        emit statusTextChanged();
    }

    void notify(const QString &title, const QString &message)
    {
        // Default to theme icon if resource extraction fails
        QString iconPath = ensureIconOnDisk(":/icon-normal.png", "icon-normal.png");
        QProcess::execute("notify-send", QStringList{QStringLiteral("-i"), iconPath, title, message});
    }

    void notify(const QString &title, const QString &message, const QString &resourcePath)
    {
        QString outName = resourcePath;
        if (outName.startsWith(":/")) outName = outName.mid(2);
        QString iconPath = ensureIconOnDisk(resourcePath, outName);
        QProcess::execute("notify-send", QStringList{QStringLiteral("-i"), iconPath, title, message});
    }

    QString ensureIconOnDisk(const QString &resourcePath, const QString &outName)
    {
        QString dirPath = QDir::tempPath() + "/neko-updater-icons";
        QDir dir;
        if (!dir.exists(dirPath)) dir.mkpath(dirPath);

        QString outPath = dirPath + "/" + outName;
        if (QFile::exists(outPath)) return outPath;

        QFile res(resourcePath);
        if (!res.exists()) return QString::fromUtf8(kNotifyIcon);
        if (!res.open(QIODevice::ReadOnly)) return QString::fromUtf8(kNotifyIcon);

        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly)) return QString::fromUtf8(kNotifyIcon);
        out.write(res.readAll());
        out.close();
        res.close();
        return outPath;
    }

    QSettings *m_settings;
    QProcess *m_checkProcess;
    QProcess *m_applyProcess;
    QObject *m_mainWindow;
    QTimer m_timer;
    qint64 m_lastCheck = 0;
    int m_updateCount = 0;
    QString m_statusText;
    QString m_frequency;
    QString m_corePath;
    bool m_busy = false;
    bool m_serviceActive = false;
};

int main(int argc, char *argv[])
{
    // Force Qt Quick Controls style to a neutral Basic style so our QML theme is consistent
    QQuickStyle::setStyle("Basic");
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/logo.png"));
    app.setQuitOnLastWindowClosed(false);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "neko_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    NekoUpdaterBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No se pudo cargar la interfaz QML.";
        return -1;
    }

    QObject *rootObject = engine.rootObjects().first();
    backend.setWindowObject(rootObject);

    QSystemTrayIcon trayIcon;
    QWidget *fallbackWidget = nullptr;
    // Use bundled icon resources
    trayIcon.setIcon(QIcon(":/icon-normal.png"));
    trayIcon.setToolTip("Neko Void Updater");

    QMenu trayMenu;
    QAction *statusAction = trayMenu.addAction(QObject::tr("Comprobando el estado..."));
    statusAction->setEnabled(false);
    trayMenu.addSeparator();
    QAction *showAction = trayMenu.addAction(QObject::tr("Mostrar/ocultar ventana"));
    QAction *applyAction = trayMenu.addAction(QObject::tr("Proceder a actualizar"));
    QAction *checkAction = trayMenu.addAction(QObject::tr("Buscar actualizaciones ahora"));
    QAction *quitAction = trayMenu.addAction(QObject::tr("Salir"));

    QMenu *frequencyMenu = trayMenu.addMenu(QObject::tr("Frecuencia de chequeo"));
    QActionGroup *frequencyGroup = new QActionGroup(&trayMenu);
    frequencyGroup->setExclusive(true);

    struct FrequencyOption { const char *key; const char *text; } freqOptions[] = {
        {"manual", QT_TR_NOOP("Manual")},
        {"1d", QT_TR_NOOP("Cada 1 Día")},
        {"3d", QT_TR_NOOP("Cada 3 Días")},
        {"1w", QT_TR_NOOP("Cada 1 Semana")},
    };

    for (auto &option : freqOptions) {
        QAction *action = frequencyMenu->addAction(QObject::tr(option.text));
        action->setCheckable(true);
        action->setData(QString::fromUtf8(option.key));
        frequencyGroup->addAction(action);
        if (QString::fromUtf8(option.key) == backend.frequency()) {
            action->setChecked(true);
        }
        QObject::connect(action, &QAction::triggered, [&backend, action]() {
            backend.setFrequency(action->data().toString());
        });
    }

    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            backend.toggleWindowVisibility();
        }
    });

    QObject::connect(showAction, &QAction::triggered, &backend, &NekoUpdaterBackend::toggleWindowVisibility);
    QObject::connect(checkAction, &QAction::triggered, &backend, &NekoUpdaterBackend::checkNow);
    QObject::connect(applyAction, &QAction::triggered, &backend, &NekoUpdaterBackend::applyUpdates);
    QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);

    QObject::connect(&backend, &NekoUpdaterBackend::statusTextChanged, [&]() {
        statusAction->setText(backend.statusText());
    });

    // Update tray icon and menu actions when update availability or busy state changes
    auto updateTrayIcon = [&]() {
        const bool busy = backend.busy();
        const bool hasUpdates = backend.hasUpdates();

        applyAction->setEnabled(!busy && hasUpdates);
        checkAction->setEnabled(!busy);

        if (busy) {
            trayIcon.setIcon(QIcon(":/icon-working.png"));
            if (fallbackWidget) {
                if (QPushButton *b = fallbackWidget->findChild<QPushButton*>("neko-fallback-btn"))
                    b->setIcon(QIcon(":/icon-working.png"));
            }
        } else {
            const QString iconPath = hasUpdates ? ":/icon-warning.png" : ":/icon-normal.png";
            trayIcon.setIcon(QIcon(iconPath));
            if (fallbackWidget) {
                if (QPushButton *b = fallbackWidget->findChild<QPushButton*>("neko-fallback-btn"))
                    b->setIcon(QIcon(iconPath));
            }
        }
    };

    QObject::connect(&backend, &NekoUpdaterBackend::hasUpdatesChanged, updateTrayIcon);
    QObject::connect(&backend, &NekoUpdaterBackend::busyChanged, updateTrayIcon);

    // Set initial state
    updateTrayIcon();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon.setContextMenu(&trayMenu);
        trayIcon.show();
    } else {
        // Fallback: create a small always-on-top clickable widget acting as tray icon
        fallbackWidget = new QWidget(nullptr, Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        fallbackWidget->setAttribute(Qt::WA_TranslucentBackground);
        fallbackWidget->setObjectName("neko-fallback-tray");

        QHBoxLayout *layout = new QHBoxLayout(fallbackWidget);
        layout->setContentsMargins(4,4,4,4);
        QPushButton *btn = new QPushButton(fallbackWidget);
        btn->setObjectName("neko-fallback-btn");
        btn->setIcon(QIcon(backend.hasUpdates() ? ":/icon-warning.png" : ":/icon-normal.png"));
        btn->setFlat(true);
        btn->setToolTip("Neko Void Updater");
        layout->addWidget(btn);
        fallbackWidget->setLayout(layout);

        // Position top-right of primary screen with small margin
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            const QRect g = screen->availableGeometry();
            const QSize s = fallbackWidget->sizeHint();
            const int margin = 8;
            fallbackWidget->setGeometry(g.right() - s.width() - margin, g.top() + margin, s.width(), s.height());
        }

        QObject::connect(btn, &QPushButton::clicked, [&backend]() { backend.toggleWindowVisibility(); });

        // Right-click shows context menu
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(btn, &QPushButton::customContextMenuRequested, [&trayMenu](const QPoint &p){
            Q_UNUSED(p)
            trayMenu.exec(QCursor::pos());
        });

        // Update fallback icon when update availability changes
        QObject::connect(&backend, &NekoUpdaterBackend::hasUpdatesChanged, [&backend, fallbackWidget]() {
            QPushButton *b = fallbackWidget->findChild<QPushButton*>("neko-fallback-btn");
            if (!b) return;
            b->setIcon(QIcon(backend.hasUpdates() ? ":/icon-warning.png" : ":/icon-normal.png"));
        });

        fallbackWidget->show();
    }

    return app.exec();
}

#include "main.moc"

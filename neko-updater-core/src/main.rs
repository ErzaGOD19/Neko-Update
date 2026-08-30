/*
 * Neko Void Updater - Core
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

use fs2::FileExt;
use std::env;
use std::fs::{self, File, OpenOptions};
use std::io::{self, Write};
use std::path::Path;
use std::process::{Command, ExitStatus, Stdio};

const LOCK_PATH: &str = "/tmp/neko_updater.lock";
const COUNT_PATH: &str = "/tmp/neko_updates_count";
const LOG_PATH: &str = "/tmp/neko_updater.log";

fn log(msg: &str) {
    let mut file = OpenOptions::new()
        .create(true)
        .append(true)
        .open(LOG_PATH)
        .unwrap();
    let now = chrono::Local::now().format("%Y-%m-%d %H:%M:%S");
    writeln!(file, "[{}] {}", now, msg).unwrap();
}

fn main() {
    log("Iniciando neko-updater-core");
    if let Err(err) = run() {
        log(&format!("ERROR FATAL: {}", err));
        eprintln!("neko-updater-core: {}", err);
        std::process::exit(1);
    }
}

fn run() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.iter().any(|arg| arg == "--apply") {
        log("Acción: Aplicar actualizaciones");
        apply_updates()
    } else if args.iter().any(|arg| arg == "--clean") {
        log("Acción: Limpieza de sistema");
        clean_system()
    } else if args.iter().any(|arg| arg == "--sync-daemon") {
        log("Acción: Modo Demonio de Sincronización");
        sync_daemon()
    } else {
        log("Acción: Comprobar actualizaciones");
        check_updates()
    }
}

fn sync_daemon() -> io::Result<()> {
    log("Iniciando demonio de sincronización en segundo plano...");
    loop {
        let frequency = read_config_value("frequency").unwrap_or_else(|| "1d".to_string());
        let last_check = read_config_value("lastCheck")
            .and_then(|v| v.parse::<i64>().ok())
            .unwrap_or(0);

        if frequency == "manual" {
            std::thread::sleep(std::time::Duration::from_secs(300));
            continue;
        }

        let interval_secs = match frequency.as_str() {
            "12h" => 12 * 3600,
            "1d" => 24 * 3600,
            "3d" => 3 * 24 * 3600,
            "1w" => 7 * 24 * 3600,
            _ => 24 * 3600, // Default 1d
        };

        let now = chrono::Local::now().timestamp();
        if now - last_check >= interval_secs {
            log(&format!("Sincronización programada iniciada (frecuencia: {}, última: {})", frequency, last_check));
            if let Err(e) = check_updates() {
                log(&format!("Error en sincronización del demonio: {}", e));
            }
            // Note: check_updates calls write_count_file, but we should also update lastCheck
            // however, the tray usually manages lastCheck. If the tray is not running,
            // we should update it here.
            let _ = update_config_last_check(now);
        }

        std::thread::sleep(std::time::Duration::from_secs(300)); // Poll every 5 minutes
    }
}

fn read_config_value(key: &str) -> Option<String> {
    let home = env::var("HOME").unwrap_or_else(|_| "/root".to_string());
    let config_path = Path::new(&home).join(".config/NekoVoid/Updater.conf");

    if let Ok(content) = fs::read_to_string(config_path) {
        for line in content.lines() {
            let line = line.trim();
            if line.starts_with(key) && line.contains('=') {
                return Some(line.split('=').last().unwrap_or("").trim().to_string());
            }
        }
    }
    None
}

fn update_config_last_check(timestamp: i64) -> io::Result<()> {
    let home = env::var("HOME").unwrap_or_else(|_| "/root".to_string());
    let config_dir = Path::new(&home).join(".config/NekoVoid");
    let config_path = config_dir.join("Updater.conf");

    let mut lines: Vec<String> = if config_path.exists() {
        fs::read_to_string(&config_path)?.lines().map(|s| s.to_string()).collect()
    } else {
        fs::create_dir_all(&config_dir)?;
        vec!["[General]".to_string()]
    };

    let mut found = false;
    for line in lines.iter_mut() {
        if line.starts_with("lastCheck=") {
            *line = format!("lastCheck={}", timestamp);
            found = true;
            break;
        }
    }

    if !found {
        lines.push(format!("lastCheck={}", timestamp));
    }

    let content = lines.join("\n");
    fs::write(config_path, content)?;
    Ok(())
}


fn run_command(program: &str, args: &[&str], capture: bool) -> io::Result<(ExitStatus, String)> {
    log(&format!("Ejecutando: {} {:?}", program, args));
    let mut cmd = Command::new(program);
    cmd.args(args);

    if capture {
        cmd.stdout(Stdio::piped());
        cmd.stderr(Stdio::piped());
    } else {
        cmd.stdout(Stdio::inherit());
        cmd.stderr(Stdio::inherit());
    }

    let output = cmd.output()?;
    let stdout = if capture {
        String::from_utf8_lossy(&output.stdout).into_owned()
    } else {
        String::new()
    };

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        log(&format!("Comando falló con código {}: {}", output.status.code().unwrap_or(-1), stderr.trim()));
        return Err(io::Error::new(
            io::ErrorKind::Other,
            format!(
                "Command `{}` failed with code {}: {}",
                program,
                output.status.code().unwrap_or(-1),
                stderr.trim()
            ),
        ));
    }

    Ok((output.status, stdout))
}

fn has_internet() -> bool {
    Command::new("ping")
        .args(&["-c", "1", "-W", "2", "8.8.8.8"])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .map(|s| s.success())
        .unwrap_or(false)
}

fn check_updates() -> io::Result<()> {
    if !has_internet() {
        log("Error: No hay conexión a internet. Saltando comprobación.");
        return Ok(());
    }

    let mut total_count = 0;
    
    log("Sincronizando índices en memoria para buscar actualizaciones...");
    if let Ok((_, output)) = run_command("xbps-install", &["-Mun"], true) {
        let count = parse_update_count(&output);
        log(&format!("XBPS encontró {} actualizaciones", count));
        total_count += count;
    }

    if Path::new("/usr/bin/flatpak").exists() {
        if let Ok((_, output)) = run_command("flatpak", &["update", "--noninteractive"], true) {
             let lower_out = output.to_lowercase();
             if !lower_out.contains("nothing to do") && !lower_out.contains("nothing to update") && !lower_out.contains("nada que hacer") {
                 let flatpak_count = output.lines().filter(|l| {
                     let lower = l.to_lowercase();
                     // Ignorar advertencias, errores y líneas vacías
                     if lower.contains("advertencia:") || lower.contains("warning:") || lower.contains("error:") || l.trim().is_empty() || lower.contains("dconf-critical") {
                         return false;
                     }
                     // Contar solo las líneas de actualización reales (suelen tener un punto o tabulaciones, "i" de install/update)
                     // Flatpak suele poner "ID  Branch  Op"
                     l.contains("\t") || l.contains(" i ") || l.contains(" u ")
                 }).count();
                 // Descontar la cabecera si existe
                 let final_flatpak_count = if flatpak_count > 0 { flatpak_count.saturating_sub(1) } else { 0 };
                 log(&format!("Flatpak encontró {} actualizaciones", final_flatpak_count));
                 total_count += final_flatpak_count;
             }
        }
    }

    log(&format!("Total actualizaciones encontradas: {}", total_count));
    write_count_file(total_count)
}

fn parse_update_count(output: &str) -> usize {
    // xbps-install -un normally outputs lines like:
    // pkgname-version update arch repo dl_size installed_size
    let mut count = 0;
    for line in output.lines() {
        let line = line.trim();
        if line.is_empty() { continue; }
        let lower = line.to_lowercase();
        // If it says it is up to date or no packages, abort and return 0
        if lower.contains("nothing to do") || lower.contains("no packages") || lower.contains("0 downloaded") {
            return 0;
        }
        // Count valid package lines that contain " update " or " -> "
        if line.contains(" update ") || line.contains(" -> ") {
            count += 1;
        }
    }
    count
}

fn write_count_file(count: usize) -> io::Result<()> {
    let path = Path::new(COUNT_PATH);
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }

    let mut file = File::create(path)?;
    writeln!(file, "{}", count)?;
    Ok(())
}

fn clean_system() -> io::Result<()> {
    log("Iniciando limpieza profunda (huerfanos y caché)...");
    let _ = run_command("xbps-remove", &["-Ooy"], false);

    if Path::new("/usr/bin/flatpak").exists() {
        log("Limpiando basura y dependencias huérfanas de Flatpak...");
        let _ = run_command("flatpak", &["uninstall", "--unused", "-y"], false);
    }

    if let Ok(user) = env::var("SUDO_USER").or_else(|_| env::var("DOAS_USER")) {
        let thumb_path = format!("/home/{}/.cache/thumbnails", user);
        if Path::new(&thumb_path).exists() {
            log("Limpiando caché de miniaturas del usuario...");
            let _ = run_command("rm", &["-rf", &thumb_path], false);
        }
    }

    log("Limpieza completada");
    Ok(())
}

fn apply_updates() -> io::Result<()> {
    let lock_file = OpenOptions::new()
        .create(true)
        .write(true)
        .open(LOCK_PATH)?;

    lock_file.try_lock_exclusive().map_err(|_| {
        log("Error: Otro proceso ya tiene el lock");
        io::Error::new(
            io::ErrorKind::AlreadyExists,
            "Otro proceso de actualización ya se está ejecutando",
        )
    })?;

    log("Aplicando actualizaciones XBPS...");
    run_command("xbps-install", &["-Sy"], false)?;
    run_command("xbps-install", &["-uy"], false)?;

    if Path::new("/usr/bin/flatpak").exists() {
        log("Aplicando actualizaciones Flatpak...");
        let _ = run_command("flatpak", &["update", "-y"], false);
    }

    log("Actualización completada exitosamente");
    let _ = write_count_file(0);
    
    Ok(())
}

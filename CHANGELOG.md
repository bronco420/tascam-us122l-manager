# Changelog

All notable changes to the Tascam US-122L Manager will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.3.2] - 2026-08-19

### Fixed
- **PipeWire-JACK bridge broken** (`pipewire_bridge.cpp`): `pactl load-module`
  returns a bare numeric ID (e.g. `536870916`), not `module-N`. The module-ID
  parsing rejected every load, so the bridge could never be enabled. Both
  `module-` and bare IDs are now handled and `pactl.exitCode()` is checked.
- **Watchdog did nothing in `--watch` mode** (`main.cpp`): the CLI handler
  started the watchdog but immediately returned, killing the process before
  any QTimer could fire. `--watch` now enters `app.exec()` so the watchdog
  monitors continuously.
- **`--watch-stop` never stopped the watchdog**: it called `stop()` on a
  local, non-running instance. It now reads `watchdog.pid`, sends `kill` and
  removes the pid file, so systemd/user-session stops work.
- **Stale watchdog pid file blocked startup** (`watchdog.cpp`): `kill -0`
  was judged by `waitForFinished` (always true) instead of `exitCode()`,
  so a pid file left by a crashed watchdog prevented restart forever. Stale
  pid files are now detected and reclaimed.
- **`~/.asoundrc` clobbered** (`sysmode.cpp`): `ensureUsbStreamConfig()`
  overwrote the whole file instead of repairing just the `usb_stream` block.
  Now appends, preserving user config.
- **Fresh install had no settings dir** (`config.cpp`): `initPaths()` created
  the icons/product dirs but not the settings dir, so `initSettings()`
  silently failed on first run. Missing `QDir().mkpath(m_settingsDir)`.
- **`--sysmode` left orphan modules** (`sysmode.cpp`): `isActive()` only
  checked the sink, so a leftover source was never cleaned up. It now checks
  both sink and source.
- **JACK shared-memory cleanup ineffective** (`jack_manager.cpp`):
  `/dev/shm/jack-*` globs were passed literally to `rm` (no shell expansion).
  Globs are now expanded with `QDir::entryList()`.
- **Firmware string corrupted** (`utils.cpp`): `QString("%1.%02d")` printed a
  literal `%02d` (e.g. `1.%02d`). Fixed with Qt zero-padding. A new unit test
  also revealed `bcdDevice` is Binary-Coded Decimal, not hex: `0111` is
  `1.11`, not `1.17`. Now `1.00` for the US-122L.
- **Watchdog backoff never reset** (`watchdog.cpp`): `resetBackoff()` was
  never called; `checkJackStatus()` now resets it after a recovered check.
- **Dead `--action:` CLI code removed** (`main.cpp`): ~190 lines of an
  unreachable handler block plus the orphaned `silent` variable were deleted.

### Docs
- Consolidated project reports into [`docs/HISTORY.md`](docs/HISTORY.md).
- README/QUICKSTART no longer document non-existent `--action:*` flags.
- Added CI build badge, Tascam asset disclaimer and roadmap section.

## [2.3.1] - 2026-08-16

### Fixed
- **Startup hang (critical)**: `Mixer::Mixer()` called `QThread::sleep(500)` which sleeps **seconds**, not milliseconds → 8+ minute block on every launch. Removed along with the now-unused `QThread` include.
- **`--status` hang**: `QTextStream` on virtual files (`/proc/asound/cards`, `/proc/modules`) never read anything because `size()==0`; switched to `readAll()` + `contains()`/regex.
- **Zombie processes reported as running**: `pgrep -x jackd` matched `<defunct>` processes. New `Utils::getLivePids()` filters zombies via `/proc/PID/stat`; used by `Utils::isJackRunning()`, `JackManager::getPID()`, `Diagnostics::getJackPID()`.
- **Missing CLI handlers**: `--start/--stop/--bridge/--sysmode/--watch/--watch-stop/--autostart` were parsed but never handled (they fell through to the GUI). Added full handlers; `--silent` suppresses modal dialogs.
- **JackManager start**: launcher args now built as `--pidfile FILE -- jackd [args]` (match `tascam-jackd-launcher.py`).
- **Sysmode `off`**: module unload now falls back to name discovery (`unloadModulesByName`) when saved IDs are missing.
- **`state.conf` corruption**: `Sysmode::savePreviousState()` and bridge sink helpers removed the state file and renamed a non-existent `.tmp`. Replaced with `Utils::updateStateFile()` which preserves other entries.
- **Destructor auto-disable**: `Sysmode`/`PipeWireBridge` no longer tear down state on process exit (broke CLI one-shot usage).

### Changed
- Removed HTML dashboard, HTTP web server (port 9900), WebEngineWidgets, `WebDashboardWidget`, `server/web_server.*`, `dashboard.html`. Dashboard is now a native Qt `QWidget` with `actionRequested(QString)` signal.

## [2.3] - 2026-08-15

### Added
- Complete C++ rewrite of the Tascam US-122L Manager
- Qt6-based GUI with modern design
- JACK server management with proper error handling
- PipeWire-JACK bridge implementation
- System card mode (Tascam as system audio)
- Real-time mixer controls with volume and mute
- Quick presets (Studio, Standard, Live, Hi-Res)
- Watchdog for automatic JACK restart
- Auto-start at login with systemd integration
- Full diagnostics and MIDI support
- HTTP server for web-based dashboard
- Comprehensive documentation

### Changed
- Replaced bash script with C++ application
- Improved performance with native C++ code
- Better error handling and recovery
- Enhanced GUI with Qt6 widgets
- More robust JACK management
- Improved PipeWire integration

### Technical
- C++17 standard
- Qt6 framework
- CMake build system
- Modular architecture
- Thread-safe operations
- Proper resource management

## [2.2] - 2026-08-15

### Added
- Mixer real sink/source controls
- Quick presets functionality
- Fix for JACK⇄Scheda di Sistema cycle
- Bug fix for settings.conf writing

### Changed
- Mixer implementation
- Preset system
- JACK lifecycle management

## [2.1] - 2026-08-14

### Added
- Complete JACK⇄Scheda di Sistema cycle
- Sysmode enable/disable with automatic restoration
- Bridge PipeWire-JACK toggle

### Changed
- Sysmode implementation
- JACK lifecycle management

## [2.0] - 2026-08-13

### Added
- Bridge PipeWire-JACK
- Sysmode (Tascam as system card)
- JACK server management
- Configurable sample rate, buffer size, periods
- Auto-start JACK at login
- Watchdog for JACK restart
- Diagnostics and MIDI support

### Changed
- Complete rewrite of JACK management
- New configuration system
- Improved error handling

## [1.8] - 2026-08-12

### Added
- Auto-start JACK at login
- Watchdog for JACK restart
- Improved GUI

### Changed
- Auto-start implementation
- Watchdog implementation

## [1.7] - 2026-08-11

### Added
- Complete diagnostics
- Audio routing information

### Changed
- Diagnostics implementation

## [1.6] - 2026-08-10

### Added
- Bridge PipeWire-JACK
- Virtual mixer

### Changed
- Bridge implementation
- Mixer implementation

## [1.5] - 2026-08-09

### Added
- Scheda di sistema
- jackdmp support
- pipewire-pulse integration

### Changed
- Jackdmp support
- PipeWire integration

## [1.4] - 2026-08-08

### Added
- Jackdmp support
- asoundrc wrapper
- detect_tascam_model

### Changed
- Jackdmp support
- asoundrc configuration

## [1.3] - 2026-08-07

### Added
- Bridge PipeWire-JACK
- Mixer virtuale

### Changed
- Bridge implementation
- Mixer implementation

## [1.2] - 2026-08-06

### Added
- detect_tascam_model
- get_usb_connection

### Changed
- USB device detection

## [1.1] - 2026-08-05

### Added
- Bridge PipeWire-JACK
- Mixer virtuale

### Changed
- Bridge implementation
- Mixer implementation

## [1.0] - 2026-08-04

### Added
- First version of Tascam US-122L Manager
- JACK server management
- GUI with yad+WebKit
- Bridge PipeWire-JACK
- Mixer virtuale
- Diagnostica
- Documentazione
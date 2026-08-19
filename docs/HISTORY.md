# Project History

Chronicle of the Tascam US-122L Manager project — from the original bash
version through the C++17 rewrite and the 2026 maintenance pass.

## Origins

The tool started as a **bash script** (`tascam-us122l-manager.sh`) wrapping
JACK, ALSA `usb_stream`, PipeWire and `yad`-based dashboards for the Tascam
US-122L audio interface on Linux. It grew to ~1900 lines and covered:

- JACK server start/stop with configurable sample rate / buffer / periods
- PipeWire–JACK bridge
- System-card mode (Tascam as default sink/source)
- Watchdog, autostart, diagnostics, MIDI loopback test
- HTML "Control Panel" dashboard

## C++17 Rewrite (2026-08-15/16)

The bash script was fully reimplemented in **C++17 with Qt6 (Widgets)**,
keeping 100% feature parity:

- Modular core: `JackManager`, `PipeWireBridge`, `Sysmode`, `Mixer`,
  `Preset`, `Watchdog`, `Diagnostics`, `Config`, `Utils`
- Native Qt GUI (MainWindow + dashboard/mixer/preset/config/info/docs/diag
  widgets) with a dark "Studio Rack" QSS theme
- The HTML dashboard and HTTP web server were **removed** in favor of the
  native Qt dashboard (no WebEngine dependency)
- CLI preserved: `--start/--stop/--restart/--status/--diag/--bridge/
  --sysmode/--watch/--watch-stop/--autostart/--silent`

Key fixes during the rewrite (see `CHANGELOG.md`):

- Startup hang: `Mixer` slept **seconds** instead of milliseconds
- `--status` hang on virtual proc files
- Zombie processes falsely reported as running
- `state.conf`/`.asoundrc` writes hardened

## Maintenance Pass (2026-08-19)

Full bug-hunting audit on the C++ codebase. All 11 findings fixed and
verified:

1. `pactl load-module` returns a bare numeric ID, not `module-N` —
   `pipewire_bridge.cpp` was rejecting every module load
2. `--watch` exited immediately (`return 0` after `watchdog.start()`),
   so the watchdog never monitored anything — now runs `app.exec()`
3. `--watch-stop` called `stop()` on a local, non-running instance
   instead of stopping the cross-process watchdog via its pid file
4. Watchdog treated any finished `kill -0` check as "running" —
   stale pid files blocked startup forever
5. `.asoundrc` repair clobbered the user config; now appends only
6. `Config` never created its settings dir on fresh installs
7. `sysmode.isActive()` ignored the source, leaving orphan modules
8. `/dev/shm/jack-*` globs were passed literally to `rm` (no shell)
9. Firmware string printed `1.%02d` instead of `1.00`
10. Watchdog backoff never reset after recovery
11. ~190 lines of dead `--action:` CLI code removed

Full detail in `TascamUS122L Technical Notes` (internal vault) and the
commit history.
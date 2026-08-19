# Architecture Documentation

## Overview

The Tascam US-122L Manager is a professional audio interface control tool built in C++17 with Qt6. It provides complete management of JACK audio server, PipeWire integration, and system card mode for the Tascam US-122L audio interface.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ MainWindow│  │Dashboard │  │ Mixer    │  │ Preset   │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      Core Layer                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ JackMgr  │  │PipeBridge│  │ Sysmode  │  │ Mixer    │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Preset   │  │ Watchdog │  │ Diagnostics│  │ Config   │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      System Layer                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ JACK     │  │PipeWire  │  │ ALSA     │  │ systemd  │  │
│  │ (jackd)  │  │ (pipewire)│  │ (alsa-lib)│  │ (service)│  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Module Descriptions

### Core Modules

#### JackManager
- **Responsibility**: Manages JACK audio server lifecycle
- **Key Functions**:
  - Start/stop JACK server
  - Configure sample rate, buffer size, periods
  - Save/load JACK settings
  - Handle JACK process signals
  - Cleanup JACK resources
- **Dependencies**: Config, Utils
- **Signals**: started, stopped, errorOccurred

#### PipeWireBridge
- **Responsibility**: Manages PipeWire-JACK bridge
- **Key Functions**:
  - Enable/disable bridge
  - Load/unload JACK modules
  - Manage JACK sink/source
  - Save/restore previous sink
- **Dependencies**: Config
- **Signals**: bridgeEnabled, bridgeDisabled

#### Sysmode
- **Responsibility**: Manages system card mode (Tascam as system audio)
- **Key Functions**:
  - Enable/disable system card
  - Load/unload ALSA modules
  - Manage PCH card profile
  - Auto-restore on PipeWire restart
- **Dependencies**: Config
- **Signals**: sysmodeEnabled, sysmodeDisabled

#### Mixer
- **Responsibility**: Provides volume and mute controls
- **Key Functions**:
  - Get/set sink volume
  - Get/set source volume
  - Adjust volume by delta
  - Toggle mute
  - Monitor volume changes
- **Dependencies**: Config
- **Signals**: sinkVolumeChanged, sourceVolumeChanged, sinkMuteChanged, sourceMuteChanged

#### Preset
- **Responsibility**: Manages quick presets
- **Key Functions**:
  - Apply preset (Studio, Standard, Live, Hi-Res)
  - Save current settings
  - Load settings
  - Validate presets
- **Dependencies**: Config
- **Signals**: presetApplied

#### Watchdog
- **Responsibility**: Automatic JACK restart on failure
- **Key Functions**:
  - Monitor JACK status
  - Restart JACK if it dies
  - Backoff after multiple failures
  - Handle sysmode auto-restore
- **Dependencies**: JackManager, Config
- **Signals**: started, stopped, jackRestarted, sysmodeAutoRestore

#### Diagnostics
- **Responsibility**: Provides system diagnostics
- **Key Functions**:
  - Generate comprehensive report
  - Test MIDI loopback
  - Check system status
  - Log analysis
- **Dependencies**: Config
- **Outputs**: Full diagnostics report

#### Config
- **Responsibility**: Manages configuration files
- **Key Functions**:
  - Save/load settings
  - Manage state file
  - Handle auto-start
  - Manage asset paths
- **Dependencies**: None
- **Singleton**: Yes

#### Utils
- **Responsibility**: Common utility functions
- **Key Functions**:
  - USB device detection
  - ALSA operations
  - JACK operations
  - PipeWire operations
  - MIDI operations
  - String operations
- **Dependencies**: None

## GUI Architecture

### Main Window
- **Responsibility**: Main application window
- **Layout**: Stacked widget with multiple views
- **Components**:
  - Dashboard widget (default)
  - Mixer widget
  - Preset widget
  - Config widget
  - Info widget
  - Documentation widget
  - Diagnostics widget

### Dashboard Widget
- **Responsibility**: Main control panel
- **Features**:
  - Real-time status display
  - Action buttons
  - Status indicators
  - Periodic refresh

### Other Widgets
- Each widget specializes in a specific functionality
- All widgets inherit from QWidget
- Widgets communicate via signals/slots

## Native Dashboard (Qt Widgets)

- **Responsibility**: Native QWidget control panel (replaces the former HTML dashboard + WebEngine/HTTP server, which were removed)
- **DashboardWidget**: QWidget with status groups and buttons, emits `actionRequested(QString)`
- **MainWindow**: dispatches actions via `onActionRequested()` and refreshes via `updateStatusWidgets()`
- No network layer (port 9900 removed)

## Data Flow

### JACK Startup
```
User Action → Main Window → JackManager → JACK Process → ALSA Driver → US-122L
```

### Bridge Enable
```
User Action → Main Window → PipeWireBridge → Load Modules → PipeWire → JACK
```

### System Card Enable
```
User Action → Main Window → Sysmode → Load ALSA Modules → PipeWire → US-122L
```

### Mixer Control
```
User Action → Main Window → Mixer → pactl → PipeWire → US-122L
```

### Watchdog
```
Timer → Check JACK Status → JACK Dead → Restart JACK → Notify
```

## Configuration Files

### ~/.asoundrc
- ALSA configuration
- Defines usb_stream plugin
- Required for US-122L

### ~/.config/tascam-us122l/settings.conf
- JACK parameters (sample rate, buffer size, periods)
- Written by Preset module
- Read by JackManager

### ~/.config/tascam-us122l/state.conf
- Runtime state
- Saved by various modules
- Contains: SYSMODE, PREV_SINK, etc.

### ~/.config/tascam-us122l/jack.log
- JACK server log
- Rotated at 1MB
- Used by Diagnostics

### ~/.config/systemd/user/tascam-us122l.service
- Auto-start service
- Runs watchdog
- Manages JACK lifecycle

## Thread Safety

- JACK operations are synchronous (blocking)
- Watchdog runs in separate thread
- GUI operations are on main thread
- No concurrent access to PipeWire/Pactl (uses locks)

## Performance Considerations

- JACK operations are blocking but necessary
- Watchdog checks every 5 seconds
- GUI refresh every 3 seconds
- No heavy CPU usage
- Minimal memory footprint

## Error Handling

- All JACK operations check return codes
- Watchdog handles JACK failures
- GUI shows error messages
- Config operations validate inputs
- Systemd service handles crashes

## Future Enhancements

- Async JACK operations
- Real-time audio visualization
- MIDI routing editor
- Audio recording/playback
- Network streaming
- Plugin architecture
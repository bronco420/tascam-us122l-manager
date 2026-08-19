# Quick Start Guide

## Installation

### 1. Install Dependencies
```bash
su -c 'pacman -S cmake ninja qt6-base jack pipewire alsa-lib'
```

### 2. Build
```bash
cd tascam-manager
./build.sh
```

### 3. Run
```bash
tascam-us122l-manager
```

## Basic Usage

### Start JACK
1. Launch the application
2. Click "Start JACK"
3. JACK will start with saved settings

### Adjust Volume
1. Click "Open Mixer"
2. Use + and - buttons to adjust volume
3. Click "Mute" to toggle mute

### Apply Preset
1. Click "Open Presets"
2. Select a preset (Studio, Standard, Live, Hi-Res)
3. Settings will be applied automatically

### Enable Bridge
1. Start JACK
2. Click "Toggle Bridge"
3. System audio will go to Tascam

### Enable System Card
1. Stop JACK
2. Click "Toggle System Card"
3. Tascam becomes system audio

## Common Commands

```bash
# Start JACK
tascam-us122l-manager --start

# Stop JACK
tascam-us122l-manager --stop

# Show status
tascam-us122l-manager --status

# Run diagnostics
tascam-us122l-manager --diag

# Toggle bridge
tascam-us122l-manager --bridge toggle

# Toggle system card
tascam-us122l-manager --sysmode toggle

# Start watchdog
tascam-us122l-manager --watch

# Enable auto-start
tascam-us122l-manager --autostart on
```

## Troubleshooting

### JACK won't start
```bash
# Check card detection
cat /proc/asound/cards

# Check driver loading
lsmod | grep snd_usb_us122l

# Check JACK log
cat ~/.config/tascam-us122l/jack.log
```

### Audio not working
```bash
# Verify JACK is running
jack_lsp -l

# Check bridge status
pactl list short modules | grep jack

# Check system card
pactl list short sinks | grep US122L
```

### MIDI not working
```bash
# Check MIDI device
amidi -l
```

## Getting Help

- Status: `tascam-us122l-manager --status`
- Diagnostics: `tascam-us122l-manager --diag`
- Full help: `tascam-us122l-manager --help`

---

For more detailed information, see [`README.md`](../README.md),
[`INSTALL.md`](../INSTALL.md), and [`ARCHITECTURE.md`](ARCHITECTURE.md).
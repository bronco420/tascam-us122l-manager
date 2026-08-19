# Installation Guide

## Prerequisites

Make sure you have all required packages installed:

```bash
su -c 'pacman -S cmake ninja qt6-base jack pipewire alsa-lib'
```

## Building

1. Navigate to the project directory:
```bash
cd tascam-manager
```

2. Run the build script:
```bash
./build.sh
```

This will:
- Check for all dependencies
- Configure CMake
- Build the project
- Install the binary to `/usr/bin/tascam-us122l-manager`

## Manual Build

If you prefer to build manually:

```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --install .
```

## Running

After installation, you can run the application:

```bash
tascam-us122l-manager
```

Or from the build directory:

```bash
cd build
./tascam-us122l-manager
```

## Uninstallation

To uninstall:

```bash
sudo rm /usr/bin/tascam-us122l-manager
sudo rm /etc/systemd/user/tascam-us122l.service
sudo systemctl --user daemon-reload
```

## Troubleshooting

### Build fails with "Qt6 not found"

Make sure Qt6 is installed:
```bash
su -c 'pacman -S qt6-base'
```

### Build fails with "JACK not found"

Make sure JACK is installed:
```bash
su -c 'pacman -S jack'
```

### Build fails with "PipeWire not found"

Make sure PipeWire is installed:
```bash
su -c 'pacman -S pipewire'
```

### Binary won't run

Make sure you're in the audio group:
```bash
groups
# Should include 'audio'
```

If not, add yourself:
```bash
su -c 'gpasswd -a $USER audio'
# Then logout and login again
```
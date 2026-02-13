# RootStream KDE Plasma Client - Building

## Dependencies

### Arch Linux / CachyOS

```bash
# Core dependencies
sudo pacman -S base-devel cmake qt6-base qt6-declarative qt6-quickcontrols2

# RootStream dependencies
sudo pacman -S libsodium opus

# Optional: Hardware decoding
sudo pacman -S libva libva-intel-driver  # Intel
sudo pacman -S libva mesa              # AMD
sudo pacman -S libva-vdpau-driver      # NVIDIA

# Optional: Audio
sudo pacman -S libpulse pipewire

# Optional: KDE integration
sudo pacman -S kconfig kcoreaddons
```

### Ubuntu 22.04+

```bash
# Core dependencies
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls

# RootStream dependencies
sudo apt install libsodium-dev libopus-dev

# Optional: Hardware decoding
sudo apt install libva-dev mesa-va-drivers

# Optional: Audio
sudo apt install libpulse-dev pipewire-dev

# Optional: KDE integration
sudo apt install libkf6config-dev libkf6coreaddons-dev
```

### Fedora

```bash
# Core dependencies
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel

# RootStream dependencies
sudo dnf install libsodium-devel opus-devel

# Optional: Hardware decoding
sudo dnf install libva-devel mesa-va-drivers

# Optional: Audio
sudo dnf install pulseaudio-libs-devel pipewire-devel

# Optional: KDE integration
sudo dnf install kf6-kconfig-devel kf6-kcoreaddons-devel
```

## Building

### Quick Build

```bash
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream/clients/kde-plasma-client
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### Build Options

```bash
# Debug build with AI logging
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_AI_LOGGING=ON \
      ..

# Release build without AI logging
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_AI_LOGGING=OFF \
      ..

# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

### Building RootStream Library First

The client requires the RootStream library (libRootStream). Build it first:

```bash
cd RootStream
make
# or with CMake:
mkdir build && cd build
cmake ..
make
```

Then build the client, making sure it can find the library:

```bash
cd ../clients/kde-plasma-client
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Installation

### System-wide Installation

```bash
sudo make install
```

This installs:
- `/usr/bin/rootstream-kde-client` - Executable
- `/usr/share/applications/rootstream-kde-client.desktop` - Desktop entry
- `/usr/share/icons/hicolor/scalable/apps/rootstream-kde-client.svg` - Icon

### Local Installation

```bash
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make install
```

## Running

### From Build Directory

```bash
./rootstream-kde-client
```

### From Installation

```bash
rootstream-kde-client
```

### With Options

```bash
# Enable AI logging
rootstream-kde-client --ai-logging

# Auto-connect to peer
rootstream-kde-client --connect "kXx7YqZ3...@hostname"
```

## Troubleshooting

### Qt not found

```bash
# Set Qt6 path if not auto-detected
export CMAKE_PREFIX_PATH=/usr/lib/qt6
cmake ..
```

### libRootStream not found

```bash
# Build and install libRootStream first
cd ../../
make
sudo make install
```

### Missing KDE Frameworks

KDE Frameworks are optional. If not found, the build will continue without KConfig integration.

### VA-API not found

VA-API is optional. Software decoding will be used as fallback.

### Build fails with Qt version error

Ensure Qt 6.4+ is installed:

```bash
qmake6 --version  # Should show Qt 6.4.0 or higher
```

## Packaging

### Arch Linux PKGBUILD

```bash
cd packaging
makepkg -si
```

### Create Tarball

```bash
cd ..
tar czf rootstream-kde-client-1.0.0.tar.gz \
    --exclude=build \
    --exclude=.git \
    kde-plasma-client/
```

## Development Build

### With Debug Symbols

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-g -O0" \
      ..
make -j$(nproc)
```

### With Sanitizers

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined" \
      ..
make -j$(nproc)
```

### With Coverage

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage" \
      ..
make -j$(nproc)
# Run tests
ctest
# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Uninstalling

```bash
sudo make uninstall
# or manually:
sudo rm /usr/bin/rootstream-kde-client
sudo rm /usr/share/applications/rootstream-kde-client.desktop
sudo rm /usr/share/icons/hicolor/scalable/apps/rootstream-kde-client.svg
```

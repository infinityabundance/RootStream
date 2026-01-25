# RootStream - Secure P2P Game Streaming
# Professional Makefile with full dependency management

# ============================================================================
# Configuration
# ============================================================================

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=gnu11 -O2 -D_GNU_SOURCE
CFLAGS += -I./include

# Debug flags (use: make DEBUG=1)
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
    LDFLAGS += -g
else
    CFLAGS += -DNDEBUG
endif

# Libraries
LIBS := -ldrm -lva -lva-drm -lpthread -lsodium -lqrencode -lpng

# GTK3
CFLAGS += $(shell pkg-config --cflags gtk+-3.0)
LIBS += $(shell pkg-config --libs gtk+-3.0)

# Avahi (optional)
ifeq ($(shell pkg-config --exists avahi-client && echo yes),yes)
    CFLAGS += -DHAVE_AVAHI $(shell pkg-config --cflags avahi-client)
    LIBS += $(shell pkg-config --libs avahi-client)
endif

# ============================================================================
# Targets
# ============================================================================

# Binary name
TARGET := rootstream

# Source files
SRCS := src/main.c \
        src/drm_capture.c \
        src/vaapi_encoder.c \
        src/network.c \
        src/input.c \
        src/crypto.c \
        src/discovery.c \
        src/tray.c \
        src/service.c \
        src/qrcode.c \
        src/config.c

# Object files
OBJS := $(SRCS:.c=.o)

# Dependency files
DEPS := $(OBJS:.o=.d)

# Install directories
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
SHAREDIR := $(PREFIX)/share
ICONDIR := $(SHAREDIR)/icons/hicolor
DESKTOPDIR := $(SHAREDIR)/applications
SYSTEMDDIR := $(HOME)/.config/systemd/user

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean install uninstall deps check help

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@echo "🔗 Linking $@..."
	@$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)
	@echo "✓ Build complete: $(TARGET)"

# Compile with dependency generation
%.o: %.c
	@echo "🔨 Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependencies
-include $(DEPS)

# ============================================================================
# Installation
# ============================================================================

install: $(TARGET) install-icons install-desktop install-service
	@echo "📦 Installing binary..."
	@install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "✓ Installed to $(BINDIR)/$(TARGET)"
	@echo ""
	@echo "╔════════════════════════════════════════════════╗"
	@echo "║  Installation Complete!                        ║"
	@echo "╚════════════════════════════════════════════════╝"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Run: rootstream --qr"
	@echo "  2. Share your QR code with another device"
	@echo "  3. Start the tray app: rootstream"
	@echo ""
	@echo "Enable auto-start:"
	@echo "  systemctl --user enable rootstream.service"
	@echo "  systemctl --user start rootstream.service"
	@echo ""

install-icons:
	@echo "🎨 Installing icons..."
	@mkdir -p $(DESTDIR)$(ICONDIR)/48x48/apps
	@mkdir -p $(DESTDIR)$(ICONDIR)/scalable/apps
	@# Note: Icons should be in ui/icons/ directory
	@# For now, create placeholder
	@touch $(DESTDIR)$(ICONDIR)/48x48/apps/rootstream.png

install-desktop:
	@echo "🖥️  Installing desktop entry..."
	@mkdir -p $(DESTDIR)$(DESKTOPDIR)
	@cat > $(DESTDIR)$(DESKTOPDIR)/rootstream.desktop <<EOF
[Desktop Entry]
Name=RootStream
Comment=Secure P2P Game Streaming
Exec=rootstream
Icon=rootstream
Type=Application
Categories=Network;AudioVideo;
StartupNotify=false
Terminal=false
EOF

install-service:
	@echo "⚙️  Installing systemd service..."
	@mkdir -p $(SYSTEMDDIR)
	@cat > $(SYSTEMDDIR)/rootstream.service <<EOF
[Unit]
Description=RootStream Secure P2P Streaming
After=network.target graphical.target

[Service]
Type=simple
ExecStart=$(BINDIR)/rootstream --service
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=default.target
EOF
	@echo "✓ Service installed"
	@echo "  Enable: systemctl --user enable rootstream.service"
	@echo "  Start:  systemctl --user start rootstream.service"

# ============================================================================
# Uninstallation
# ============================================================================

uninstall:
	@echo "Uninstalling RootStream..."
	@rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@rm -f $(DESTDIR)$(DESKTOPDIR)/rootstream.desktop
	@rm -f $(SYSTEMDDIR)/rootstream.service
	@rm -f $(DESTDIR)$(ICONDIR)/*/apps/rootstream.*
	@echo "✓ Uninstalled"
	@echo ""
	@echo "Note: Configuration preserved in ~/.config/rootstream/"
	@echo "      Run 'rm -rf ~/.config/rootstream' to remove"

# ============================================================================
# Cleaning
# ============================================================================

clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -f $(OBJS) $(DEPS) $(TARGET)
	@rm -f src/*.o src/*.d
	@echo "✓ Clean complete"

# ============================================================================
# Dependency Checking
# ============================================================================

deps:
	@echo "Checking dependencies..."
	@echo ""
	@echo "Required:"
	@pkg-config --exists libdrm || { echo "  ❌ libdrm"; exit 1; }
	@echo "  ✓ libdrm"
	@pkg-config --exists libva || { echo "  ❌ libva"; exit 1; }
	@echo "  ✓ libva"
	@pkg-config --exists gtk+-3.0 || { echo "  ❌ gtk3"; exit 1; }
	@echo "  ✓ gtk3"
	@pkg-config --exists libsodium || { echo "  ❌ libsodium"; exit 1; }
	@echo "  ✓ libsodium"
	@pkg-config --exists libqrencode || { echo "  ❌ qrencode"; exit 1; }
	@echo "  ✓ qrencode"
	@pkg-config --exists libpng || { echo "  ❌ libpng"; exit 1; }
	@echo "  ✓ libpng"
	@echo ""
	@echo "Optional:"
	@pkg-config --exists avahi-client && echo "  ✓ avahi" || echo "  ⚠ avahi (auto-discovery disabled)"
	@echo ""
	@echo "✓ All required dependencies found"

# ============================================================================
# Code Quality
# ============================================================================

check:
	@echo "Running code checks..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all src/ || echo "  ⚠ cppcheck not found"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy src/*.c -- $(CFLAGS) || echo "  ⚠ clang-tidy not found"

format:
	@echo "Formatting code..."
	@command -v clang-format >/dev/null 2>&1 && clang-format -i src/*.c include/*.h || echo "  ⚠ clang-format not found"

# ============================================================================
# Help
# ============================================================================

help:
	@echo "RootStream Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all             Build RootStream (default)"
	@echo "  install         Install to system"
	@echo "  uninstall       Remove from system"
	@echo "  clean           Remove build artifacts"
	@echo "  deps            Check dependencies"
	@echo "  check           Run code analysis"
	@echo "  format          Format source code"
	@echo "  help            Show this help"
	@echo ""
	@echo "Build options:"
	@echo "  DEBUG=1         Build with debug symbols"
	@echo "  PREFIX=/path    Install prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build"
	@echo "  make DEBUG=1            # Debug build"
	@echo "  make install            # Install to /usr/local"
	@echo "  sudo make PREFIX=/usr install   # Install to /usr"

# ============================================================================
# Special targets
# ============================================================================

.DEFAULT_GOAL := all
.SUFFIXES:
.DELETE_ON_ERROR:

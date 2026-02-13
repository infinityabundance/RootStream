# RootStream - Secure P2P Game Streaming
# Professional Makefile with full dependency management

# ============================================================================
# Configuration
# ============================================================================

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-format-truncation -Wno-stringop-truncation -pedantic -std=gnu11 -O2 -D_GNU_SOURCE
CFLAGS += -I./include

# Debug flags (use: make DEBUG=1)
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
    LDFLAGS += -g
else
    CFLAGS += -DNDEBUG
endif

# Libraries
LIBS := -ldrm -lpthread -lqrencode -lpng -lm

# libsodium (required for crypto unless NO_CRYPTO=1)
SODIUM_FOUND := $(shell pkg-config --exists libsodium && echo yes)
ifdef NO_CRYPTO
    CFLAGS += -DROOTSTREAM_NO_CRYPTO
else
    ifeq ($(SODIUM_FOUND),yes)
        CFLAGS += $(shell pkg-config --cflags libsodium)
        LIBS += $(shell pkg-config --libs libsodium)
    else
        $(error libsodium development files not found. Install libsodium (e.g., libsodium-dev) or run `make deps` for checks. Alternatively set NO_CRYPTO=1 for a non-functional build.)
    endif
endif

# VA-API (optional, required for encoding)
VA_FOUND := $(shell pkg-config --exists libva && echo yes)
ifeq ($(VA_FOUND),yes)
    CFLAGS += $(shell pkg-config --cflags libva libva-drm)
    LIBS += $(shell pkg-config --libs libva libva-drm)
    CFLAGS += -DHAVE_VAAPI
endif

# GTK3 (required unless HEADLESS=1)
GTK_PKG := gtk+-3.0
GTK_FOUND := $(shell pkg-config --exists $(GTK_PKG) && echo yes)
ifdef HEADLESS
    CFLAGS += -DROOTSTREAM_HEADLESS
else
    ifeq ($(GTK_FOUND),yes)
        CFLAGS += $(shell pkg-config --cflags $(GTK_PKG))
        LIBS += $(shell pkg-config --libs $(GTK_PKG))
    else
        $(error GTK3 development files not found. Install gtk3 and pkg-config (e.g., libgtk-3-dev or gtk3-devel) or run `make deps` for checks. Alternatively set HEADLESS=1 for a non-GUI build.)
    endif
endif

# Avahi (optional)
ifeq ($(shell pkg-config --exists avahi-client && echo yes),yes)
    CFLAGS += -DHAVE_AVAHI $(shell pkg-config --cflags avahi-client)
    LIBS += $(shell pkg-config --libs avahi-client)
endif

# SDL2 (required for client display)
SDL2_FOUND := $(shell pkg-config --exists sdl2 && echo yes)
ifeq ($(SDL2_FOUND),yes)
    CFLAGS += $(shell pkg-config --cflags sdl2)
    LIBS += $(shell pkg-config --libs sdl2)
else
    $(warning SDL2 not found - client display will not work. Install libsdl2-dev or sdl2-devel)
endif

# Opus (required for audio)
OPUS_FOUND := $(shell pkg-config --exists opus && echo yes)
ifeq ($(OPUS_FOUND),yes)
    CFLAGS += $(shell pkg-config --cflags opus)
    LIBS += $(shell pkg-config --libs opus)
else
    $(warning Opus not found - audio will not work. Install libopus-dev or opus-devel)
endif

# ALSA (required for audio)
ALSA_FOUND := $(shell pkg-config --exists alsa && echo yes)
ifeq ($(ALSA_FOUND),yes)
    CFLAGS += $(shell pkg-config --cflags alsa)
    LIBS += $(shell pkg-config --libs alsa)
else
    $(warning ALSA not found - audio will not work. Install libasound2-dev or alsa-lib-devel)
endif

# X11 (optional, for X11 capture fallback)
X11_FOUND := $(shell pkg-config --exists x11 && echo yes)
ifeq ($(X11_FOUND),yes)
    CFLAGS += $(shell pkg-config --cflags x11)
    LIBS += $(shell pkg-config --libs x11)
    CFLAGS += -DHAVE_X11
else
    $(info X11 not found - X11 capture backend will be disabled)
endif

# NVENC (optional, for NVIDIA GPU encoding)
# Check both CUDA and NVENC SDK headers
CUDA_FOUND := $(shell pkg-config --exists cuda && echo yes)
NVENC_HEADER := $(shell if [ -f "/usr/include/nvEncodeAPI.h" ] || \
                        [ -f "/usr/local/cuda/include/nvEncodeAPI.h" ] || \
                        [ -f "/opt/cuda/include/nvEncodeAPI.h" ]; then echo yes; fi)

ifeq ($(CUDA_FOUND),yes)
    ifeq ($(NVENC_HEADER),yes)
        CFLAGS += $(shell pkg-config --cflags cuda) -DHAVE_NVENC
        LIBS += $(shell pkg-config --libs cuda) -ldl
        $(info NVENC support enabled)
    else
        $(info CUDA found but NVENC headers missing. Install nvidia-video-codec-sdk for NVENC support.)
    endif
else
    # Try manual detection
    ifneq ($(wildcard /usr/local/cuda/include/cuda.h),)
        ifeq ($(NVENC_HEADER),yes)
            CFLAGS += -I/usr/local/cuda/include -DHAVE_NVENC
            LIBS += -L/usr/local/cuda/lib64 -lcuda -ldl
            $(info NVENC support enabled (manual detection))
        endif
    else ifneq ($(wildcard /opt/cuda/include/cuda.h),)
        ifeq ($(NVENC_HEADER),yes)
            CFLAGS += -I/opt/cuda/include -DHAVE_NVENC
            LIBS += -L/opt/cuda/lib64 -lcuda -ldl
            $(info NVENC support enabled (manual detection))
        endif
    endif
endif

# ============================================================================
# Targets
# ============================================================================

# Binary name
TARGET := rootstream
PLAYER := tools/rstr-player

# Source files
SRCS := src/main.c \
        src/core.c \
        src/drm_capture.c \
        src/x11_capture.c \
        src/dummy_capture.c \
        src/vaapi_encoder.c \
        src/vaapi_decoder.c \
        src/nvenc_encoder.c \
        src/ffmpeg_encoder.c \
        src/raw_encoder.c \
        src/display_sdl2.c \
        src/opus_codec.c \
        src/audio_capture.c \
        src/audio_capture_pulse.c \
        src/audio_capture_dummy.c \
        src/audio_playback.c \
        src/audio_playback_pulse.c \
        src/audio_playback_dummy.c \
        src/network.c \
        src/network_tcp.c \
        src/network_reconnect.c \
        src/input.c \
        src/crypto.c \
        src/discovery.c \
        src/discovery_broadcast.c \
        src/discovery_manual.c \
        src/tray.c \
        src/service.c \
        src/qrcode.c \
        src/config.c \
        src/latency.c \
        src/recording.c \
        src/platform/platform_linux.c \
        src/packet_validate.c

ifdef HEADLESS
    SRCS := $(filter-out src/tray.c,$(SRCS))
    SRCS += src/tray_stub.c
endif

ifdef NO_CRYPTO
    SRCS := $(filter-out src/crypto.c,$(SRCS))
    SRCS := $(filter-out src/network.c,$(SRCS))
    SRCS := $(filter-out src/network_tcp.c,$(SRCS))
    SRCS := $(filter-out src/network_reconnect.c,$(SRCS))
    SRCS += src/crypto_stub.c
    SRCS += src/network_stub.c
endif

ifdef NO_QR
    SRCS := $(filter-out src/qrcode.c,$(SRCS))
    SRCS += src/qrcode_stub.c
    LIBS := $(filter-out -lqrencode -lpng,$(LIBS))
endif

ifdef NO_DRM
    SRCS := $(filter-out src/drm_capture.c,$(SRCS))
    SRCS += src/drm_capture_stub.c
    LIBS := $(filter-out -ldrm,$(LIBS))
endif

ifneq ($(VA_FOUND),yes)
    SRCS := $(filter-out src/vaapi_encoder.c,$(SRCS))
    SRCS += src/vaapi_stub.c
endif

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

# Test binaries
TEST_CRYPTO := tests/unit/test_crypto
TEST_ENCODING := tests/unit/test_encoding

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean install uninstall deps check help player test test-build test-unit test-integration test-clean

# Default target
all: $(TARGET) $(PLAYER)

# Link
$(TARGET): $(OBJS)
	@echo "🔗 Linking $@..."
	@$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)
	@echo "✓ Build complete: $(TARGET)"

# Build rstr-player tool
# Note: Needs many modules for dependencies - simplified player would be better long-term
$(PLAYER): tools/rstr-player.c src/recording.c src/vaapi_decoder.c src/display_sdl2.c src/network.c src/network_tcp.c src/network_reconnect.c src/crypto.c src/config.c src/input.c src/opus_codec.c src/audio_playback.c src/latency.c src/platform/platform_linux.c src/packet_validate.c
	@echo "🔗 Building rstr-player..."
	@$(CC) $(CFLAGS) $^ -o $(PLAYER) $(LDFLAGS) $(LIBS)
	@echo "✓ Build complete: $(PLAYER)"

# Compile with dependency generation
%.o: %.c
	@echo "🔨 Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependencies
-include $(DEPS)

# ============================================================================
# Installation
# ============================================================================

install: $(TARGET) $(PLAYER) install-icons install-desktop install-service
	@echo "📦 Installing binary..."
	@install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@install -Dm755 $(PLAYER) $(DESTDIR)$(BINDIR)/rstr-player
	@echo "✓ Installed to $(BINDIR)/$(TARGET)"
	@echo "✓ Installed to $(BINDIR)/rstr-player"
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
	@cat > $(DESTDIR)$(DESKTOPDIR)/rootstream.desktop <<-EOF
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
	@cat > $(SYSTEMDDIR)/rootstream.service <<-EOF
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

clean: test-clean
	@echo "🧹 Cleaning build artifacts..."
	@rm -f $(OBJS) $(DEPS) $(TARGET) $(PLAYER)
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
	@echo "Testing:"
	@echo "  test            Run all tests (unit + integration)"
	@echo "  test-build      Build test binaries"
	@echo "  test-unit       Run unit tests only"
	@echo "  test-integration Run integration tests only"
	@echo "  test-clean      Remove test artifacts"
	@echo ""
	@echo "Build options:"
	@echo "  DEBUG=1         Build with debug symbols"
	@echo "  PREFIX=/path    Install prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build"
	@echo "  make DEBUG=1            # Debug build"
	@echo "  make test               # Run all tests"
	@echo "  make install            # Install to /usr/local"
	@echo "  sudo make PREFIX=/usr install   # Install to /usr"

# ============================================================================
# Testing
# ============================================================================

# Build all test binaries
test-build: $(TEST_CRYPTO) $(TEST_ENCODING)

# Build and run all tests
test: test-unit test-integration

# Run unit tests
test-unit: test-build
	@echo ""
	@echo "╔════════════════════════════════════════════════╗"
	@echo "║  Running Unit Tests                            ║"
	@echo "╚════════════════════════════════════════════════╝"
	@echo ""
	@./$(TEST_CRYPTO)
	@./$(TEST_ENCODING)
	@echo ""
	@echo "✓ All unit tests passed"

# Run integration tests
test-integration: $(TARGET)
	@echo ""
	@echo "╔════════════════════════════════════════════════╗"
	@echo "║  Running Integration Tests                     ║"
	@echo "╚════════════════════════════════════════════════╝"
	@echo ""
	@./tests/integration/test_stream.sh

# Build crypto test
$(TEST_CRYPTO): tests/unit/test_crypto.c src/crypto.c src/platform/platform_linux.c
	@echo "🔨 Building crypto tests..."
	@mkdir -p tests/unit
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)
	@echo "✓ Built: $@"

# Build encoding test (standalone, doesn't need hardware)
$(TEST_ENCODING): tests/unit/test_encoding.c
	@echo "🔨 Building encoding tests..."
	@mkdir -p tests/unit
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	@echo "✓ Built: $@"

# Clean test artifacts
test-clean:
	@echo "🧹 Cleaning test artifacts..."
	@rm -f $(TEST_CRYPTO) $(TEST_ENCODING)
	@rm -f tests/unit/*.o
	@echo "✓ Test clean complete"

# ============================================================================
# Special targets
# ============================================================================

.DEFAULT_GOAL := all
.SUFFIXES:
.DELETE_ON_ERROR:

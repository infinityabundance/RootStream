#!/usr/bin/env bash
# setup-dev.sh — RootStream developer environment bootstrap
#
# Usage:
#   ./scripts/setup-dev.sh [--headless] [--check-only]
#
#   --headless    Skip GTK3 (suitable for CI and server builds)
#   --check-only  Only check deps without installing anything
#
# Supported distros:
#   Ubuntu / Debian: apt-get
#   Arch Linux:      pacman
#   Fedora / RHEL:   dnf
#
# After running this script, build with:
#   make                   # full GUI build
#   make HEADLESS=1        # headless (no GTK system tray)
#   make DEBUG=1           # debug symbols
#
# See docs/BUILD_VALIDATION.md for details.

set -euo pipefail
# Allow individual check commands inside conditionals to return non-zero
# without triggering set -e by using explicit if/else constructs.

# ── Configuration ─────────────────────────────────────────────────────────────
HEADLESS=0
CHECK_ONLY=0

for arg in "$@"; do
    case "$arg" in
        --headless)    HEADLESS=1 ;;
        --check-only)  CHECK_ONLY=1 ;;
        --help|-h)
            sed -n '/^# setup-dev/,/^$/p' "$0" | grep '^#' | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

# ── Helpers ───────────────────────────────────────────────────────────────────
PASS="✅"
FAIL="❌"
WARN="⚠️ "
INFO="ℹ️ "

check_pkg() {
    local name="$1"
    if pkg-config --exists "$name" 2>/dev/null; then
        echo "  $PASS $name"
        return 0
    else
        echo "  $FAIL $name (missing)"
        return 1
    fi
}

check_cmd() {
    local cmd="$1"
    if command -v "$cmd" >/dev/null 2>&1; then
        echo "  $PASS $cmd"
        return 0
    else
        echo "  $FAIL $cmd (missing)"
        return 1
    fi
}

# ── Distro detection ──────────────────────────────────────────────────────────
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    elif command -v apt-get >/dev/null 2>&1; then
        echo "debian"
    elif command -v pacman >/dev/null 2>&1; then
        echo "arch"
    elif command -v dnf >/dev/null 2>&1; then
        echo "fedora"
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)

# ── Dependency lists ──────────────────────────────────────────────────────────
APT_REQUIRED=(
    build-essential pkg-config
    libdrm-dev libva-dev
    libsodium-dev libopus-dev
    libasound2-dev libsdl2-dev
    libqrencode-dev libpng-dev
    libx11-dev
)

APT_OPTIONAL=(
    libavahi-client-dev
    libpipewire-0.3-dev
    libavformat-dev libavcodec-dev libavutil-dev
    libncurses-dev
    clang-format cppcheck
)

APT_GUI=(
    libgtk-3-dev
)

PACMAN_REQUIRED=(
    base-devel
    libdrm libva
    libsodium opus
    alsa-lib sdl2
    qrencode libpng
    libx11
)

PACMAN_OPTIONAL=(
    avahi
    pipewire
    ffmpeg
    ncurses
    clang cppcheck
)

PACMAN_GUI=(
    gtk3
)

DNF_REQUIRED=(
    gcc make pkgconfig
    libdrm-devel libva-devel
    libsodium-devel opus-devel
    alsa-lib-devel SDL2-devel
    qrencode-devel libpng-devel
    libX11-devel
)

DNF_OPTIONAL=(
    avahi-devel
    pipewire-devel
    ffmpeg-devel
    ncurses-devel
    clang cppcheck
)

DNF_GUI=(
    gtk3-devel
)

# ── Banner ─────────────────────────────────────────────────────────────────────
echo ""
echo "╔═══════════════════════════════════════════════════════╗"
echo "║     RootStream Developer Environment Bootstrap        ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""
echo "$INFO Detected distro: $DISTRO"
echo "$INFO Headless mode: $([ $HEADLESS -eq 1 ] && echo 'yes (no GTK)' || echo 'no (full GUI)')"
echo ""

# ── Check-only mode ───────────────────────────────────────────────────────────
if [ $CHECK_ONLY -eq 1 ]; then
    echo "── Checking required dependencies ───────────────────────────"
    MISSING=0

    check_cmd gcc    || MISSING=$((MISSING+1))
    check_cmd make   || MISSING=$((MISSING+1))
    check_pkg libdrm || MISSING=$((MISSING+1))
    check_pkg libsodium || MISSING=$((MISSING+1))
    check_pkg opus   || MISSING=$((MISSING+1))
    check_pkg alsa   || MISSING=$((MISSING+1))
    check_pkg sdl2   || MISSING=$((MISSING+1))
    check_pkg libqrencode || MISSING=$((MISSING+1))

    if [ $HEADLESS -eq 0 ]; then
        check_pkg gtk+-3.0 || MISSING=$((MISSING+1))
    fi

    echo ""
    echo "── Checking optional dependencies ───────────────────────────"
    check_pkg libva          || true
    check_pkg avahi-client   || true
    check_pkg libpipewire-0.3 || true
    check_pkg x11            || true
    check_pkg ncurses        || true
    check_cmd clang-format   || true
    check_cmd cppcheck       || true

    echo ""
    if [ $MISSING -eq 0 ]; then
        echo "$PASS All required dependencies found."
        echo ""
        echo "Build with:"
        [ $HEADLESS -eq 1 ] && echo "  make HEADLESS=1" || echo "  make"
    else
        echo "$FAIL $MISSING required dependencies missing."
        echo "Run without --check-only to install them."
        exit 1
    fi
    exit 0
fi

# ── Install mode ──────────────────────────────────────────────────────────────
install_deps() {
    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop)
            local pkgs=("${APT_REQUIRED[@]}" "${APT_OPTIONAL[@]}")
            [ $HEADLESS -eq 0 ] && pkgs+=("${APT_GUI[@]}")
            echo "$INFO Installing via apt-get..."
            sudo apt-get update -qq
            sudo apt-get install -y "${pkgs[@]}" || {
                echo "$WARN Some optional packages may not be available; continuing..."
                sudo apt-get install -y "${APT_REQUIRED[@]}"
                [ $HEADLESS -eq 0 ] && sudo apt-get install -y "${APT_GUI[@]}" || true
            }
            ;;
        arch|manjaro|endeavouros)
            local pkgs=("${PACMAN_REQUIRED[@]}" "${PACMAN_OPTIONAL[@]}")
            [ $HEADLESS -eq 0 ] && pkgs+=("${PACMAN_GUI[@]}")
            echo "$INFO Installing via pacman..."
            sudo pacman -Sy --noconfirm "${pkgs[@]}" || {
                echo "$WARN Some optional packages may not be available; continuing..."
                sudo pacman -Sy --noconfirm "${PACMAN_REQUIRED[@]}"
                [ $HEADLESS -eq 0 ] && sudo pacman -Sy --noconfirm "${PACMAN_GUI[@]}" || true
            }
            ;;
        fedora|rhel|centos|rocky|almalinux)
            local pkgs=("${DNF_REQUIRED[@]}" "${DNF_OPTIONAL[@]}")
            [ $HEADLESS -eq 0 ] && pkgs+=("${DNF_GUI[@]}")
            echo "$INFO Installing via dnf..."
            sudo dnf install -y "${pkgs[@]}" || {
                echo "$WARN Some optional packages may not be available; continuing..."
                sudo dnf install -y "${DNF_REQUIRED[@]}"
                [ $HEADLESS -eq 0 ] && sudo dnf install -y "${DNF_GUI[@]}" || true
            }
            ;;
        *)
            echo "$WARN Unknown distro. Please install the following manually:"
            echo ""
            echo "Required: ${APT_REQUIRED[*]}"
            [ $HEADLESS -eq 0 ] && echo "GUI:      ${APT_GUI[*]}"
            echo "Optional: ${APT_OPTIONAL[*]}"
            echo ""
            echo "See docs/BUILD_VALIDATION.md for the full dependency list."
            exit 1
            ;;
    esac
}

install_deps

# ── Runtime setup ─────────────────────────────────────────────────────────────
echo ""
echo "── Runtime permissions ─────────────────────────────────────────────"

if ! groups | grep -qw video; then
    echo "$INFO Adding $USER to the 'video' group (for DRM/KMS access)..."
    sudo usermod -aG video "$USER"
    echo "$WARN You must log out and back in for the group change to take effect."
else
    echo "$PASS $USER is already in the 'video' group."
fi

if ! lsmod | grep -q uinput; then
    echo "$INFO Loading uinput kernel module..."
    sudo modprobe uinput || echo "$WARN Failed to load uinput; input injection may not work."
fi

if [ ! -f /etc/modules-load.d/rootstream.conf ]; then
    echo "$INFO Persisting uinput module load..."
    echo "uinput" | sudo tee /etc/modules-load.d/rootstream.conf >/dev/null
fi

# ── Verify and build ──────────────────────────────────────────────────────────
echo ""
echo "── Verification ─────────────────────────────────────────────────────"

MISSING=0
check_cmd gcc    || MISSING=$((MISSING+1))
check_pkg libdrm || MISSING=$((MISSING+1))
check_pkg libsodium || MISSING=$((MISSING+1))

if [ $MISSING -gt 0 ]; then
    echo ""
    echo "$FAIL $MISSING required dependencies are still missing."
    echo "Please resolve them manually before building."
    exit 1
fi

echo ""
echo "$PASS Setup complete."
echo ""
echo "── Next steps ───────────────────────────────────────────────────────"
echo ""
if [ $HEADLESS -eq 1 ]; then
    echo "  make HEADLESS=1          # build without GUI"
else
    echo "  make                     # full build"
fi
echo "  make test-build          # build unit tests"
echo "  ./tests/unit/test_crypto # run crypto tests"
echo "  ./rootstream --help      # verify binary"
echo ""
echo "  See docs/CORE_PATH.md for the full canonical workflow."
echo ""

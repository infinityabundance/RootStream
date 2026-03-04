#!/usr/bin/env bash
# build_appimage.sh — Build a portable AppImage for RootStream
#
# Prerequisites:
#   - appimagetool  (downloaded automatically if not in PATH)
#   - linuxdeploy   (downloaded automatically if not in PATH)
#   - A built rootstream binary (run `make` first)
#
# Usage: ./packaging/build_appimage.sh [--output-dir DIR]

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${REPO_ROOT}/dist"
APPDIR="${REPO_ROOT}/build/AppDir"
ARCH="${ARCH:-x86_64}"

for arg in "$@"; do
    case "${arg}" in
        --output-dir) shift; OUTPUT_DIR="$1" ;;
        --output-dir=*) OUTPUT_DIR="${arg#*=}" ;;
        *) echo "Unknown argument: ${arg}"; exit 1 ;;
    esac
done

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# ---------------------------------------------------------------------------
# Ensure required tools are available
# ---------------------------------------------------------------------------
ensure_tool() {
    local name="$1" url="$2"
    if ! command -v "${name}" &>/dev/null; then
        echo -e "${YELLOW}==> Downloading ${name}...${NC}"
        local dest="/usr/local/bin/${name}"
        curl -fsSL "${url}" -o "${dest}"
        chmod +x "${dest}"
    fi
}

ensure_tool appimagetool \
    "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage"

ensure_tool linuxdeploy \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"

# ---------------------------------------------------------------------------
# Verify binary exists
# ---------------------------------------------------------------------------
BINARY="${REPO_ROOT}/rootstream"
if [[ ! -f "${BINARY}" ]]; then
    echo -e "${RED}ERROR: Binary not found at ${BINARY}. Run 'make' first.${NC}"
    exit 1
fi

# ---------------------------------------------------------------------------
# Populate AppDir
# ---------------------------------------------------------------------------
echo -e "${YELLOW}==> Populating AppDir...${NC}"
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin" "${APPDIR}/usr/share/applications" \
         "${APPDIR}/usr/share/icons/hicolor/256x256/apps"

cp "${BINARY}" "${APPDIR}/usr/bin/rootstream"

# Desktop entry
cat >"${APPDIR}/usr/share/applications/rootstream.desktop" <<'EOF'
[Desktop Entry]
Name=RootStream
Comment=Native Linux game streaming
Exec=rootstream
Icon=rootstream
Type=Application
Categories=Network;Game;
EOF

# Icon (use the repo asset if present, otherwise create a placeholder)
if [[ -f "${REPO_ROOT}/assets/rootstream.png" ]]; then
    cp "${REPO_ROOT}/assets/rootstream.png" \
       "${APPDIR}/usr/share/icons/hicolor/256x256/apps/rootstream.png"
else
    # Minimal 1×1 transparent PNG as placeholder
    printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82' \
        >"${APPDIR}/usr/share/icons/hicolor/256x256/apps/rootstream.png"
fi

# Symlink for AppImage spec compliance
ln -sf usr/share/applications/rootstream.desktop "${APPDIR}/rootstream.desktop"
ln -sf usr/share/icons/hicolor/256x256/apps/rootstream.png "${APPDIR}/rootstream.png"

# AppRun entry-point
cat >"${APPDIR}/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "$0")")"
exec "${HERE}/usr/bin/rootstream" "$@"
EOF
chmod +x "${APPDIR}/AppRun"

# ---------------------------------------------------------------------------
# Bundle shared libraries with linuxdeploy
# ---------------------------------------------------------------------------
echo -e "${YELLOW}==> Bundling shared libraries...${NC}"
linuxdeploy --appdir "${APPDIR}" --executable "${APPDIR}/usr/bin/rootstream" || true

# ---------------------------------------------------------------------------
# Build the AppImage
# ---------------------------------------------------------------------------
mkdir -p "${OUTPUT_DIR}"
OUTPUT_FILE="${OUTPUT_DIR}/RootStream-${ARCH}.AppImage"

echo -e "${YELLOW}==> Building AppImage → ${OUTPUT_FILE}...${NC}"
ARCH="${ARCH}" appimagetool "${APPDIR}" "${OUTPUT_FILE}"

echo -e "${GREEN}DONE: ${OUTPUT_FILE}${NC}"

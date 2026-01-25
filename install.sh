#!/bin/bash
# RootStream - Installation and Setup Script

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════╗"
echo "║         RootStream Installation               ║"
echo "║  Native Linux Game Streaming                  ║"
echo "╚═══════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if running on Arch-based system
if [ -f /etc/arch-release ]; then
    echo -e "${GREEN}✓ Arch Linux detected${NC}"
    DISTRO="arch"
elif [ -f /etc/debian_version ]; then
    echo -e "${YELLOW}⚠ Debian/Ubuntu detected (untested)${NC}"
    DISTRO="debian"
elif [ -f /etc/redhat-release ]; then
    echo -e "${YELLOW}⚠ Fedora/RHEL detected (untested)${NC}"
    DISTRO="fedora"
else
    echo -e "${YELLOW}⚠ Unknown distribution${NC}"
    DISTRO="unknown"
fi

# Function to install dependencies
install_deps() {
    echo -e "\n${BLUE}📦 Installing dependencies...${NC}"
    
    case $DISTRO in
        arch)
            sudo pacman -S --needed base-devel libdrm libva mesa
            ;;
        debian)
            sudo apt-get update
            sudo apt-get install -y build-essential libdrm-dev libva-dev mesa-va-drivers
            ;;
        fedora)
            sudo dnf install -y gcc make libdrm-devel libva-devel mesa-va-drivers
            ;;
        *)
            echo -e "${RED}✗ Unknown distro - please install: gcc, make, libdrm, libva${NC}"
            exit 1
            ;;
    esac
    
    echo -e "${GREEN}✓ Dependencies installed${NC}"
}

# Check for dependencies
check_deps() {
    echo -e "\n${BLUE}🔍 Checking dependencies...${NC}"
    
    local missing=()
    
    if ! command -v gcc &> /dev/null; then
        missing+=("gcc")
    fi
    
    if ! command -v make &> /dev/null; then
        missing+=("make")
    fi
    
    if ! pkg-config --exists libdrm 2>/dev/null; then
        missing+=("libdrm")
    fi
    
    if ! pkg-config --exists libva 2>/dev/null; then
        missing+=("libva")
    fi
    
    if [ ${#missing[@]} -eq 0 ]; then
        echo -e "${GREEN}✓ All dependencies found${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠ Missing dependencies: ${missing[*]}${NC}"
        read -p "Install missing dependencies? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_deps
        else
            echo -e "${RED}✗ Cannot continue without dependencies${NC}"
            exit 1
        fi
    fi
}

# Build the project
build() {
    echo -e "\n${BLUE}🔨 Building RootStream...${NC}"
    
    if [ ! -f Makefile ]; then
        echo -e "${RED}✗ Makefile not found - are you in the right directory?${NC}"
        exit 1
    fi
    
    make clean 2>/dev/null || true
    
    if make; then
        echo -e "${GREEN}✓ Build successful${NC}"
    else
        echo -e "${RED}✗ Build failed${NC}"
        exit 1
    fi
}

# Install the binary
install_binary() {
    echo -e "\n${BLUE}📦 Installing RootStream...${NC}"
    
    if [ ! -f rootstream ]; then
        echo -e "${RED}✗ Binary not found - build first${NC}"
        exit 1
    fi
    
    sudo install -m 755 rootstream /usr/local/bin/
    echo -e "${GREEN}✓ Installed to /usr/local/bin/rootstream${NC}"
}

# Setup permissions
setup_permissions() {
    echo -e "\n${BLUE}🔐 Setting up permissions...${NC}"
    
    # Add user to video group
    if ! groups $USER | grep -q '\bvideo\b'; then
        sudo usermod -a -G video $USER
        echo -e "${YELLOW}⚠ Added to 'video' group - you must log out and back in${NC}"
    else
        echo -e "${GREEN}✓ Already in 'video' group${NC}"
    fi
    
    # Check uinput permissions
    if [ ! -c /dev/uinput ]; then
        echo -e "${YELLOW}⚠ /dev/uinput not found${NC}"
        echo "  Loading uinput module..."
        sudo modprobe uinput
        echo "uinput" | sudo tee /etc/modules-load.d/rootstream.conf > /dev/null
    fi
    
    # Check for DRM devices
    if [ -e /dev/dri/card0 ]; then
        echo -e "${GREEN}✓ DRM device found: /dev/dri/card0${NC}"
    else
        echo -e "${RED}✗ No DRM device found${NC}"
        echo "  This is probably a problem."
    fi
}

# Test VA-API
test_vaapi() {
    echo -e "\n${BLUE}🧪 Testing VA-API...${NC}"
    
    if command -v vainfo &> /dev/null; then
        if vainfo &> /dev/null; then
            echo -e "${GREEN}✓ VA-API working${NC}"
        else
            echo -e "${YELLOW}⚠ VA-API not working - encoder may fail${NC}"
            echo "  For NVIDIA, try: sudo pacman -S libva-vdpau-driver"
        fi
    else
        echo -e "${YELLOW}⚠ vainfo not found - install libva-utils to test${NC}"
    fi
}

# Main installation
main() {
    check_deps
    build
    
    read -p "Install to /usr/local/bin? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        install_binary
        setup_permissions
        test_vaapi
        
        echo -e "\n${GREEN}╔═══════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║  🎉 Installation Complete!                    ║${NC}"
        echo -e "${GREEN}╚═══════════════════════════════════════════════╝${NC}"
        
        echo -e "\nQuick start:"
        echo -e "  ${BLUE}rootstream host${NC}           # Start streaming"
        echo -e "  ${BLUE}rootstream client HOST${NC}    # Connect to host"
        
        if ! groups $USER | grep -q '\bvideo\b'; then
            echo -e "\n${YELLOW}⚠ IMPORTANT: Log out and back in for permissions to take effect${NC}"
        fi
    else
        echo -e "\n${YELLOW}Build complete. To install manually:${NC}"
        echo "  sudo make install"
    fi
}

# Run
main

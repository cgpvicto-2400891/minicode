#!/usr/bin/env bash
# ============================================================
# MiniCode — Installation script for Ubuntu/Debian
# ============================================================
set -e

BLUE='\033[0;34m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   MiniCode — Installer               ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- Detect package manager ----
if command -v apt-get &>/dev/null; then
    PKG="apt-get"
elif command -v dnf &>/dev/null; then
    PKG="dnf"
elif command -v pacman &>/dev/null; then
    PKG="pacman"
else
    echo "Unsupported package manager. Install Qt6 manually."
    PKG=""
fi

# ---- Install dependencies ----
if [ -n "$PKG" ]; then
    echo -e "\n${YELLOW}Installing dependencies...${NC}"
    if [ "$PKG" = "apt-get" ]; then
        sudo apt-get update -q
        sudo apt-get install -y \
            cmake build-essential \
            qt6-base-dev qt6-base-private-dev \
            clangd || \
        sudo apt-get install -y \
            cmake build-essential \
            qtbase5-dev qtbase5-private-dev \
            clangd
    elif [ "$PKG" = "dnf" ]; then
        sudo dnf install -y cmake gcc-c++ qt6-qtbase-devel clang-tools-extra
    elif [ "$PKG" = "pacman" ]; then
        sudo pacman -S --noconfirm cmake gcc qt6-base clang
    fi
fi

# ---- Always wipe build dir to avoid stale CMakeCache ----
echo -e "\n${YELLOW}Cleaning build directory...${NC}"
rm -rf "$SCRIPT_DIR/build"
mkdir -p "$SCRIPT_DIR/build"
cd "$SCRIPT_DIR/build"

# ---- Configure — always Unix Makefiles (no Ninja dependency) ----
echo -e "\n${YELLOW}Configuring...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# ---- Build ----
echo -e "\n${YELLOW}Building MiniCode...${NC}"
make -j"$(nproc)"

echo -e "\n${GREEN}✓ Build successful!${NC}"
echo -e "\n${YELLOW}Run with:${NC}"
echo -e "  cd ${SCRIPT_DIR}/build && ./MiniCode"
echo -e "  ./MiniCode myfile.cpp"
echo ""

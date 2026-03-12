#!/bin/bash
# install.sh - Installs Syncpad for macOS and Linux
# Usage: curl -sL https://raw.githubusercontent.com/Antares699/syncpad-app/main/install.sh | bash

set -e

# ===== CONFIGURATION =====
REPO="Antares699/syncpad-app"

# Detect OS (lowercase)
OS=$(uname -s | tr '[:upper:]' '[:lower:]')

# ===== PRE-FLIGHT CHECKS =====

# Check if Git is installed
if ! command -v git &> /dev/null; then
    echo "ERROR: Git is not installed. Syncpad requires Git for synchronization."
    echo ""
    echo "Install Git for your platform:"
    echo ""
    
    # Detect OS and provide specific install command
    if [ "$OS" = "darwin" ]; then
        echo "  macOS:"
        echo "    brew install git"
        echo "    or download from: https://git-scm.com/download/mac"
    else
        echo "  Ubuntu/Debian:"
        echo "    sudo apt-get update && sudo apt-get install git"
        echo ""
        echo "  Fedora:"
        echo "    sudo dnf install git"
        echo ""
        echo "  Arch Linux:"
        echo "    sudo pacman -S git"
        echo ""
        echo "  Other Linux: https://git-scm.com/download/linux"
    fi
    echo ""
    exit 1
fi

echo "✓ Git detected"

# Check if SSH keys exist (warning only)
if [ ! -f "$HOME/.ssh/id_rsa" ] && [ ! -f "$HOME/.ssh/id_ed25519" ]; then
    echo ""
    echo "WARNING: No SSH keys detected in ~/.ssh/"
    echo ""
    echo "Syncpad requires GitHub authentication. You have two options:"
    echo "  1. Set up SSH keys (see https://github.com/Antares699/syncpad-app/blob/main/SETUP.md)"
    echo "  2. Install and authenticate with GitHub CLI: gh auth login"
    echo ""
    read -p "Continue installation anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Construct the download URL
if [ "$OS" = "darwin" ]; then
    BINARY_URL="https://github.com/${REPO}/releases/latest/download/syncpad-macos"
else
    BINARY_URL="https://github.com/${REPO}/releases/latest/download/syncpad-linux"
fi

# ===== INSTALLATION =====
echo ""
echo "Downloading Syncpad for ${OS}..."

# Download the binary to a temporary location
curl -sL "$BINARY_URL" -o /tmp/syncpad

echo "Making executable..."
chmod +x /tmp/syncpad

# Try /usr/local/bin first (works for all shells)
if [ -w /usr/local/bin ] || [ -w /usr/local/bin/ ]; then
    mv /tmp/syncpad /usr/local/bin/syncpad
    echo "✓ Installed to /usr/local/bin/syncpad"
else
    # Fallback to ~/.local/bin
    mkdir -p "$HOME/.local/bin"
    mv /tmp/syncpad "$HOME/.local/bin/syncpad"
    
    # Add to PATH for current session and shell config
    export PATH="$HOME/.local/bin:$PATH"
    
    # Add to shell config (works for bash, zsh, fish, etc.)
    if [ -n "$ZSH_VERSION" ]; then
        if ! grep -q "\.local/bin" "$HOME/.zshrc" 2>/dev/null; then
            echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zshrc"
        fi
    elif [ -n "$BASH_VERSION" ]; then
        if ! grep -q "\.local/bin" "$HOME/.bashrc" 2>/dev/null; then
            echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        fi
    else
        # Generic fallback
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.profile"
    fi
    
    echo "✓ Installed to ~/.local/bin/syncpad"
    echo "  (Added to PATH - restart terminal if needed)"
fi

echo ""
echo "✓ Syncpad installed successfully!"
echo ""
echo "Run 'syncpad' in your terminal to get started."

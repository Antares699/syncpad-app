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

# Try to install to /usr/local/bin using sudo if needed
if sudo -v 2>/dev/null && command -v sudo >/dev/null 2>&1; then
    # If sudo works, move to /usr/local/bin with sudo
    sudo mv /tmp/syncpad /usr/local/bin/syncpad
    sudo chmod +x /usr/local/bin/syncpad
    echo "✓ Installed to /usr/local/bin/syncpad (with sudo)"
else
    # Fallback to ~/.local/bin
    mkdir -p "$HOME/.local/bin"
    mv /tmp/syncpad "$HOME/.local/bin/syncpad"
    
    # Add to PATH for current session
    export PATH="$HOME/.local/bin:$PATH"
    
    # Determine which shell config file to use based on the currently running shell
    if [ -n "$ZSH_VERSION" ]; then
        # Zsh is running
        if ! grep -q "$HOME/.local/bin" "$HOME/.zshrc" 2>/dev/null; then
            echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zshrc"
            echo "Added to ~/.zshrc"
        fi
    elif [ -n "$BASH_VERSION" ]; then
        # Bash is running
        if ! grep -q "$HOME/.local/bin" "$HOME/.bashrc" 2>/dev/null; then
            echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
            echo "Added to ~/.bashrc"
        fi
    else
        # Unknown shell - try common shell configs based on $SHELL
        if [[ "$SHELL" == *"/zsh"* ]]; then
            if ! grep -q "$HOME/.local/bin" "$HOME/.zshrc" 2>/dev/null; then
                echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zshrc"
                echo "Added to ~/.zshrc"
            fi
        elif [[ "$SHELL" == *"/bash"* ]]; then
            if ! grep -q "$HOME/.local/bin" "$HOME/.bashrc" 2>/dev/null; then
                echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
                echo "Added to ~/.bashrc"
            fi
        else
            # Last fallback
            if ! grep -q "$HOME/.local/bin" "$HOME/.profile" 2>/dev/null; then
                echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.profile"
                echo "Added to ~/.profile"
            fi
        fi
    fi
    
    echo "✓ Installed to ~/.local/bin/syncpad"
    echo "  (Added to PATH - restart terminal or run 'source ~/.zshrc' or 'source ~/.bashrc')"
fi

echo ""
echo "✓ Syncpad installed successfully!"
echo ""
echo "Run 'syncpad' in your terminal to get started."

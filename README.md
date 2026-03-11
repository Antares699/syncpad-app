# Syncpad

**Syncpad** is a lightweight, terminal-based notepad written in pure C that uses Git as a silent backend database to synchronize notes across multiple computers. It provides a distraction-free editing experience with automatic cloud synchronization to GitHub.

## Overview

Syncpad is designed for developers who want a simple, fast note-taking solution that seamlessly syncs across devices without the overhead of complex cloud services or heavy Electron-based applications. The entire application is a single, statically-compiled binary with minimal dependencies.

### Key Features

- **Pure C Implementation**: Written entirely in C using only standard libraries, ncurses/PDCurses for the UI, and libgit2 for Git operations
- **Minimal Footprint**: Single binary executable, typically under 100KB
- **Cross-Platform**: Runs natively on macOS, Linux, and Windows without modification
- **Silent Synchronization**: Automatically commits and pushes changes to your private GitHub repository in the background
- **Fast and Responsive**: Local file operations happen instantly; Git operations run asynchronously without blocking the UI

## Prerequisites

Before installing Syncpad, ensure you have:

- **Git**: Required for synchronization operations
- **GitHub Authentication**: Either SSH keys OR GitHub CLI

### Installing Git

If you don't have Git installed, the installer will detect this and provide platform-specific installation instructions.

### Authentication Setup

Syncpad requires GitHub authentication. Choose one of the following methods:

**Option 1: GitHub CLI (Recommended for beginners)**

First, check if you already have GitHub CLI installed and authenticated:

```bash
gh auth status
```

If you see "Logged in to github.com", you're all set and can skip to the Installation section.

If not installed or not authenticated, install the GitHub CLI for your platform:

**macOS:**
```bash
brew install gh
```

**Linux:**
See installation instructions at: https://github.com/cli/cli#installation

**Windows:**
```powershell
winget install GitHub.CLI
```

Then authenticate:
```bash
gh auth login
```

**Option 2: SSH Keys (For advanced users)**

See the [SSH Setup Guide](SETUP.md) for detailed instructions on generating and configuring SSH keys for GitHub.

## Installation

Pre-compiled binaries are available for all major platforms. 

### macOS and Linux

```bash
curl -sL https://raw.githubusercontent.com/Antares699/syncpad-app/main/install.sh | bash
```

### Windows

```powershell
powershell -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/Antares699/syncpad-app/main/install.ps1 | iex"
```

## Usage

Launch Syncpad from your terminal:

```bash
syncpad
```

### First Run

On first launch, Syncpad will detect that the `~/.syncpad` directory does not exist and offer to automatically create a private GitHub repository called `syncpad-notes`. If you have the GitHub CLI (`gh`) installed and authenticated, it will handle this automatically. Otherwise, you can provide an SSH repository URL manually.

### Keyboard Shortcuts

- `Ctrl+S`: Save the current buffer to disk and synchronize to GitHub
- `Ctrl+X`: Exit the application

## Building from Source

If you want to build Syncpad locally or contribute to development, you will need:

- A C compiler (GCC or Clang)
- ncurses development libraries (or PDCurses on Windows)
- libgit2 development libraries
- pkg-config

### Build Instructions

```bash
git clone https://github.com/Antares699/syncpad-app.git
cd syncpad-app
make
```

The Makefile automatically detects your platform and links the appropriate libraries.

### Manual Compilation

**Linux:**
```bash
gcc main.c -lncurses -ltinfo $(pkg-config --libs --cflags libgit2) -o syncpad
```

**macOS:**
```bash
gcc main.c -lncurses $(pkg-config --libs --cflags libgit2) -o syncpad
```

**Windows (MSYS2):**
```bash
gcc main.c -lpdcurses $(pkg-config --libs --cflags libgit2) -o syncpad.exe
```

## Technical Details

- **Language**: Pure C (C99 standard)
- **UI Library**: ncurses (Unix) / PDCurses (Windows)
- **Git Integration**: libgit2 for local operations, system Git for network operations
- **Authentication**: Uses your existing SSH keys and ssh-agent
- **Storage**: All notes are stored in `~/.syncpad/notes.txt` and versioned in a local Git repository

## Requirements

- SSH keys configured for GitHub (for automatic synchronization)
- Git installed on your system (for push operations)
- Terminal emulator with ncurses support

## License

This project is open source and available for educational and personal use.

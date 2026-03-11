# install.ps1 - Installs Syncpad for Windows
# Usage: powershell -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/Antares699/syncpad-app/main/install.ps1 | iex"

# ===== CONFIGURATION =====
$Repo = "Antares699/syncpad-app"

# ===== MAIN =====
$ErrorActionPreference = "Stop"

Write-Host "Syncpad Installer for Windows" -ForegroundColor Cyan
Write-Host ""

# ===== PRE-FLIGHT CHECKS =====

# Check if Git is installed
Write-Host "Checking prerequisites..." -ForegroundColor Cyan
try {
    $null = git --version
    Write-Host "✓ Git detected" -ForegroundColor Green
} catch {
    Write-Host ""
    Write-Host "ERROR: Git is not installed. Syncpad requires Git for synchronization." -ForegroundColor Red
    Write-Host ""
    Write-Host "Install Git for Windows:" -ForegroundColor Yellow
    Write-Host "  Download from: https://git-scm.com/download/win"
    Write-Host "  or use winget: winget install Git.Git"
    Write-Host ""
    exit 1
}

# Check if SSH keys exist (warning only)
$sshDir = "$env:USERPROFILE\.ssh"
$hasSSHKeys = (Test-Path "$sshDir\id_rsa") -or (Test-Path "$sshDir\id_ed25519")

if (-not $hasSSHKeys) {
    Write-Host ""
    Write-Host "WARNING: No SSH keys detected in $sshDir" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Syncpad requires GitHub authentication. You have two options:" -ForegroundColor Yellow
    Write-Host "  1. Set up SSH keys (see https://github.com/Antares699/syncpad-app/blob/main/SETUP.md)" -ForegroundColor Yellow
    Write-Host "  2. Install and authenticate with GitHub CLI: gh auth login" -ForegroundColor Yellow
    Write-Host ""
    $response = Read-Host "Continue installation anyway? (y/n)"
    if ($response -notmatch '^[Yy]$') {
        exit 1
    }
}

# Build the download URL
$BinaryUrl = "https://github.com/$Repo/releases/latest/download/syncpad-windows.exe"

# Create installation directory
$InstallDir = "$env:USERPROFILE\.syncpad_bin"
$ExePath = "$InstallDir\syncpad.exe"

if (!(Test-Path -Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

# ===== INSTALLATION =====
Write-Host ""
Write-Host "Downloading Syncpad for Windows..." -ForegroundColor Cyan

# Download the binary
try {
    Invoke-WebRequest -Uri $BinaryUrl -OutFile $ExePath -UseBasicParsing
} catch {
    Write-Host "ERROR: Failed to download Syncpad." -ForegroundColor Red
    Write-Host "Make sure a release exists at: $BinaryUrl" -ForegroundColor Yellow
    exit 1
}

Write-Host "✓ Downloaded to $ExePath" -ForegroundColor Green

# ===== PATH SETUP =====
Write-Host "Adding Syncpad to User PATH..." -ForegroundColor Cyan

$UserPath = [Environment]::GetEnvironmentVariable("PATH", "User")

if ($UserPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable("PATH", "$UserPath;$InstallDir", "User")
    Write-Host "✓ PATH updated! Please restart your terminal to use the 'syncpad' command." -ForegroundColor Yellow
} else {
    Write-Host "✓ Syncpad is already in your PATH." -ForegroundColor Green
}

Write-Host ""
Write-Host "✓ Syncpad installed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Run 'syncpad' in your terminal to get started." -ForegroundColor Cyan

# SSH Setup Guide for Syncpad

This guide walks you through setting up SSH keys for GitHub authentication. SSH keys allow Syncpad to securely push your notes to your private GitHub repository without requiring a password each time.

## Step 1: Check for Existing SSH Keys

First, check if you already have SSH keys configured:

```bash
ls -la ~/.ssh
```

Look for files named `id_rsa.pub`, `id_ed25519.pub`, or similar. If you see these files, you already have SSH keys and can skip to Step 4.

## Step 2: Generate a New SSH Key

If you don't have SSH keys, generate a new one using the Ed25519 algorithm (recommended):

```bash
ssh-keygen -t ed25519 -C "your_email@example.com"
```

Replace `your_email@example.com` with your actual GitHub email address.

When prompted:
- Press Enter to accept the default file location (`~/.ssh/id_ed25519`)
- Enter a passphrase for added security (optional but recommended)
- Confirm the passphrase

**Note for older systems:** If your system doesn't support Ed25519, use RSA instead:

```bash
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
```

## Step 3: Add Your SSH Key to the SSH Agent

Start the SSH agent in the background:

```bash
eval "$(ssh-agent -s)"
```

Add your SSH private key to the agent:

```bash
ssh-add ~/.ssh/id_ed25519
```

If you used RSA, replace `id_ed25519` with `id_rsa`.

**macOS users:** If you're on macOS Sierra 10.12.2 or later, you may need to modify your `~/.ssh/config` file to automatically load keys:

```bash
cat << 'CONFIG' >> ~/.ssh/config
Host github.com
  AddKeysToAgent yes
  UseKeychain yes
  IdentityFile ~/.ssh/id_ed25519
CONFIG
```

## Step 4: Add Your SSH Key to GitHub

Copy your public key to the clipboard:

**Linux:**
```bash
cat ~/.ssh/id_ed25519.pub
```

**macOS:**
```bash
pbcopy < ~/.ssh/id_ed25519.pub
```

**Windows (Git Bash):**
```bash
cat ~/.ssh/id_ed25519.pub | clip
```

Then:

1. Go to GitHub: https://github.com/settings/keys
2. Click "New SSH key"
3. Give it a descriptive title (e.g., "My Laptop - Syncpad")
4. Paste your public key into the "Key" field
5. Click "Add SSH key"

## Step 5: Test Your SSH Connection

Verify that your SSH key is working correctly:

```bash
ssh -T git@github.com
```

You should see a message like:

```
Hi username! You've successfully authenticated, but GitHub does not provide shell access.
```

If you see this message, your SSH setup is complete and Syncpad will work correctly.

## Platform-Specific Notes

### Windows Users

If you're using Windows, ensure you're running these commands in Git Bash (installed with Git for Windows) or WSL (Windows Subsystem for Linux), not in PowerShell or Command Prompt.

### SSH Agent on Windows

On Windows, the SSH agent may not start automatically. You can configure it to start on boot:

1. Open Services (Win + R, type `services.msc`)
2. Find "OpenSSH Authentication Agent"
3. Right-click and select "Properties"
4. Set "Startup type" to "Automatic"
5. Click "Start" to start the service now

## Next Steps

Once your SSH keys are configured, you can install and run Syncpad. The first time you run it, it will automatically create a private repository on your GitHub account and begin syncing your notes.

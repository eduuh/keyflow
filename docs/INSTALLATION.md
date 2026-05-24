# Installation Guide

Complete installation instructions for keyflow.

---

## Quick Install

```bash
# 1. Install Interception driver (one-time)
# Download: https://github.com/oblitum/Interception
# Reboot after install

# 2. Create config.json
cp examples\simple.json config.json

# 3. Run as administrator
.\keyflow.exe
```

---

## Prerequisites

- **Windows 10/11** (64-bit)
- **Administrator privileges** (required)
- **Interception driver** (install once)

---

## Step 1: Install Interception Driver

1. **Download:** https://github.com/oblitum/Interception/releases
2. **Run installer as Administrator**
3. **Reboot required** ⚠️

### Verify Installation

```bash
.\keyflow.exe
# Should see: "[HardwareIO] Initialized successfully"
```

---

## Step 2: Get keyflow

### Option A: Download Binary

```bash
# Download latest release
https://github.com/yourusername/keyflow/releases/latest

# Extract to: C:\Tools\keyflow\
```

### Option B: Build from Source

```bash
git clone https://github.com/yourusername/keyflow.git
cd keyflow
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

---

## Step 3: Configure

```bash
# Copy example
copy examples\simple.json config.json

# Or create minimal config
{
  "version": "1.0",
  "remapping": {
    "CapsLock": "LeftCtrl"
  }
}
```

---

## Step 4: Run

```bash
# Right-click keyflow.exe → Run as administrator
.\keyflow.exe

# Test: Press CapsLock (should act as Ctrl)
# F12 = Toggle on/off
# F11 = Toggle logging
# Ctrl+C = Exit
```

---

## Optional: Startup Shortcut

1. Right-click `keyflow.exe` → Create shortcut
2. Properties → Advanced → ✓ Run as administrator
3. Move shortcut to: `Win+R → shell:startup`

---

## Troubleshooting

### Driver Not Found
- Reboot after driver installation
- Run keyflow as administrator

### Access Denied
- Must run as administrator
- Right-click → "Run as administrator"

### Keys Not Remapping
- Press F12 to toggle on/off
- Press F11 to enable logging
- Check config.json is valid

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more.

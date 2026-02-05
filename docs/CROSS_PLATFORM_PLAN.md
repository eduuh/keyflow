# Cross-Platform Implementation Plan for Keyflow

## Executive Summary

Transform Keyflow from Windows-only to cross-platform (Windows, Linux, macOS) like KANATA, while preserving all existing functionality and maintaining the ~2500 lines of platform-agnostic core logic.

**Timeline:** 12 weeks (3 months)
**Target Release:** v3.0.0
**Estimated Effort:** 350-600 lines per platform + infrastructure

---

## Table of Contents

1. [Investigation Results](#investigation-results)
2. [Architecture Strategy](#architecture-strategy)
3. [Implementation Phases](#implementation-phases)
4. [Platform-Specific Details](#platform-specific-technical-details)
5. [Timeline & Success Criteria](#timeline)

---

## Investigation Results

### Windows Dependencies Analysis

Comprehensive analysis of Keyflow's Windows-specific code revealed **878 lines** requiring platform abstraction across 4 components:

#### 1. Hardware I/O Layer (91 lines)
**File:** `src/hardware/HardwareIO.cpp`

**Interception Driver Integration:**
- Uses closed-source `interception.dll` (11KB) and `interception.lib` (5KB)
- Driver API calls in lines 14-87:
  - `interception_create_context()` - Initialize kernel driver
  - `interception_wait_with_timeout()` - Block until keyboard event (10ms timeout)
  - `interception_receive()` - Read `InterceptionKeyStroke` structure
  - `interception_send()` - Inject modified keystroke back to Windows
  - `interception_set_filter()` - Configure which events to intercept

**Event Data Structures:**
```c
// InterceptionKeyStroke (lines 138-143 in interception.h)
struct {
    unsigned short code;        // Scancode (0x00-0xFF)
    unsigned short state;       // Key state + E0/E1 flags
    unsigned int information;   // Extended data
};
```

**Key Flow:**
```
Hardware → Interception Driver → HardwareIO::waitForKey()
  → KeyEvent{scancode, isDown} → Pipeline → ProcessingResult
  → HardwareIO::sendKey() → Interception Driver → Windows
```

**Abstraction Point:** Already has clean interface (`waitForKey`, `sendKey`) - just needs interface extraction.

#### 2. System Tray UI (205 lines)
**File:** `src/SystemTray.h`

**Windows API Usage:**
- `RegisterClassEx()` - Register window class (line 62)
- `CreateWindowEx()` - Create hidden message-only window (line 66)
- `Shell_NotifyIcon()` - Add/modify/remove tray icon (lines 88, 118, 148)
- `LoadImage()` - Load keyboard-icon.ico (line 81)
- `CreatePopupMenu()`, `AppendMenu()`, `TrackPopupMenu()` - Context menu (lines 175-181)
- `PeekMessage()`, `TranslateMessage()`, `DispatchMessage()` - Message pump (lines 127-132)

**Message Loop Integration:**
- Non-blocking message polling in main event loop
- Handles WM_COMMAND, WM_RBUTTONUP, WM_LBUTTONUP
- Window procedure at lines 158-202

**Abstraction Needed:** Complete rewrite per platform (libappindicator on Linux, NSStatusBar on macOS).

#### 3. Application Lifecycle (177 lines)
**File:** `src/Application.h`

**Single-Instance Enforcement (lines 54-71):**
```cpp
singleInstanceMutex_ = CreateMutexA(nullptr, TRUE,
    "Global\\KeyflowSingleInstanceMutex");
if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // Another instance already running
}
```

**Cleanup (lines 161-165):**
- `ReleaseMutex()` - Release mutex on shutdown
- `CloseHandle()` - Close Windows kernel handles

**Abstraction Needed:** Platform-specific single-instance mechanisms (file locks on Unix).

#### 4. Console & Signal Handling (248 lines)
**File:** `src/main.cpp`

**Console Management (lines 43-46):**
```cpp
HWND console = GetConsoleWindow();
if (console)
    ShowWindow(console, SW_HIDE);  // Hide in Release build
```

**Signal Handling (lines 34-40, 127-131):**
- Windows: `SetConsoleCtrlHandler()` for Ctrl+C/Ctrl+Break
- POSIX: `std::signal(SIGINT)` and `std::signal(SIGTERM)` (already cross-platform)

**Abstraction Needed:** Minimal - mostly console hiding (Windows-only feature).

#### 5. Build System
**File:** `CMakeLists.txt`

**Windows-Specific Configuration:**
```cmake
# MSVC compiler flags (lines 22-28)
if(MSVC)
    add_compile_options(/W4 /WX /MP)
endif()

# Link Interception driver (lines 39, 52)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/platform)
target_link_libraries(keyflow PRIVATE interception)

# Copy interception.dll to output (lines 70-74)
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/interception.dll"
        "$<TARGET_FILE_DIR:keyflow>/interception.dll"
)
```

**Abstraction Needed:** Platform detection with conditional sources and libraries.

---

### Platform-Agnostic Core Analysis

Identified **~2500 lines** of reusable code requiring **ZERO changes**:

#### Pipeline Architecture (95 lines)
- `pipeline/Pipeline.h` (98 lines) - Orchestrates processor chain
- `pipeline/Context.h` (81 lines) - Mutable state (64 bytes, cache-optimized)
- `pipeline/IProcessor.h` (52 lines) - Pure virtual interface
- `pipeline/Modifiers.h` (252 lines) - Modifier bit flags and helpers

**Key Design:** Simple data flow pattern - initialize Context, run processors, return result. Zero I/O, zero platform dependencies.

#### Processors (600+ lines)
All processors implement `IProcessor::process(Context&)` with no platform dependencies:

- **Rewire.h** (77 lines) - O(1) scancode lookup table
- **ComboAdvanced.h** (178 lines) - Modifier+key combos with layer support
- **ModifierTracker.h** (126 lines) - Tracks held modifiers (Shift, Ctrl, Alt, custom)
- **Combo.h** (92 lines) - Simpler combo processor
- **CapsLockBlocker.h**, **LayerTriggerBlocker.h**, **StrictModeFilter.h** - Pure data transformation

**Key Quality:** Zero I/O, zero system calls, zero allocations in hot path.

#### Configuration System (1000+ lines)
- **ConfigLoader.h** (440 lines) - JSON parsing, validation (uses nlohmann/json - cross-platform)
- **ConfigBuilder.h** (478 lines) - Pipeline construction from config
- **JsonConfig.h** - Data structures (remappings, layers, customModifiers)
- **KeyNameMapper.h** - String names ("CapsLock") → scancodes (0x3A)

**Already Cross-Platform:** Uses standard C++ `std::ifstream` for file I/O.

#### Data Models (300+ lines)
- **KeyEvent.h** (29 lines) - Trivially copyable (8 bytes: `uint16_t scancode, bool isDown`)
- **Scancodes.h** (410 lines) - HID standard scancodes (SC_ESCAPE, SC_LSHIFT, etc.)
  - Extended key handling (E0/E1 prefixes)
  - Helper functions: `isModifier()`, `isNumpad()`, `isExtendedKey()`
  - Human-readable names for all 100+ keys
- **Config.h** (55 lines) - Runtime settings, command-line parsing

**Platform-Agnostic:** Scancodes are hardware-neutral identifiers once abstracted from driver APIs.

---

### KANATA Architecture Research

**Key Findings from KANATA (Rust-based cross-platform keyboard remapper):**

#### Platform Backend Strategy
- **Windows:** Uses Interception driver (same as Keyflow!)
- **Linux:** Uses evdev for input reading, uinput for output injection
- **macOS:** Uses Karabiner-DriverKit (low-level kernel extension alternative)

#### Architectural Patterns
1. **Single Config Format:** Same config works across all platforms
2. **Event Stream Abstraction:** Platform backends all produce same event stream
3. **Scancode Normalization:** Internal representation is platform-agnostic
4. **Layer System:** Complex layer/mod-tap features are platform-independent

#### Performance Characteristics
- Sub-2ms latency on all platforms
- Hot path optimization critical (no allocations, O(1) lookups)
- Minimal overhead from platform abstraction

**Key Takeaway:** Abstraction overhead is negligible with proper design - KANATA proves cross-platform is viable.

---

### Linux Platform Research

#### evdev/uinput Stack
**evdev (Event Device Interface):**
- Linux kernel's generic input event interface
- Device nodes: `/dev/input/event0`, `/dev/input/event1`, ...
- Each device represents physical hardware (keyboard, mouse)
- API: `libevdev` provides type-safe wrappers

**uinput (Userspace Input):**
- Allows userspace programs to create virtual input devices
- Device node: `/dev/uinput`
- Write events to virtual device → kernel delivers to applications

**Proven Tools Using This Stack:**
- **keyd** - Keyboard remapping daemon (5.3k stars on GitHub)
- **xremap** - Rust-based remapper (3.8k stars)
- **evsieve** - Event router/modifier (700+ stars)
- **interception-tools** - Linux equivalent of Windows Interception

**Permission Models:**
1. Run as root (not recommended for security)
2. Add user to `input` group: `usermod -aG input $USER`
3. udev rules for device access: `/etc/udev/rules.d/99-keyflow.rules`

**Code Example:**
```c
// Reading events with libevdev
struct libevdev* dev;
int fd = open("/dev/input/event3", O_RDONLY | O_NONBLOCK);
libevdev_new_from_fd(fd, &dev);

// Grab device exclusively
libevdev_grab(dev, LIBEVDEV_GRAB);

// Read events
struct input_event ev;
while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
    if (ev.type == EV_KEY) {
        // ev.code is evdev keycode (e.g., KEY_ESC = 1)
        // ev.value: 0 = release, 1 = press, 2 = repeat
    }
}

// Write to uinput
int uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
// ... setup virtual device ...
write(uinput_fd, &ev, sizeof(ev));
```

**Scancode Mapping Challenge:**
- evdev uses different key codes than Windows
- Example: ESC is 1 in evdev, 0x01 in Windows (happens to match)
- Example: CapsLock is 58 in evdev, 0x3A (58 decimal) in Windows (matches!)
- Example: Left Ctrl is 29 in evdev, 0x1D (29 decimal) in Windows
- **Many codes align, but translation table still needed for extended keys**

---

### macOS Platform Research

#### Quartz Event Services (CGEvent API)
**Event Tap Mechanism:**
```objc
// Create event tap at HID level (requires accessibility permissions)
CGEventTapCallBack callback = ^CGEventRef(CGEventTapProxy proxy,
                                          CGEventType type,
                                          CGEventRef event,
                                          void* userInfo) {
    // Intercept keyboard event
    // Return nullptr to consume, return event to pass through
};

CFMachPortRef eventTap = CGEventTapCreate(
    kCGHIDEventTap,              // Tap at hardware level
    kCGHeadInsertEventTap,       // Insert at head of event stream
    kCGEventTapOptionDefault,    // Monitor and modify
    CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
    callback,
    nullptr
);

// Add to run loop
CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(NULL, eventTap, 0);
CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
```

**Permission Model:**
- Requires "Accessibility" permission (System Preferences > Security & Privacy)
- Check with `AXIsProcessTrusted()`
- First run prompts user to grant permission
- App must be restarted after granting permission

**Event Model Differences:**
- **Async callbacks** (not blocking wait like Windows/Linux)
- **Run loop integration** required
- **No device concept** (system-wide event stream)

**Scancode System:**
- macOS uses "virtual key codes" (different from Windows scancodes)
- Example: Escape is 53, A is 0, Space is 49
- Translation table required: macOS key codes ↔ Windows scancodes

**Challenges:**
1. Async model requires event queue + run loop integration
2. Permission prompts can be confusing for users
3. Event tap can be disabled by system (e.g., after screen lock)
4. Must handle tap re-enabling gracefully

---

### Architectural Analysis Summary

#### Natural Abstraction Boundaries

**Boundary 1: Hardware Interface (Easiest)**
```
Current HardwareIO Interface (already clean):
  - waitForKey(timeout) → optional<KeyEvent>
  - sendKey(scancode, isDown) → void

Platform Implementations:
  - HardwareIO_Windows: Interception driver
  - HardwareIO_Linux: evdev + uinput
  - HardwareIO_MacOS: CGEvent + run loop + queue
```

**Boundary 2: UI Integration (Medium)**
```
SystemTray Interface Needed:
  - initialize(appName) → bool
  - showNotification(title, message) → void
  - processMessages() → bool (false = exit requested)
  - cleanup() → void

Platform Implementations:
  - Windows: Win32 (Shell_NotifyIcon, TrackPopupMenu)
  - Linux: libappindicator3 or Qt
  - macOS: NSStatusBar + NSMenu
```

**Boundary 3: Platform Services (Simple)**
```
IPlatformServices Interface:
  - checkSingleInstance() → bool
  - setupSignalHandlers(callback) → void
  - checkPermissions() → PermissionStatus

Platform Implementations:
  - Windows: Named mutexes, SetConsoleCtrlHandler
  - Linux: File locks (flock), POSIX signals
  - macOS: BSD locks, POSIX signals, AXIsProcessTrusted()
```

#### Porting Effort Estimation

| Component | Existing Lines | New Lines per Platform | Effort |
|-----------|----------------|------------------------|--------|
| HardwareIO | 91 | ~150-200 | Medium (driver API complexity) |
| SystemTray | 205 | ~150-250 | Medium (UI framework differences) |
| Platform Services | Scattered (~50) | ~50-100 | Low (standard APIs) |
| Build System | CMakeLists | ~30-50 per platform | Low (conditional compilation) |
| **Total** | **~878** | **~350-600 per platform** | **3-5 weeks per platform** |

**Core Logic:** ~2500 lines require **ZERO changes** ✅

---

## Architecture Strategy

### Layer Separation Pattern

```
┌─────────────────────────────────────────────────────────────┐
│                    Platform-Agnostic Core                    │
│  (Pipeline, Processors, Config, Business Logic)              │
│  ~2500 lines - ZERO CHANGES NEEDED                          │
└────────────────────┬────────────────────────────────────────┘
                     │ Clean Interfaces
                     │
┌────────────────────┴────────────────────────────────────────┐
│              Platform Abstraction Layer (NEW)                │
│  • IHardwareIO - keyboard capture/injection interface       │
│  • ISystemTray - UI notifications interface                 │
│  • IPlatformServices - OS utilities interface               │
└────────────────────┬────────────────────────────────────────┘
                     │ Factory Pattern
                     │
┌────────────────────┴────────────────────────────────────────┐
│           Platform-Specific Implementations                  │
│  Windows: Interception + Win32                              │
│  Linux:   evdev/uinput + libappindicator                    │
│  macOS:   Quartz Events + NSStatusBar                       │
└─────────────────────────────────────────────────────────────┘
```

### Directory Structure

```
src/
├── main.cpp                    # Platform-agnostic (REFACTORED)
├── Application.h/.cpp          # Platform-agnostic (REFACTORED)
│
├── hardware/                   # Hardware abstraction
│   ├── IHardwareIO.h           # NEW: Pure virtual interface
│   ├── HardwareFactory.h       # NEW: Factory function
│   ├── KeyEvent.h              # Existing (no changes)
│   ├── Scancodes.h             # Existing (no changes)
│   └── platform/               # Platform implementations
│       ├── HardwareIO_Windows.h/.cpp    # Move existing code here
│       ├── HardwareIO_Linux.h/.cpp      # NEW: evdev/uinput
│       └── HardwareIO_MacOS.h/.cpp      # NEW: Quartz Events
│
├── ui/                         # UI abstraction
│   ├── ISystemTray.h           # NEW: Pure virtual interface
│   └── platform/
│       ├── SystemTray_Windows.h/.cpp    # Move existing code
│       ├── SystemTray_Linux.h/.cpp      # NEW: libappindicator
│       └── SystemTray_MacOS.h/.cpp      # NEW: NSStatusBar
│
├── platform/                   # Platform utilities
│   ├── IPlatformServices.h     # NEW: Interface
│   └── impl/
│       ├── PlatformServices_Windows.cpp
│       ├── PlatformServices_Linux.cpp
│       └── PlatformServices_MacOS.cpp
│
├── pipeline/                   # Existing (no changes)
├── processors/                 # Existing (no changes)
└── config/                     # Existing (no changes)
```

---

## Implementation Phases

### Phase 1: Foundation & Abstraction (Weeks 1-2)

**Goal:** Create platform abstraction layer without breaking Windows build

#### Task 1.1: Hardware Abstraction Interface

**Create new files:**
1. `src/hardware/IHardwareIO.h` - Pure virtual interface:
   ```cpp
   class IHardwareIO {
   public:
       virtual ~IHardwareIO() = default;
       virtual bool initialize() noexcept = 0;
       virtual void shutdown() noexcept = 0;
       virtual std::optional<KeyEvent> waitForKey(int timeoutMS = 0) noexcept = 0;
       virtual void sendKey(uint16_t scancode, bool isDown) noexcept = 0;
       virtual bool isInitialized() const noexcept = 0;
   };
   ```

2. `src/hardware/HardwareFactory.h` - Factory function:
   ```cpp
   std::unique_ptr<IHardwareIO> createHardwareIO();
   ```

3. Move existing code:
   - `src/hardware/HardwareIO.cpp` → `src/hardware/platform/HardwareIO_Windows.cpp`
   - Create `src/hardware/platform/HardwareIO_Windows.h`
   - Make it inherit from `IHardwareIO`

**Modify existing files:**
4. `src/Application.h`:
   - Change `HardwareIO hardware_;` to `std::unique_ptr<IHardwareIO> hardware_;`
   - Use factory function in constructor

5. `CMakeLists.txt`:
   - Add conditional compilation for Windows
   - Set `PLATFORM_SOURCES` based on OS detection

**Verification:**
- Windows build compiles and runs
- All existing tests pass
- No functionality changes

#### Task 1.2: System Tray Abstraction

**Create new files:**
1. `src/ui/ISystemTray.h` - Interface for tray operations
2. `src/ui/platform/SystemTray_Windows.h/.cpp` - Move existing SystemTray.h code

**Modify:**
3. `src/Application.h` - Use `std::unique_ptr<ISystemTray>`

**Verification:**
- Windows tray icon works
- Notifications display
- Context menu functional

#### Task 1.3: Platform Services Abstraction

**Create new files:**
1. `src/platform/IPlatformServices.h` - Interface for:
   - Single-instance enforcement
   - Signal handling setup
   - Permission checks

2. `src/platform/impl/PlatformServices_Windows.cpp` - Extract:
   - Mutex logic from Application.h
   - Signal handlers from main.cpp

**Modify:**
3. `src/Application.h` - Remove Windows-specific mutex code
4. `src/main.cpp` - Use platform services

**Verification:**
- Single instance enforcement works
- Ctrl+C/Ctrl+Break handling works
- Windows build fully functional

---

### Phase 2: Linux Implementation (Weeks 3-5)

**Goal:** Full Linux support with evdev/uinput

#### Task 2.1: Linux Hardware Layer

**Implementation:** `src/hardware/platform/HardwareIO_Linux.cpp`

**Key APIs:**
- libevdev: Event reading from `/dev/input/eventX`
- uinput: Virtual device creation for key injection
- udev: Device enumeration (find keyboards)

**Implementation steps:**
1. `initialize()`:
   - Enumerate `/dev/input/event*` devices using udev
   - Open each device, check if keyboard (has KEY capability)
   - Grab devices exclusively with `LIBEVDEV_GRAB`
   - Create single uinput virtual device for output

2. `waitForKey()`:
   - Poll all keyboard devices with timeout
   - Read events with `libevdev_next_event()`
   - Translate evdev codes → Windows scancodes
   - Return KeyEvent

3. `sendKey()`:
   - Translate Windows scancode → evdev code
   - Write to uinput device

**Dependencies:**
- libevdev-dev
- libudev-dev

**Challenges:**
- Permission requirements (root or input group)
- Scancode translation table (evdev uses different codes)
- Multiple keyboard support

#### Task 2.2: Linux System Tray

**Implementation:** `src/ui/platform/SystemTray_Linux.cpp`

**Approach:** libappindicator3 (GNOME/Ubuntu standard)

**Dependencies:**
- libappindicator3-dev
- libgtk-3-dev

**Fallback:** CLI-only mode if no tray support

#### Task 2.3: Linux Platform Services

**Implementation:** `src/platform/impl/PlatformServices_Linux.cpp`

**Features:**
- Single-instance: File locking with `flock()`
- Signal handling: SIGINT, SIGTERM, SIGHUP
- Permission check: `geteuid()` or input group membership

#### Task 2.4: CMake Linux Support

**Updates to CMakeLists.txt:**
```cmake
if(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES
        src/hardware/platform/HardwareIO_Linux.cpp
        src/ui/platform/SystemTray_Linux.cpp
        src/platform/impl/PlatformServices_Linux.cpp
    )
    set(PLATFORM_LIBS evdev udev appindicator3 gtk-3)
    add_compile_definitions(KEYFLOW_PLATFORM_LINUX)
endif()
```

**Verification:**
- Builds on Ubuntu 22.04+
- Intercepts keyboard correctly
- System tray appears
- All remapping features work

---

### Phase 3: macOS Implementation (Weeks 6-8)

**Goal:** Full macOS support with Quartz Event Services

#### Task 3.1: macOS Hardware Layer

**Implementation:** `src/hardware/platform/HardwareIO_MacOS.cpp`

**Key APIs:**
- CGEvent: Event tap creation with `CGEventTapCreate()`
- Quartz Event Services: System-level event interception
- Core Graphics: Event posting with `CGEventPost()`

**Implementation steps:**
1. `initialize()`:
   - Check accessibility permissions with `AXIsProcessTrusted()`
   - If denied, prompt user to grant in System Preferences
   - Create event tap with `CGEventTapCreate()`
   - Add tap to run loop

2. `waitForKey()`:
   - macOS uses async callback model
   - Maintain internal event queue
   - Poll queue with timeout, integrate with run loop

3. `sendKey()`:
   - Create CGEvent with scancode
   - Post with `CGEventPost()`

**Dependencies:**
- CoreGraphics.framework
- Carbon.framework (for key codes)

**Challenges:**
- Async event model (callbacks, not blocking wait)
- Permission prompts (System Preferences)
- Different scancode system
- Event tap stability

#### Task 3.2: macOS System Tray

**Implementation:** `src/ui/platform/SystemTray_MacOS.cpp`

**Approach:** Cocoa/Objective-C++
- NSStatusBar for tray icon
- NSMenu for context menu
- NSUserNotification for notifications

#### Task 3.3: macOS Platform Services

**Implementation:** `src/platform/impl/PlatformServices_MacOS.cpp`

**Features:**
- Single-instance: BSD file locks
- Signal handling: POSIX signals
- Permission checks: Accessibility API

#### Task 3.4: CMake macOS Support

**Updates to CMakeLists.txt:**
```cmake
if(APPLE)
    set(PLATFORM_SOURCES
        src/hardware/platform/HardwareIO_MacOS.cpp
        src/ui/platform/SystemTray_MacOS.cpp
        src/platform/impl/PlatformServices_MacOS.cpp
    )
    set(PLATFORM_LIBS
        "-framework CoreGraphics"
        "-framework Carbon"
        "-framework Cocoa"
    )
    add_compile_definitions(KEYFLOW_PLATFORM_MACOS)
endif()
```

**Verification:**
- Builds on macOS 12+
- Permission prompts appear
- Event interception works
- System tray displays
- All features functional

---

### Phase 4: Build System & Testing (Weeks 9-10)

#### Task 4.1: CMake Multi-Platform Build

**Refactor CMakeLists.txt:**
```cmake
# Platform detection
if(WIN32)
    # Windows-specific
elseif(APPLE)
    # macOS-specific
elseif(UNIX)
    # Linux-specific
endif()

# Common sources (platform-agnostic)
set(COMMON_SOURCES
    src/main.cpp
    src/Application.cpp
    src/pipeline/Pipeline.cpp
    # ... all platform-agnostic files
)

# Add executable with platform-specific sources
add_executable(keyflow ${COMMON_SOURCES} ${PLATFORM_SOURCES})
target_link_libraries(keyflow PRIVATE ${PLATFORM_LIBS})
```

#### Task 4.2: CI/CD Setup

**GitHub Actions workflows:**
1. `windows-build.yml` - Windows Server 2022
2. `linux-build.yml` - Ubuntu 22.04
3. `macos-build.yml` - macOS 13/14

**Each workflow:**
- Checkout code
- Install dependencies
- Build Release + Debug
- Run unit tests
- Generate release artifacts

#### Task 4.3: Testing Strategy

**Unit Tests:**
- Mock IHardwareIO implementations
- Test platform factories
- Verify scancode translation tables
- All existing processor tests (unchanged)

**Manual Testing Checklist (per platform):**
- [ ] Basic key remapping (CapsLock → Ctrl)
- [ ] Layer activation (RightAlt + HJKL → arrows)
- [ ] Modifier combos
- [ ] System tray icon
- [ ] Notifications
- [ ] Exit via tray menu
- [ ] Ctrl+Escape hotkey
- [ ] Config reload
- [ ] Single instance enforcement

---

### Phase 5: Documentation & Release (Weeks 11-12)

#### Task 5.1: Documentation

**Create/update:**
1. `docs/CROSS_PLATFORM.md` - Platform comparison, limitations
2. `docs/PLATFORM_PORTING.md` - Guide for adding new platforms
3. `docs/INSTALL_LINUX.md` - Linux setup instructions
4. `docs/INSTALL_MACOS.md` - macOS setup instructions
5. Update README.md with platform-specific sections

#### Task 5.2: Packaging

**Windows:**
- Existing installer + portable ZIP
- No changes needed

**Linux:**
- DEB package (Debian/Ubuntu)
- RPM package (Fedora/RHEL)
- AppImage (universal)
- AUR package (Arch)

**macOS:**
- DMG installer
- Homebrew tap
- Code signing for Gatekeeper

#### Task 5.3: Release

**Version:** v3.0.0 (major release)

**Release notes:**
- Cross-platform support announcement
- Platform-specific installation guides
- Migration notes (configs unchanged)
- Known limitations per platform

---

## Platform-Specific Technical Details

### Linux: evdev/uinput Architecture

**Device Paths:**
- Input devices: `/dev/input/event0`, `/dev/input/event1`, ...
- Virtual output: `/dev/uinput`

**Permissions:**
- Option 1: Run as root (not recommended)
- Option 2: Add user to `input` group
- Option 3: udev rule for device access

**Scancode Translation:**
```cpp
// evdev codes → Windows scancodes
static const std::unordered_map<uint16_t, uint16_t> evdevToWindows = {
    {KEY_ESC, 0x01},
    {KEY_1, 0x02},
    {KEY_CAPSLOCK, 0x3A},
    {KEY_LEFTCTRL, 0x1D},
    // ~200 mappings total
};
```

### macOS: Quartz Event Services

**Event Tap Levels:**
- kCGHIDEventTap: Hardware-level (requires accessibility)
- kCGSessionEventTap: Session-level (limited)

**Permission Prompt:**
```cpp
if (!AXIsProcessTrusted()) {
    // Show dialog to user
    // Open System Preferences > Security & Privacy
    // Wait for permission grant
}
```

**Event Queue Pattern:**
```cpp
// Callback from event tap (async)
CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
                   CGEventRef event, void* userInfo) {
    // Convert CGEvent → KeyEvent
    // Push to thread-safe queue
    return nullptr; // Consume event
}

// Main thread polling
std::optional<KeyEvent> waitForKey(int timeoutMS) {
    // Poll queue with timeout
    // Integrate with CFRunLoop
}
```

### Windows: No Changes

**Existing implementation preserved:**
- Interception driver continues to work
- All Windows-specific code moves to `platform/` directory
- No functional changes
- Same performance characteristics

---

## Critical Files for Implementation

### Phase 1 (Foundation):
1. `src/hardware/HardwareIO.h` - Becomes IHardwareIO interface
2. `src/hardware/HardwareIO.cpp` - Moves to platform/HardwareIO_Windows.cpp
3. `src/Application.h` - Refactor to use interfaces
4. `CMakeLists.txt` - Add platform detection

### Phase 2 (Linux):
5. `src/hardware/platform/HardwareIO_Linux.cpp` - New implementation
6. `src/ui/platform/SystemTray_Linux.cpp` - New implementation
7. `src/platform/impl/PlatformServices_Linux.cpp` - New implementation

### Phase 3 (macOS):
8. `src/hardware/platform/HardwareIO_MacOS.cpp` - New implementation
9. `src/ui/platform/SystemTray_MacOS.cpp` - New implementation
10. `src/platform/impl/PlatformServices_MacOS.cpp` - New implementation

---

## Key Technical Decisions

### 1. Interface Pattern
**Choice:** Pure virtual base classes (IHardwareIO, ISystemTray, IPlatformServices)

**Rationale:**
- Clear separation of concerns
- Easy to test with mocks
- Factory pattern works naturally
- Virtual function overhead negligible (<0.1% total latency)

### 2. Scancode Normalization
**Choice:** Keep Windows scancodes internally, translate at hardware boundaries

**Rationale:**
- Existing code uses Windows scancodes
- Zero changes to processors, configs, tests
- Translation only in HardwareIO implementations
- Users can share same config across platforms

### 3. Build System
**Choice:** Single CMakeLists.txt with platform detection

**Rationale:**
- Simpler than multiple build files
- CMake handles platform detection well
- Conditional compilation per platform
- Standard C++ practice

### 4. Config Compatibility
**Choice:** Single config.json format works on all platforms

**Rationale:**
- User convenience (share configs)
- Platform-specific overrides optional
- Scancode translation handles differences
- Most mappings are universal

---

## Timeline

| Phase | Duration | Milestone |
|-------|----------|-----------|
| Phase 1: Foundation | 2 weeks | Windows still works, abstraction in place |
| Phase 2: Linux | 3 weeks | Linux build functional |
| Phase 3: macOS | 3 weeks | macOS build functional |
| Phase 4: Build & Test | 2 weeks | CI/CD complete, all tests pass |
| Phase 5: Documentation | 2 weeks | Docs complete, v3.0.0 released |
| **Total** | **12 weeks** | **Full cross-platform support** |

---

## Success Criteria

### Must Have:
- ✅ Windows: 100% feature parity with current v2.0.2
- ✅ Linux: Full functionality on Ubuntu 22.04+
- ✅ macOS: Full functionality on macOS 12+
- ✅ Single config.json works on all platforms
- ✅ <2ms latency on all platforms
- ✅ All existing tests pass
- ✅ Documentation for each platform
- ✅ CI builds for all platforms

### Should Have:
- Package managers (Homebrew, AUR, APT PPA)
- Auto-update mechanism
- Platform-specific optimizations

### Nice to Have:
- GUI configuration tool
- Wayland support on Linux
- BSD support

---

## Risk Assessment

### High Risk:
1. **macOS Event Tap Stability** - System can disable event taps unpredictably
   - Mitigation: Implement robust re-enabling logic
2. **Linux Permission Complexity** - Different distros have different security models
   - Mitigation: Provide multiple permission methods, clear documentation

### Medium Risk:
1. **Scancode Translation Bugs** - Edge cases in translation tables
   - Mitigation: Comprehensive testing with international keyboards
2. **Performance Regression** - Virtual function overhead, queue latency
   - Mitigation: Benchmark each platform, optimize hot paths

### Low Risk:
1. **Build System Complexity** - CMake conditional compilation
   - Mitigation: Well-understood patterns, existing examples
2. **UI Framework Differences** - System tray implementations vary
   - Mitigation: Optional tray, CLI-only fallback

---

## Next Steps

1. **Review this plan** - Ensure alignment with project goals
2. **Set up development environments** - Access to Linux VM and macOS machine
3. **Begin Phase 1** - Create abstraction layer (Tasks #1-9)
4. **Test Windows thoroughly** - Ensure no regression
5. **Prototype Linux** - Validate evdev/uinput approach (Tasks #10-14)
6. **Iterate and refine** - Adjust plan based on learnings

---

## Questions for Review

1. **Priority:** Should we target Linux or macOS first after Phase 1?
2. **Scope:** Are there any features we should defer to v3.1.0?
3. **Resources:** Do we have access to Linux and macOS development machines?
4. **Testing:** Should we set up automated cross-platform testing in CI before Phase 2?
5. **Release:** Should we do beta releases (v3.0.0-beta.1) for early adopters?

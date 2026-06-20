# BCScan

![BCScan main window](screenshots/screenshot1.png)

A desktop controller and programming application for the **Uniden BC125AT** handheld scanner, built with Qt.

Connect your scanner over USB, monitor activity in real time, program channels, and log transmissions — all from a clean, dark-themed interface that mirrors the scanner's LCD.

---

## Download

**[→ Download the latest release](https://github.com/jamesburnettio/BCScanner/releases/latest)**

Pre-built installers are available for Linux Mint / Debian (`.deb`), Fedora / RHEL (`.rpm`), and Windows (`.exe`).

All releases: [github.com/jamesburnettio/BCScanner/releases](https://github.com/jamesburnettio/BCScanner/releases)

---

## Table of Contents

- [Download](#download)
- [Features](#features)
- [Screenshots](#screenshots)
- [Requirements](#requirements)
- [Building](#building)
- [Linux Setup & Troubleshooting](#linux-setup--troubleshooting)
- [Self-Test Suite](#self-test-suite)
- [Scanner Compatibility](#scanner-compatibility)
- [Project Structure](#project-structure)
- [License](#license)

---

## Features

- **Real-time LCD mirror** — frequency, channel label, signal strength, and bank indicators update live via the scanner's `STS` command (4×/sec)
- **SCAN / HOLD / SEARCH / EXPLORE** controls with automatic state detection — buttons enable and disable based on what the scanner is actually doing
- **Service Search** — pick from Police, Fire/Emergency, HAM Radio, Marine, Railroad, Civil Air, Military Air, CB Radio, FRS/GMRS/MURS, and Racing; the app configures the scanner and starts searching
- **Explore** — triggers the scanner's custom frequency range search mode
- **Transmission logging** — hits are auto-detected when squelch opens; short blips and long digital bursts are filtered out (configurable thresholds); a per-session transmission log shows channel, frequency, CTCSS/DCS, and duration
- **Auto-skip / resume** — after a configurable timeout a long-running transmission is skipped and the scanner resumes the mode it was in (Scan, Explore, or Service Search)
- **Channel programming** — read, edit, and write all 500 channels with full CTCSS/DCS support
- **Channel cache** — bulk-load all 500 channels from the scanner for instant display while scanning
- **Number key buttons** — send `KEY,1,P` through `KEY,0,P` directly to the scanner; visually reflect which banks are active via STS field data
- **Volume / Squelch sliders** — live control without entering program mode
- **Backlight toggle**
- **Debug console** — serial monitor with STS field breakdown and a raw command console for testing undocumented commands
- **Settings** — serial port, log directory, auto-connect on startup, transmission duration thresholds, auto-skip timeout, service search group presets
- **Persistent window size and serial port** via `QSettings`

---

## Screenshots

**Holding on a GMRS channel with activity log**
![Hold mode](screenshots/screenshot1.png)

**Service Search — Racing frequencies**
![Service search mode](screenshots/scerenshot2.png)

**Scanning with transmission log**
![Scanning with transmissions](screenshots/screenshot3.png)

---

## Requirements

| Dependency | Version |
|---|---|
| Qt | 5.15 or 6.x |
| CMake | 3.16+ |
| C++ compiler | C++17 (GCC, Clang, MSVC) |

Qt modules used: `Core`, `Gui`, `Widgets`, `SerialPort`

---

## Building

### Linux (Ubuntu / Debian)

**1. Install dependencies**

Qt5:
```bash
sudo apt install git cmake build-essential qtbase5-dev qtserialport5-dev
```

Qt6:
```bash
sudo apt install git cmake build-essential qt6-base-dev qt6-serialport-dev
```

**2. Clone and build**

```bash
git clone https://github.com/YOUR_USERNAME/BCScanner.git
cd BCScanner
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**3. Run**

```bash
./build/bcscan
```

**4. Serial port access (one-time setup)**

```bash
sudo usermod -aG dialout $USER
# Log out and back in for the change to take effect
```

The scanner appears as `/dev/ttyUSB0` or `/dev/ttyACM0`.

---

### Windows

#### Prerequisites (install once)

1. Download and run the **Qt Online Installer** from [https://www.qt.io/download-open-source](https://www.qt.io/download-open-source) (free account required)
   - Under the Qt version you want (6.x recommended, or 5.15), select:
     - `MSVC 2022 64-bit` **or** `MinGW 64-bit`
     - `Qt Serial Port` (listed under the same Qt version node)
   - Under **Developer and Designer Tools**, CMake is included — leave it checked

2. Install **Git** from [https://git-scm.com](https://git-scm.com)

#### Option A — Qt Creator (easiest)

```
1. git clone https://github.com/YOUR_USERNAME/BCScanner.git
2. Open Qt Creator
3. File → Open File or Project → select BCScanner\CMakeLists.txt
4. Qt Creator detects your installed kit automatically — click Configure
5. Click the Build button (Ctrl+B)
```

The executable is placed in `build\Release\bcscan.exe` (MSVC) or `build\bcscan.exe` (MinGW).

#### Option B — Command line with MSVC

Open the **x64 Native Tools Command Prompt for VS 2022**, then:

```bat
git clone https://github.com/YOUR_USERNAME/BCScanner.git
cd BCScanner
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
```

Replace `6.x.x` and `msvc2022_64` with your actual installed Qt version and kit folder name.

#### Option C — Command line with MinGW

Open the **Qt MinGW command prompt** (installed alongside Qt), then:

```bat
git clone https://github.com/YOUR_USERNAME/BCScanner.git
cd BCScanner
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\mingw_64"
cmake --build build --parallel
```

#### Creating a standalone distributable folder

After building, use Qt's `windeployqt` to copy all required DLLs next to the executable:

```bat
mkdir dist
copy build\Release\bcscan.exe dist\
cd dist
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt bcscan.exe
```

The `dist` folder is fully self-contained — zip it up and distribute it. No Qt installation is required on the end user's machine.

The scanner appears as a `COM` port (e.g. `COM3`). Select it from the port drop-down in the toolbar.

---

### macOS

**1. Install prerequisites**

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt and CMake
brew install qt cmake
```

**2. Clone and build**

```bash
git clone https://github.com/YOUR_USERNAME/BCScanner.git
cd BCScanner
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
```

**3. Run**

```bash
open build/bcscan.app
```

Or directly from the terminal:

```bash
./build/bcscan.app/Contents/MacOS/bcscan
```

**4. Creating a distributable disk image**

```bash
$(brew --prefix qt)/bin/macdeployqt build/bcscan.app -dmg
```

This produces `bcscan.dmg` — a self-contained disk image ready to distribute.

The scanner appears as `/dev/tty.usbserial-*` or `/dev/tty.usbmodem*`. If the port list is empty, check **System Settings → Privacy & Security → USB**.

---

## Linux Setup & Troubleshooting

### The Problem

The BC125AT has a **malformed USB descriptor** that causes the standard `cdc_acm` kernel driver to reject it with `Error -22 (EINVAL)`. The device never binds to a driver, so nothing appears in `/dev/` — no `ttyUSB0`, no `ttyACM0`.

### The Solution

The device must be manually bound to the `usbserial_generic` driver. A udev rule handles this automatically on every plug-in.

**1. Create the udev rule**

```bash
sudo nano /etc/udev/rules.d/99-bc125at.rules
```

Paste the following single line:

```
ACTION=="add", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1965", ATTRS{idProduct}=="0017", RUN+="/sbin/modprobe usbserial_generic", RUN+="/bin/sh -c 'echo 1965 0017 > /sys/bus/usb-serial/drivers/generic/new_id'", MODE="0666"
```

Save and close the file.

**2. Reload udev rules**

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

**3. Replug the scanner**

Unplug and replug the BC125AT. It should now appear as `/dev/ttyUSB0` (or `/dev/ttyUSB1` if another device is already using `ttyUSB0`).

**4. Serial port access (one-time setup)**

```bash
sudo usermod -aG dialout $USER
# Log out and back in for the change to take effect
```

### Troubleshooting

#### Phantom symlink (`ttyUSB0 -> ttyACM0`)

If the port list shows a broken symlink or ModemManager is interfering with the device, add an ignore rule so ModemManager leaves the scanner alone:

```bash
sudo nano /etc/udev/rules.d/99-bc125at-ignore.rules
```

Paste:

```
ACTION=="add|change", SUBSYSTEM=="usb", ATTRS{idVendor}=="1965", ATTRS{idProduct}=="0017", ENV{ID_MM_DEVICE_IGNORE}="1"
```

Then reload rules and replug:

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

---

## Self-Test Suite

The binary includes a built-in test suite that exercises everything up to the physical serial port. It is designed to run headlessly in CI/CD pipelines — no scanner, no display required.

```bash
./bcscan --test
```

Exit code `0` means all tests passed. Any failure returns `1`. Output is plain text suitable for CI logs.

---

### STS Parsing (23 tests)

Tests the parser that processes every `STS` response from the scanner. The `STS` command is the backbone of the app — it mirrors the scanner's LCD display over serial as a comma-separated string, and all real-time state (scanning, active channel, search, menu) is derived from it.

| Test | What it checks |
|---|---|
| Scanning state | `SCAN` in the main display field → state is `Scanning`, squelch closed, no tone |
| Active channel | `CH027 467.6875` pattern → state is `Active`, squelch open, channel number and frequency extracted correctly |
| Active channel label | Channel label field (row 1 of the LCD) is captured |
| No CTCSS/DCS when field empty | An empty field[8] does not set a tone |
| Active + CTCSS | `C67.0` in field[8] → `ctcssDcs` is captured as `"C67.0"` |
| Active + DCS | `DCS032` in field[8] → `ctcssDcs` is captured as `"DCS032"` |
| Custom search with signal | `SEARCH BANK1` label + signal > 0 → state is `Search`, squelch open, frequency extracted |
| Custom search no signal | Signal strength 0 → squelch closed |
| Service search | Label containing a service keyword (e.g. `POLICE`) → state is `Search`, `serviceLabel` mapped to human-readable name (`"Police"`) |
| Service search squelch | Squelch opens when signal > 0 during service search |
| Menu / programming mode | All-`1` flags field → state is `Menu` |
| Bad input | Garbage string → `Unknown` state, no crash |
| Empty input | Empty string → `Unknown` state, no crash |

---

### CTCSS/DCS Label Table (8 tests)

Tests the lookup table that maps internal scanner tone codes (integers) to human-readable labels shown in the UI and written to the transmission log.

| Test | What it checks |
|---|---|
| Code 0 | Maps to `"None / All"` |
| Code 64 | Maps to `"CTCSS 67.0 Hz"` |
| Code 76 | Maps to `"CTCSS 100.0 Hz"` |
| Code 113 | Maps to `"CTCSS 254.1 Hz"` (last CTCSS entry) |
| Code 128 | Maps to `"DCS 023"` (first DCS entry) |
| Code 132 | Maps to `"DCS 032"` |
| Code 240 | Maps to `"No Tone"` |
| Unknown code | Returns a fallback string (`"Code N"`) rather than crashing |

---

### Transmission Logger (11 tests)

Tests the object responsible for recording channel hits. A transmission begins when the squelch opens and ends when it closes; the logger measures duration, builds a record, writes a log file, and emits a signal to update the UI.

| Test | What it checks |
|---|---|
| Active after begin | `isActive()` returns true immediately after `beginTransmission` |
| Inactive after end | `isActive()` returns false after `endTransmission` |
| Signal emitted on end | `transmissionLogged` signal fires when `endTransmission` is called |
| Channel index in record | The channel number passed to `beginTransmission` appears in the emitted record |
| Frequency in record | The frequency string is preserved through to the logged record |
| CTCSS/DCS label in record | The tone label is preserved through to the logged record |
| Inactive after cancel | `cancelTransmission` clears the active state |
| No signal on cancel | Cancelling a transmission does not emit `transmissionLogged` |
| Mid-transmission tone update | Calling `updateCtcssDcs` after `beginTransmission` overwrites the initial (possibly empty) label; the updated value appears in the final logged record |
| Update when inactive is safe | Calling `updateCtcssDcs` when no transmission is active does not crash or store state |
| End when inactive is safe | Calling `endTransmission` when nothing is active does not emit a signal or crash |

---

### Settings (2 tests)

Tests that the `AppSettings` layer is operational and that stored values are within a sane operating range. These pass regardless of whether the user has customised the settings.

| Test | What it checks |
|---|---|
| `autoSkipSeconds` | Returns a value between 1 and 3600 |
| `minTransmissionSeconds` | Returns a value between 0 and 60 |

---

### UI Smoke Test (4 tests)

Creates real Qt widgets using the offscreen platform plugin (no display needed). These tests verify that the binary is correctly linked against Qt, that the widget hierarchy initialises without error, and that the shutdown path does not crash — the destructor chain for `MainWindow` is non-trivial due to Qt's parent-child ownership model and live signal connections on `ScannerSerial`.

| Test | What it checks |
|---|---|
| `LcdWidget` constructed | The custom LCD painter widget instantiates and calls `show()` without error |
| `LcdWidget` setters | All public setters (`setFrequency`, `setChannelLabel`, `setState`, etc.) can be called without crashing |
| `LcdWidget` destroyed cleanly | The destructor completes without error |
| `MainWindow` constructed and shown | The full application window — including `ScannerSerial`, `StsPoller`, `TransmissionLogger`, all timers, and all child widgets — instantiates and calls `show()` without error |
| `MainWindow` destroyed cleanly | The destructor chain completes without a crash or abort, confirming that signal connections are properly torn down on shutdown |

---

## Scanner Compatibility

Designed and tested with the **Uniden BC125AT**.

The application uses both standard documented commands and undocumented commands specific to the BC125AT firmware (notably the `STS` LCD-mirror command for real-time state detection). Other Uniden scanners with a similar serial protocol may work partially but are untested.

---

## Project Structure

```
BCScanner/
├── CMakeLists.txt           # Cross-platform build definition (Qt5 + Qt6)
├── bcscan.pro               # Alternative qmake project file
└── src/
    ├── main.cpp
    ├── mainwindow.*         # Main UI — controls, LCD mirror, log tabs
    ├── lcdwidget.*          # Custom LCD-style painter widget
    ├── scannerserial.*      # Async serial command queue (QSerialPort)
    ├── stspoller.*          # STS polling — real-time state, squelch detection
    ├── sqlpoller.*          # SQL latency squelch detector (legacy)
    ├── programwindow.*      # Channel programming dialog
    ├── settingswindow.*     # App settings dialog
    ├── debugwindow.*        # Serial monitor + raw command console
    ├── transmissionlogger.* # Hit recording and CSV logging
    ├── appsettings.*        # QSettings wrapper
    └── ctcssdcsdata.h       # CTCSS/DCS tone tables
```

---

## License

MIT License — see [LICENSE](LICENSE) for details.

Qt is used under the **LGPL v3** license. When distributed as a dynamically linked library (the default), no source-disclosure obligations apply to application code.

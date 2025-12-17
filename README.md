
# ESP32 Uptime Kuma Monitor

A small firmware that turns an ESP32 into a Uptime Kuma health‑check device.  
It monitors the reachability of one or more Uptime Kuma instances, checks that all
monitors inside those instances are **up**, and reflects the overall state on
two LEDs. The device is initially reachable through a temporary Wi‑Fi hotspot,
then connects to your home network and can be managed through a lightweight
web UI.

---

## Table of Contents
- [Hardware Requirements](#hardware-requirements)
- [Software Prerequisites](#software-prerequisites)
- [Installation & Flashing](#installation--flashing)
- [Initial Setup](#initial-setup)
- [Configuration](#configuration)
- [Rebooting & Updating](#rebooting--updating)
- [mDNS Discovery](#mdns-discovery)
- [LED Status Legend](#led-status-legend)
- [Debugging](#debugging)
- [Contributing](#contributing)
- [License](#license)

---

## Hardware Requirements

| Item | Specification | Notes |
|------|---------------|-------|
| **ESP32 board** | Any ESP32‑based development board | Recommended: DevKit v1 (GPIO pins shown below) |
| **USB cable** | For programming | |
| **Green LED** | 2 mm  | Connect through a 150 Ω resistor to a GPIO |
| **Red LED** | 2 mm  | Connect through a 150 Ω resistor to a GPIO |
| **Resistors** | 2 × 150 Ω | One per LED |
| **Optional** | None | The firmware is fully functional with just the two LEDs |

**Pin assignments** (default in `config.h`):

| LED | GPIO | Function |
|-----|------|----------|
| Green | `GPIO_22` | Status OK |
| Red | `GPIO_23` | Status Bad |

> If you need different pins, edit `src/config.h` and rebuild.

---

## Software Prerequisites

| Tool | How to install |
|------|----------------|
| **VS Code**| [Website](https://code.visualstudio.com/) |
| **PlatformIO** | [VS Code extension](https://docs.platformio.org/en/latest/integration/ide/vscode.html) |

> The firmware is written for the Arduino framework, PlatformIO is recommended for convenience.

---

## Installation & Flashing

1. **Clone or download** the repo.  
2. **Open** the project in VS Code.  
3. **Build & upload** the firmware:

   *PlatformIO tasks (shown in the *PIO Home* → *Project Tasks* panel)*

   | Task | When to use |
   |------|-------------|
   | **Erase Flash** | Only when you need a clean start (e.g. initially or after a corrupted flash). |
   | **Upload Filesystem Image** | When you modify the web UI files (`/data` folder). Note, this erases the current configuration that is also stored on the file system. |
   | **Upload** | Normal firmware update. |

   > On the first flash you typically run **Erase Flash** → **Upload Filesystem Image** → **Upload**.

4. **Open the Serial Monitor** at 115200 baud.  
   The device will print the AP credentials (SSID and password) that it
   creates for the initial setup.

---

## Initial Setup

| Step | Action | Details |
|------|--------|---------|
| 1 | **Connect** to the temporary hotspot | SSID: `UptimeKumaMonitor` (default) <br> Password: configured in ` config.h` and printed in the serial monitor |
| 2 | **Open a browser** and go to `http://esp-uptimemonitor.local` (or `http://<IP‑address>`) | The mDNS hostname defaults to `esp-uptimemonitor` (see `config.h`). |
| 3 | **Enter your home Wi‑Fi credentials** | SSID and password. |
| 4 | **Reboot** the ESP32 (either via the web UI or by pressing the reset button). | The ESP32 will join your network. |

After reboot you can navigate again to the mDNS hostname and add Uptime Kuma instances. 

> Check the onboard LED or the serial monitor for the WiFi status.

---

## Configuration

| Section | How to configure | Notes |
|---------|------------------|-------|
| **Wi‑Fi** | Web UI → *Wi‑Fi Settings* | Stored in `/config.json`. |
| **Uptime Kuma instances** | Web UI → *Instances* | For each instance you set: <br> 1. Hostname/IP <br> 2. API key (optional) <br> 3. Port (default 3001) |
| **LED pins** | Edit `src/config.h` | Example: `#define LED_GREEN_PIN GPIO_NUM_2` |
| **Hostname** | Edit `src/config.h` | `#define MDNS_HOSTNAME "esp-uptimemonitor"` |

> **Backing up the configuration**  
> Use the *Config (Download / Upload)* option in the web UI. It displays the configuration as JSON that can be imported later.

---

## Rebooting & Updating

* **Reboot** – Click the *Reboot* button in the web UI or press the reset button on the board.  
* **OTA updates** – Future releases may support OTA. For now, re‑flash via PlatformIO.  

> Changing the Wi‑Fi settings **does** require a reboot.  
> Adding or removing Uptime Kuma instances **does not**.

---

## mDNS Discovery

Once the ESP32 is connected to your network it advertises itself via mDNS.  
You can access the web UI at:

```
http://esp-uptimemonitor.local
```

If you changed the hostname in `config.h`, replace it accordingly.

---

## LED Status Legend

| LED / Indicator | Status |
|-----------------|--------|
| **Solid green** | All monitored Uptime Kuma instances are reachable and all monitors are up. |
| **Solid red**   | At least one monitor is down. |
| **Blinking red**| One or more instances are unreachable or an API‑key is invalid. |
| **Both off**    | No Uptime Kuma instances configured. |
| **Onboard LED (if present)** | • **Blinking** – AP hotspot active. <br> • **Solid** – Connecting to Wi‑Fi. <br> • **Off** – Connected to Wi‑Fi. |

> The onboard LED is optional; many boards include a built‑in LED connected to GPIO 2.

---

## Debugging

All internal events are logged to the serial console at 115200 baud.  
Typical troubleshooting steps:

1. **Serial output** – Look for "Connecting to", "Monitoring instance …", and any error messages.  
2. **LED pattern** – Matches the legend above.

---

## Contributing

Feel free to open issues or submit pull requests.  
Improvements, bug fixes, and new features are welcome!

---

## License

This project is licensed under the MIT License.  
See the [LICENSE](LICENSE) file for details.

**Attribution** – If you redistribute or modify this code, please reference the original repository:  
`https://github.com/HarmEllis/Uptime-Kuma-ESP32-Statusindicator` and give credit to the original creator.
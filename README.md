# InputRedirectionClient-Android

[中文版](./README.zh_CN.md)

Android port of the [InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) project.
Use your Android device as a wireless game-pad for Nintendo 3DS running Luma3DS.

## Requirements

1. Android 11+
2. 3DS with Luma3DS 8.0+

## Usage

### Basic Setup

[Video tutorial](https://b23.tv/IBf9KgU)

1. Make sure the Android device and the 3DS are on the same LAN. The 3DS can also connect directly to the Android device's Wi-Fi hotspot.
2. Connect a game-pad to the Android device.
   > **Note**: For NS Joy-Con, rename your Android device to `Nintendo` in Bluetooth settings to reduce latency.
3. Start InputRedirection on the 3DS:
   1. Open the Rosalina menu with `L + Select + D-Pad Down`.
   2. Choose `Miscellaneous options...` and press `A`.
   3. Select `Start InputRedirection`, press `A`. When `Starting InputRedirection... OK` appears, press `B` to return. The 3DS IP address will be shown in the upper-right corner of the bottom screen.
4. Launch this app, enter the 3DS IP, and tap **Save**. The connected game-pad should now control your 3DS.

### Screen-Off Feature (non-lock)

Due to Android system limitations, the app cannot receive gamepad events in the background after the screen is locked. Therefore, this app includes a screen-off feature that turns off the screen while keeping the app in the foreground, allowing you to continue controlling your 3DS while saving power.

**One-time setup (requires ADB or [LADB](https://play.google.com/store/apps/details?id=com.draco.ladb)):**

[Video tutorial](https://b23.tv/S9HrO2k)

If the service is not running when you tap the **ScreenOff** button, the app will copy the command to your clipboard and show a prompt:

```
sh /sdcard/Android/data/com.jingrong.inputredirectionclient_android/files/scrctl.sh
```

- **LADB**: Paste into the `Shell command` input box and press Enter.
- **Computer (ADB)**: Requires ADB environment. Prefix with `adb shell`.

The script prints step-by-step logs with `[ScrCtl]` prefix. On failure, Java crash details are shown for troubleshooting.

After running this script once, the screen-off service starts in the background. It will need to be restarted after a device reboot.

**How to use:**

- Tap **ScreenOff** to turn off the screen. The screen turns off but the device stays unlocked, so the 3DS connection remains active.
- To wake the screen, press either **Volume Up** or **Volume Down**. Do NOT use the Power button — it will lock the device, which disconnects the gamepad and interrupts the 3DS connection.

## Key Mapping

### Default Mapping

| Physical Button | 3DS Button |
|:-:|:-:|
| A / B | A / B (swappable) |
| X / Y | X / Y (swappable) |
| L / R / ZL / ZR | L / R / ZL / ZR |
| D-Pad | +Control Pad (auto-detected for Joy-Con / Xbox / Pro Controller) |
| Left Stick | Circle Pad |
| Right Stick | C-Stick |
| HOME button | HOME (disabled by default) |
| SHARE / Capture button | Power short press (disabled by default) |
| L3 + R3 | Power long press / shutdown (disabled by default) |

### Key Mapping Modes

The key mapping area has three tabs:

- **Simple** — A⇌B / X⇌Y swap switches plus stick swap.
- **Custom** — Fully remap any physical button to any 3DS key. Tap any button slot to enter key capture mode, then press the desired physical key. Conflicts are detected and flagged, and special keys (HOME/POWER/POWEROFF) cannot be reassigned.
- **Special** — Enable or disable HOME / POWER / SHUTDOWN mappings with individual switches.

Controller type (Xbox / Joy-Con / Pro Controller) is auto-detected from D-pad inputs and adapts mappings automatically.

### Turbo

Turbo (rapid fire) can be enabled per key (A/B/X/Y/L/R/ZL/ZR) in semi-auto or full-auto mode. The turbo interval and per-key settings are configurable. The "Disable All" button turns off all turbo at once.

## Q&A

1. **Cannot run in background or while screen is locked.**
   - The app must stay in the foreground. Use the built-in **ScreenOff** feature to save power without locking.
2. **ABXY button mapping may be incorrect.**
   - Enable "Swap A<->B" or "Swap X<->Y" in the settings.
3. **First-gen NS Joy-Con responds slowly.**
   - Rename your Android device to `Nintendo` in Bluetooth settings.
4. **Screen-off service must be restarted after reboot.**
   - Run the setup command again via LADB or ADB after restarting your device.
5. **3DS shows `Starting InputRedirection... failed`.**
   - Make sure your 3DS is connected to Wi-Fi and try again.


# InputRedirectionClient-Android

[中文版](./README.zh_CN.md)

Android port of the [InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) project.
Use your Android device as a wireless game-pad for Nintendo 3DS running Luma3DS.

## Requirements

1. Android 13+
2. 3DS with Luma3DS 8.0+

## Usage

### Basic Setup

1. Make sure the Android device and the 3DS are on the same LAN. The 3DS can also connect directly to the Android device's Wi-Fi hotspot.
2. Connect a game-pad to the Android device.
   > **Note**: For NS Joy-Con, rename your Android device to `Nintendo` in Bluetooth settings to reduce latency.
3. Start InputRedirection on the 3DS:
   1. Open the Rosalina menu with `L + Select + D-Pad Down`.
   2. Choose `Miscellaneous options...` and press `A`.
   3. Select `Start InputRedirection`, press `A`. When `Starting InputRedirection... OK` appears, press `B` to return. The 3DS IP address will be shown in the upper-right corner of the bottom screen.
4. Launch this app, enter the 3DS IP, and tap **Save**. The connected game-pad should now control your 3DS.

### Screen-Off Feature (V1.0)

The app can turn off the screen without locking the device, so 3DS input redirection continues while saving power.

**One-time setup (requires ADB or [LADB](https://play.google.com/store/apps/details?id=com.draco.ladb)):**

When you first tap the **ScreenOff** button, the app will prompt you with a command to run. You can also find the script at:

```
sh /sdcard/Android/data/com.jingrong.inputredirectionclient_android/files/scrctl.sh
```

- **LADB**: paste and run directly.
- **Computer (ADB)**: prefix with `adb shell`.

After running this script once, the screen-off service starts in the background. It will need to be restarted after a device reboot.

**How to use:**
- Tap **ScreenOff** to turn off the screen.
- Press any **Volume key** to wake the screen. Do NOT use the Power button, as that will lock the device.

## Key Mapping

| Physical Button | 3DS Button |
|:-:|:-:|
| A / B | A / B (swappable) |
| X / Y | X / Y (swappable) |
| L / R / ZL / ZR | L / R / ZL / ZR |
| D-Pad / Left Stick | Circle Pad |
| Right Stick | C-Stick |
| HOME button | HOME |
| SHARE button | Power (short press) |
| L3 + R3 | Power (long press / shutdown) |

## Q&A

1. **Cannot run in background or while screen is locked.**
   - The app must stay in the foreground. Use the built-in **ScreenOff** feature to save power without locking.
2. **ABXY button mapping may be incorrect.**
   - Enable "Swap A⇌B" or "Swap X⇌Y" in the settings.
3. **First-gen NS Joy-Con responds slowly.**
   - Rename your Android device to `Nintendo` in Bluetooth settings.
4. **Screen-off service must be restarted after reboot.**
   - Run the setup command again via LADB or ADB after restarting your device.
5. **3DS shows `Starting InputRedirection... failed`.**
   - Make sure your 3DS is connected to Wi-Fi and try again.

## Credits

- Luma3DS: [Luma3DS](https://github.com/LumaTeam/Luma3DS)
- Original Qt project: [InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) by TuxSH

# InputRedirectionClient-Android
[中文版](./README.zh_CN.md)

Android port of the [InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) project.

#### Requirements
1. Android 13+
2. 3DS with Luma3DS 8.0+

#### Usage
1. Make sure the Android device and the 3DS are on the same LAN; the 3DS can also connect to the Android device’s Wi-Fi hotspot directly.
2. Connect a game-pad to the Android device.
   > **Note**: If you are using NS Joy-Con, it is recommended to rename your Android device to `Nintendo` in the Bluetooth settings.
3. Start the InputRedirection service on the 3DS:
    1. Open the Rosalina menu with `L + Select + D-Pad Down`.
    2. Choose `Miscellaneous options...` and press `A`.
    3. Select `Start InputRedirection`, press `A`; when `Starting InputRedirection... OK` appears, press `B` to return.  
       The 3DS IP address will now be shown in the upper-right corner of the bottom screen, remember this IP address.
4. Launch this app on the Android device, enter the 3DS IP you just noted into the field under `3DS IP`, and tap **Save**.  
   Now, the connected game-pad should control your 3DS.
   > **Note**: Game-pad buttons may not map 1-to-1 to the 3DS buttons.

#### Known Issues & Fixes

1. Cannot run in the background or while the screen is locked.
    * Due to system limitations, the app only works while in the foreground.  
      To save power, use the [ScreenOff](https://github.com/WuDi-ZhanShen/ScreenOff/releases/tag/V18) app to turn off the screen without locking the device.
2. ABXY buttons are mapped incorrectly.
    * ~~A future update will add A↔B and X↔Y swap options.~~
    * Enable “Swap A⇌B” or “Swap X⇌Y” in the settings.
3. First-gen NS Joy-Con responds slowly.
    * Rename your Android device to `Nintendo` in Bluetooth settings.
4. 3DS shows `Starting InputRedirection... failed (0xffffffff).`
    * Make sure your 3DS is connected to Wi-Fi and try again.

#### Planned Features
1. ~~A↔B and X↔Y button swap.~~  **Done**
2. ~~Turbo (rapid-fire) support for ABXY, etc.~~  **Done**
3. Turn off screen without locking.

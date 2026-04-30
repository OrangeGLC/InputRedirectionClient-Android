# InputRedirectionClient-Android

[English](./README.md)

[InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) 项目的 Android 移植版本。
使用 Android 设备作为 Nintendo 3DS 的无线手柄。

## 运行要求

1. Android 13+
2. 3DS 安装 Luma3DS 8.0+

## 使用说明

### 基本设置

1. Android 设备与 3DS 处于同一局域网内，也可以让 3DS 直接连接 Android 设备的 Wi-Fi 热点。
2. 将手柄连接到 Android 设备。
   > **注意**：如果使用 NS Joy-Con，建议在蓝牙设置中将设备名称改为 `Nintendo`，可减少延迟。
3. 在 3DS 上启动 InputRedirection 服务：
   1. 打开 Rosalina 菜单（`L + Select + 十字键下`）。
   2. 选择 `Miscellaneous options...` 并按 `A` 进入。
   3. 选择 `Start InputRedirection` 并按 `A` 启动。显示 `Starting InputRedirection... OK` 后按 `B` 返回，下屏右上角会显示 3DS 的 IP 地址。
4. 在本 APP 中输入 3DS 的 IP 地址，点击**保存**。连接的手柄即可操控 3DS。

### 熄屏功能（V1.0）

APP 内置熄屏功能，可在关闭屏幕的同时保持前台运行继续操控 3DS，节省电量。

**首次配置（需要 ADB 或 [LADB](https://play.google.com/store/apps/details?id=com.draco.ladb)）：**

首次点击**熄屏**按钮时，APP 会弹出提示并将命令复制到剪贴板：

```
sh /sdcard/Android/data/com.jingrong.inputredirectionclient_android/files/scrctl.sh
```

- **LADB**：直接粘贴执行。
- **电脑（ADB）**：在命令前加 `adb shell`。

执行一次后，熄屏服务会在后台运行。设备重启后需要重新执行。

**使用方法：**
- 点击**熄屏**关闭屏幕。
- 按任意**音量键**唤醒屏幕。请勿使用电源键，电源键会导致锁屏并中断连接。

## 按键映射

| 物理按键 | 3DS 按键 |
|:-:|:-:|
| A / B | A / B（可交换）|
| X / Y | X / Y（可交换）|
| L / R / ZL / ZR | L / R / ZL / ZR |
| 十字键 / 左摇杆 | 滑杆 |
| 右摇杆 | C-Stick |
| HOME 键 | HOME |
| SHARE 键 | 电源（短按）|
| L3 + R3 | 关机（长按电源）|

## Q&A

1. **无法后台运行或锁屏运行。**
   - APP 必须保持前台。可使用内置**熄屏**功能来省电而不锁屏。
2. **ABXY 按键映射错误。**
   - 在设置中开启 A⇌B 或 X⇌Y 映射交换。
3. **一代 NS Joy-Con 响应迟缓。**
   - 在蓝牙设置中将设备名称改为 `Nintendo`。
4. **设备重启后熄屏服务需重新启动。**
   - 通过 LADB 或 ADB 重新执行一次配置命令即可。
5. **3DS 提示 `Starting InputRedirection... failed`。**
   - 确认 3DS 已连接 Wi-Fi 后重试。

## 致谢

- Luma3DS: [Luma3DS](https://github.com/LumaTeam/Luma3DS)
- 原版 Qt 项目：[InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) by TuxSH

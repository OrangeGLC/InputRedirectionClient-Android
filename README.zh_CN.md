# 手柄热点

[English](./README.md)

[InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt) 项目的 Android 移植版本。
使用 Android 设备作为 Nintendo 3DS 的无线手柄。

## 运行要求

1. Android 11+
2. 3DS 安装 Luma3DS 8.0+

## 使用说明

### 基本设置

[视频教程](https://b23.tv/IBf9KgU)

1. Android 设备与 3DS 处于同一局域网内，也可以让 3DS 直接连接 Android 设备的 Wi-Fi 热点。
2. 将手柄连接到 Android 设备。
   > **注意**：如果使用 NS Joy-Con，建议在蓝牙设置中将设备名称改为 `Nintendo`，可减少延迟。
3. 在 3DS 上启动 InputRedirection 服务：
   1. 打开 Rosalina 菜单（`L + Select + 十字键下`）。
   2. 选择 `Miscellaneous options...` 并按 `A` 进入。
   3. 选择 `Start InputRedirection` 并按 `A` 启动。显示 `Starting InputRedirection... OK` 后按 `B` 返回，下屏右上角会显示 3DS 的 IP 地址。
4. 在本 APP 中输入 3DS 的 IP 地址，点击**保存**。连接的手柄即可操控 3DS。

### 熄屏功能（非锁屏）

由于Android系统限制，锁屏后App无法在后台接收手柄事件，因此本App开发了熄屏功能，可在关闭屏幕的同时保持前台运行继续操控 3DS，节省电量。

**首次配置（需要 ADB 或 [LADB](https://play.google.com/store/apps/details?id=com.draco.ladb)）：**

[视频教程](https://b23.tv/S9HrO2k)

若服务未运行，点击**熄屏**按钮时 APP 会自动将命令复制到剪贴板并弹出提示：

```
sh /sdcard/Android/data/com.jingrong.inputredirectionclient_android/files/scrctl.sh
```

- **LADB**：粘贴到`Shell command`输入框，按回车键执行。
- **电脑（ADB）**：需要ADB环境，在命令前加 `adb shell`。

脚本执行时会输出 `[ScrCtl]` 标记的分步日志，启动失败时自动打印 Java 崩溃信息方便排查。

执行一次后，熄屏服务会在后台运行。设备重启后需要重新执行。

**使用方法：**

- 点击**熄屏**关闭屏幕。屏幕关闭但设备不锁屏，3DS 操控连接保持不断。
- 按**音量加**或**音量减**任一键即可唤醒屏幕。切勿使用电源键——电源键会锁屏，导致手柄断开、3DS 连接中断。

## 按键映射

### 默认映射

| 物理按键 | 3DS 按键 |
|:-:|:-:|
| A / B | A / B（可交换）|
| X / Y | X / Y（可交换）|
| L / R / ZL / ZR | L / R / ZL / ZR |
| 十字键 | 十字键（自动识别 Joy-Con / Xbox / Pro 手柄）|
| 左摇杆 | 滑杆 |
| 右摇杆 | C-Stick |
| HOME 键 | HOME（默认关闭）|
| 分享/截图键 | 电源短按（默认关闭）|
| L3 + R3 | 长按关机（默认关闭）|

### 映射模式

按键映射区域提供三个标签页：

- **简单** — A⇌B / X⇌Y 互换开关 + 摇杆交换。
- **自定义** — 将任意物理按键映射到任意 3DS 按键。点击按键槽进入捕获模式，再按下目标物理键即可完成映射。冲突自动检测并提示，特殊按键（HOME/POWER/POWEROFF）不可被重新映射。
- **特殊** — 通过独立开关启用/禁用 HOME / POWER / SHUTDOWN 映射。

手柄类型（Xbox / Joy-Con / Pro Controller）根据十字键输入自动识别并适配按键布局。

### 连发

支持为 A/B/X/Y/L/R/ZL/ZR 八个键单独开启连发，分半自动和全自动两种模式。可调节连发间隔，也可一键关闭全部连发。

## Q&A

1. **无法后台运行或锁屏运行。**
   - APP 必须保持前台。可使用内置**熄屏**功能来省电而不锁屏。
2. **ABXY 按键映射错误。**
   - 在设置中开启 A<->B 或 X<->Y 映射交换。
3. **一代 NS Joy-Con 响应迟缓。**
   - 在蓝牙设置中将设备名称改为 `Nintendo`。
4. **设备重启后熄屏服务需重新启动。**
   - 通过 LADB 或 ADB 重新执行一次配置命令即可。
5. **3DS 提示 `Starting InputRedirection... failed`。**
   - 确认 3DS 已连接 Wi-Fi 后重试。


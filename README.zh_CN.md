# InputRedirectionClient-Android
[English](./README.md)

[InputRedirectionClient-Qt](https://github.com/TuxSH/InputRedirectionClient-Qt)项目的Android移植版本。


#### 运行要求
1. Android 13+


#### 使用说明
1. Android设备与3DS处于同一局域网内，可以使用3DS直接连接Android设备的WI-FI热点。
2. 手柄连接Android设备。
   > **注意**：如果你使用的手柄是NS Joy-con，建议在Android设备的蓝牙设置中修改设备名称为`Nintendo`。
3. 3DS端打开InputRedirection服务:
    1. 打开Rosalina菜单`L+Select+十字键下`。
    2. 选择`Miscellaneous options...`, 按`A`进入。
    3. 选择`Start InputRedirection`, 按`A`启动，显示`Starting InputRedirection... OK`即开启成功，按`B`返回后即可在下屏右上角看到3ds的`IP地址`，记住这个`IP地址`。
1. Android设备上打开此App，在`3DS IP:`下方的输入栏填写上一步记录的3DS IP地址，并点击保存。现在连接Android设备的手柄应该可以正常操作3DS了。
   > **注意**：手柄按键与3DS按键映射可能不一致。

#### 已知问题与解决方案

1.  无法后台or锁屏运行。
    * 受系统限制，APP只有前台运行时才能正常工作。如果想省电的话，可以使用 [ScreenOff](https://github.com/WuDi-ZhanShen/ScreenOff/releases/tag/V18) APP来关闭屏幕但不锁屏。
2.  ABXY按键映射错误。
    * ~~后续增加AB按键反转和XY按键反转功能。~~
    * 开启A⇌B或X⇌Y映射交换
3.  连接NS1代Joy-con手柄响应迟缓。
    * 在Android设备的蓝牙设置中修改设备名称为`Nintendo`。
4.  3DS端打开InputRedirection服务显示`Starting InputRedirection... failed (0xffffffff).`
    * 确认3DS连接到WI-FI后重试。

#### 计划新增的功能
1. ~~A<->B X<->Y按键反转。~~ 已支持
2. ~~ABXY等按键支持连发。~~ 已支持
3. 关闭屏幕但不锁屏。


# Getting started with Sequana BLE sensors demo on mbed OS

This guide reviews the steps required to get example program working on Sequana platform.

Please install [mbed CLI](https://github.com/ARMmbed/mbed-cli#installing-mbed-cli).

## Import the example application

From the command-line, import the example:

```
mbed import https://github.com/lrusinowicz/sequana-ble-sensors-demo
cd sequana-ble-sensors-demo
```

### Now compile

Invoke `mbed compile`, and specify the name of your platform and your favorite toolchain (`GCC_ARM`, `ARM`, `IAR`). For example, for GCC:

```
mbed target FUTURE_SEQUANA
mbed toolchain GCC_ARM
mbed compile
```

Your PC may take a few minutes to compile your code. At the end, you see the following result:

```
[snip]
| Module                         |      .text |    .data |      .bss |
|--------------------------------|------------|----------|-----------|
| [fill]                         |    453(+0) |   14(+0) |    71(+0) |
| [lib]\c.a                      |  29139(+0) | 2472(+0) |    89(+0) |
| [lib]\gcc.a                    |   3168(+0) |    0(+0) |     0(+0) |
| [lib]\misc                     |    252(+0) |   16(+0) |    28(+0) |
| [lib]\stdc++.a                 |      1(+0) |    0(+0) |     0(+0) |
| [misc]                         |     48(+0) |   56(+0) |     0(+0) |
| mbed-os\cmsis                  |   1033(+0) |    0(+0) |    84(+0) |
| mbed-os\drivers                |   4527(+0) |    0(+0) |   104(+0) |
| mbed-os\events                 |   1202(+0) |    0(+0) |     0(+0) |
| mbed-os\features               | 101297(+0) |  191(+0) |  6175(+0) |
| mbed-os\hal                    |   2308(+0) |    4(+0) |    68(+0) |
| mbed-os\platform               |   5730(+0) |  260(+0) |   274(+0) |
| mbed-os\rtos                   |   9673(+0) |  168(+0) |  5973(+0) |
| mbed-os\targets                |  19282(+0) | 1009(+0) |  1725(+0) |
| source\AirQSensor.o            |    262(+0) |    0(+0) |     0(+0) |
| source\As7261Driver.o          |    430(+0) |    0(+0) |     0(+0) |
| source\ComboEnvSensor.o        |    282(+0) |    0(+0) |     0(+0) |
| source\Hs3001Driver.o          |    156(+0) |    0(+0) |     5(+0) |
| source\Kx64.o                  |    846(+0) |    0(+0) |     0(+0) |
| source\SequanaPrimaryService.o |   2002(+0) |   36(+0) |   140(+0) |
| source\Sps30.o                 |   1194(+0) |   14(+0) |     0(+0) |
| source\Zmod44xxDriver.o        |     20(+0) |    0(+0) |     0(+0) |
| source\main.o                  |   2157(+0) |    4(+0) |  1552(+0) |
| Subtotals                      | 185462(+0) | 4244(+0) | 16288(+0) |
Total Static RAM memory (data + bss): 20532(+0) bytes
Total Flash memory (text + data): 189706(+0) bytes

Image: .\BUILD\FUTURE_SEQUANA\GCC_ARM\sequana-ble-sensors-demo.hex
```

### Program your board

1. Connect your Sequana board to the computer over USB.
2. Copy the hex file to the mbed device (DAPLINK removable storage device).
3. Your program will start automatically after programming has finished.

You can use 'Future Sequana' Android/IOS application to view sensor's data on a phone/tablet.

## Troubleshooting

1. Make sure `mbed-cli` is working correctly and its version is `>=1.8.2`

 ```
 mbed --version
 ```

 If not, you can update it:

 ```
 pip install mbed-cli --upgrade
 ```

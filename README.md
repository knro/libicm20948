# libicm20948

A Linux shared library for the **InvenSense ICM20948** 9-axis IMU (3-axis accelerometer + 3-axis gyroscope + 3-axis magnetometer via on-chip AK09916).

Adapted from the [cybergear-robotics/icm20948](https://github.com/cybergear-robotics/icm20948) ESP-IDF component, ported to Linux using native kernel I2C (`/dev/i2c-X`) and SPI (`/dev/spidev`) interfaces.

---

## Features

- Full C++ wrapper class (`ICM20948`) with I2C and SPI support
- Linux-native transport (kernel `I2C_RDWR` ioctl, `spidev`)
- Accelerometer, gyroscope, magnetometer (AK09916) and temperature readings
- Scaled output in m/s², deg/s, µT, and °C
- Optional DMP firmware support (build with `-DICM20948_USE_DMP=ON`)
- Optional GPIO hardware reset via libgpiod
- CMake package config + pkg-config support
- Debian packaging

---

## Dependencies

- **libgpiod** (v1.x or v2.x) — GPIO reset pin support
- Linux kernel headers — I2C and SPI ioctl interfaces
- CMake 3.10+, a C11/C++17 compiler

```bash
# Debian/Ubuntu
sudo apt install libgpiod-dev libgpiod2 cmake build-essential
```

---

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

To enable DMP support:

```bash
cmake -DICM20948_USE_DMP=ON ..
```

---

## Install

```bash
sudo make install
sudo ldconfig
```

---

## Quick Start

```cpp
#include <ICM20948.h>

int main()
{
    ICM20948 imu;

    // Initialize via I2C (AD0 pin low → 0x68, AD0 pin high → 0x69)
    imu.begin_i2c(ICM_20948_I2C_ADDR_AD0, "/dev/i2c-1");

    while (true)
    {
        if (imu.readSensor() == ICM_20948_STAT_OK)
        {
            printf("Ax=%6.3f Ay=%6.3f Az=%6.3f m/s²\n",
                imu.getAccelX_mss(), imu.getAccelY_mss(), imu.getAccelZ_mss());
            printf("Gx=%6.3f Gy=%6.3f Gz=%6.3f °/s\n",
                imu.getGyroX_dps(), imu.getGyroY_dps(), imu.getGyroZ_dps());
            printf("Mx=%6.3f My=%6.3f Mz=%6.3f µT\n",
                imu.getMagX_uT(), imu.getMagY_uT(), imu.getMagZ_uT());
            printf("T=%.2f°C\n", imu.getTemp_C());
        }
        usleep(10000); // 100 Hz
    }
}
```

## I2C Addresses

| AD0 Pin | Address |
|---------|---------|
| Low (GND) | 0x68 (`ICM_20948_I2C_ADDR_AD0`) |
| High (VCC) | 0x69 (`ICM_20948_I2C_ADDR_AD1`) |

---

## License

The Linux wrapper class and build system (`ICM20948.h`, `ICM20948.cpp`,
`icm20948_i2c.*`, `icm20948_spi.*`, `CMakeLists.txt`, etc.) are licensed under
the **Apache License 2.0** — Copyright 2025 Jasem Mutlaq, Ikarus Technologies.

The core ICM20948 C library (all other `src/icm20948_*.c/h` files) is adapted from
[cybergear-robotics/icm20948](https://github.com/cybergear-robotics/icm20948),
originally derived from the SparkFun ICM-20948 Arduino library, and is licensed
under the **MIT License** — Copyright (c) 2016 SparkFun Electronics, 2024 Johannes Fischer.

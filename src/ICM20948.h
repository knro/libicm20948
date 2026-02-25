/*
 * Copyright 2025, Jasem Mutlaq, Ikarus Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "icm20948.h"
#include "icm20948_i2c.h"
#include "icm20948_spi.h"

#include <stdexcept>
#include <string>
#include <cstdint>

/**
 * @brief Exception class thrown on ICM20948 initialization or communication errors.
 */
class ICM20948_exception : public std::runtime_error
{
    public:
        explicit ICM20948_exception(const std::string &message) : std::runtime_error(message) {}
};

/**
 * @brief C++ wrapper for the ICM20948 9-axis IMU (Accel + Gyro + Mag).
 *
 * Wraps the C-language icm20948 core library with a Linux-native transport
 * layer (I2C via /dev/i2c-X or SPI via /dev/spidevX.X).
 *
 * Typical usage:
 * @code
 *   ICM20948 imu;
 *   imu.begin_i2c(ICM_20948_I2C_ADDR_AD0, "/dev/i2c-1");
 *   icm20948_agmt_t agmt;
 *   while(true) {
 *       if (imu.readSensor(&agmt) == ICM_20948_STAT_OK) {
 *           printf("Ax=%f Ay=%f Az=%f\n",
 *               imu.getAccelX_mss(), imu.getAccelY_mss(), imu.getAccelZ_mss());
 *       }
 *   }
 * @endcode
 */
class ICM20948
{
    public:
        /**
         * @brief Construct a new ICM20948 object.
         * @param reset_pin  GPIO chip line number for the hardware reset pin, or -1 if unused.
         */
        explicit ICM20948(int reset_pin = -1);

        /**
         * @brief Destroy the ICM20948 object and release all resources.
         */
        ~ICM20948();

        /**
         * @brief Initialize with Linux I2C using a device path.
         * @param i2c_addr  7-bit I2C address (ICM_20948_I2C_ADDR_AD0 = 0x68 or ICM_20948_I2C_ADDR_AD1 = 0x69).
         * @param i2c_bus   Path to the I2C bus device (e.g. "/dev/i2c-1").
         * @throws ICM20948_exception on failure.
         */
        void begin_i2c(uint8_t i2c_addr = ICM_20948_I2C_ADDR_AD0,
                       const char *i2c_bus = "/dev/i2c-1");

        /**
         * @brief Initialize with Linux I2C using an externally opened file descriptor.
         * @param devFd     Open file descriptor for the I2C bus.
         * @param i2c_addr  7-bit I2C address of the ICM20948.
         * @throws ICM20948_exception on failure.
         */
        void begin_i2c(int devFd, uint8_t i2c_addr = ICM_20948_I2C_ADDR_AD0);

        /**
         * @brief Initialize with Linux SPI.
         * @param spi_device  Path to the SPI device (e.g. "/dev/spidev0.0").
         * @param speed_hz    SPI clock speed in Hz (default: 7 MHz).
         * @throws ICM20948_exception on failure.
         */
        void begin_spi(const char *spi_device = "/dev/spidev0.0",
                       uint32_t speed_hz = 7000000);

        /**
         * @brief Perform a hardware reset via the GPIO reset pin (if configured).
         */
        void hardwareReset();

        /**
         * @brief Read all sensor data (accel, gyro, mag, temp) into the internal buffer.
         * @param agmt  Optional output pointer; if non-null, the result is also written here.
         * @return ICM_20948_STAT_OK on success, error code otherwise.
         */
        icm20948_status_e readSensor(icm20948_agmt_t *agmt = nullptr);

        /**
         * @brief Return the most recently read accelerometer X in m/s².
         */
        [[nodiscard]] float getAccelX_mss() const;
        /**
         * @brief Return the most recently read accelerometer Y in m/s².
         */
        [[nodiscard]] float getAccelY_mss() const;
        /**
         * @brief Return the most recently read accelerometer Z in m/s².
         */
        [[nodiscard]] float getAccelZ_mss() const;

        /**
         * @brief Return the most recently read gyroscope X in degrees/second.
         */
        [[nodiscard]] float getGyroX_dps() const;
        /**
         * @brief Return the most recently read gyroscope Y in degrees/second.
         */
        [[nodiscard]] float getGyroY_dps() const;
        /**
         * @brief Return the most recently read gyroscope Z in degrees/second.
         */
        [[nodiscard]] float getGyroZ_dps() const;

        /**
         * @brief Return the most recently read magnetometer X in µT.
         */
        [[nodiscard]] float getMagX_uT() const;
        /**
         * @brief Return the most recently read magnetometer Y in µT.
         */
        [[nodiscard]] float getMagY_uT() const;
        /**
         * @brief Return the most recently read magnetometer Z in µT.
         */
        [[nodiscard]] float getMagZ_uT() const;

        /**
         * @brief Return the most recently read temperature in degrees Celsius.
         */
        [[nodiscard]] float getTemp_C() const;

        /**
         * @brief Raw sensor data from the last successful readSensor() call.
         */
        icm20948_agmt_t agmt;

        /**
         * @brief Underlying C device structure for direct low-level access.
         */
        icm20948_device_t device;

    private:
        enum comm_mode
        {
            MODE_I2C,
            MODE_SPI
        };

        int m_ResetPin;
        int m_DevFd;
        bool m_OwnsFd;   // true if this instance opened the fd and must close it
        comm_mode m_Mode;

        icm20948_config_i2c_t m_I2cConfig;
        icm20948_config_spi_t m_SpiConfig;

        /**
         * @brief Common sensor initialization sequence (reset, WHO_AM_I, defaults).
         * Called after the transport layer is configured.
         */
        void _init();

        /**
         * @brief Convert accelerometer full-scale setting to LSB/g.
         */
        static float accelFssToLsbPerG(uint8_t fss);

        /**
         * @brief Convert gyroscope full-scale setting to LSB/(deg/s).
         */
        static float gyroFssToLsbPerDps(uint8_t fss);
};

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

#include "ICM20948.h"
#include "ak09916_registers.h"
#include "config.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <gpiod.hpp>

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────

static void gpio_set_value(int pin, int value)
{
#ifdef HAVE_LIBGPIOD_V2
    gpiod::chip chip("/dev/gpiochip0");
    auto request = chip.prepare_request()
                   .set_consumer("ICM20948")
                   .add_line_settings(pin,
                                      gpiod::line_settings()
                                      .set_direction(gpiod::line::direction::OUTPUT)
                                      .set_output_value(static_cast<gpiod::line::value>(value)))
                   .do_request();
    usleep(10000);
    request.release();
#else
    gpiod::chip chip("/dev/gpiochip0", gpiod::chip::OPEN_BY_PATH);
    gpiod::line line = chip.get_line(pin);
    line.request({ "icm20948-reset", gpiod::line_request::DIRECTION_OUTPUT }, value);
    usleep(10000);
    line.release();
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────────────────────────────────────

ICM20948::ICM20948(int reset_pin)
    : m_ResetPin(reset_pin), m_DevFd(-1), m_OwnsFd(false),
      m_Mode(MODE_I2C)
{
    memset(&agmt,   0, sizeof(agmt));
    memset(&device, 0, sizeof(device));
}

ICM20948::~ICM20948()
{
    if (m_OwnsFd && m_DevFd >= 0)
    {
        close(m_DevFd);
        m_DevFd = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: begin_i2c (open by path)
// ──────────────────────────────────────────────────────────────────────────────

void ICM20948::begin_i2c(uint8_t i2c_addr, const char *i2c_bus)
{
    m_Mode = MODE_I2C;

    int fd = open(i2c_bus, O_RDWR);
    if (fd < 0)
    {
        throw ICM20948_exception(
            std::string("Failed to open I2C bus: ") + i2c_bus +
            " (" + strerror(errno) + ")");
    }

    m_DevFd  = fd;
    m_OwnsFd = true;

    m_I2cConfig.fd       = fd;
    m_I2cConfig.i2c_addr = i2c_addr;

    icm20948_init_i2c(&device, &m_I2cConfig);

    _init();
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: begin_i2c (use external fd)
// ──────────────────────────────────────────────────────────────────────────────

void ICM20948::begin_i2c(int devFd, uint8_t i2c_addr)
{
    m_Mode   = MODE_I2C;
    m_DevFd  = devFd;
    m_OwnsFd = false;

    m_I2cConfig.fd       = devFd;
    m_I2cConfig.i2c_addr = i2c_addr;

    icm20948_init_i2c(&device, &m_I2cConfig);

    _init();
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: begin_spi
// ──────────────────────────────────────────────────────────────────────────────

void ICM20948::begin_spi(const char *spi_device, uint32_t speed_hz)
{
    m_Mode = MODE_SPI;

    int fd = open(spi_device, O_RDWR);
    if (fd < 0)
    {
        throw ICM20948_exception(
            std::string("Failed to open SPI device: ") + spi_device +
            " (" + strerror(errno) + ")");
    }

    /* SPI mode 3 (CPOL=1, CPHA=1) as required by ICM20948 */
    uint8_t mode = SPI_MODE_3;
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
    {
        close(fd);
        throw ICM20948_exception("Failed to set SPI mode 3.");
    }

    uint8_t bpw = 8;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0)
    {
        close(fd);
        throw ICM20948_exception("Failed to set SPI bits-per-word.");
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0)
    {
        close(fd);
        throw ICM20948_exception("Failed to set SPI max speed.");
    }

    m_DevFd  = fd;
    m_OwnsFd = true;

    m_SpiConfig.fd       = fd;
    m_SpiConfig.speed_hz = speed_hz;

    icm20948_init_spi(&device, &m_SpiConfig);

    _init();
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: hardwareReset
// ──────────────────────────────────────────────────────────────────────────────

void ICM20948::hardwareReset()
{
    if (m_ResetPin == -1)
        return;

    try
    {
        gpio_set_value(m_ResetPin, 1);
        gpio_set_value(m_ResetPin, 0);
        gpio_set_value(m_ResetPin, 1);
    }
    catch (const std::exception &e)
    {
        std::cerr << "ICM20948: GPIO reset failed: " << e.what() << std::endl;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: readSensor
// ──────────────────────────────────────────────────────────────────────────────

icm20948_status_e ICM20948::readSensor(icm20948_agmt_t *out)
{
    icm20948_status_e status = icm20948_get_agmt(&device, &agmt);
    if (out != nullptr)
        *out = agmt;
    return status;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: scaled getters
// ──────────────────────────────────────────────────────────────────────────────

float ICM20948::getAccelX_mss() const
{
    return (agmt.acc.axes.x / accelFssToLsbPerG(agmt.fss.a)) * 9.80665f;
}

float ICM20948::getAccelY_mss() const
{
    return (agmt.acc.axes.y / accelFssToLsbPerG(agmt.fss.a)) * 9.80665f;
}

float ICM20948::getAccelZ_mss() const
{
    return (agmt.acc.axes.z / accelFssToLsbPerG(agmt.fss.a)) * 9.80665f;
}

float ICM20948::getGyroX_dps() const
{
    return agmt.gyr.axes.x / gyroFssToLsbPerDps(agmt.fss.g);
}

float ICM20948::getGyroY_dps() const
{
    return agmt.gyr.axes.y / gyroFssToLsbPerDps(agmt.fss.g);
}

float ICM20948::getGyroZ_dps() const
{
    return agmt.gyr.axes.z / gyroFssToLsbPerDps(agmt.fss.g);
}

/* AK09916 magnetometer: fixed sensitivity of 0.15 µT/LSB */
float ICM20948::getMagX_uT() const
{
    return agmt.mag.axes.x * 0.15f;
}

float ICM20948::getMagY_uT() const
{
    return agmt.mag.axes.y * 0.15f;
}

float ICM20948::getMagZ_uT() const
{
    return agmt.mag.axes.z * 0.15f;
}

/* Temperature formula from ICM20948 datasheet:
 *   TEMP_degC = (TEMP_OUT / 333.87) + 21  */
float ICM20948::getTemp_C() const
{
    return (agmt.tmp.val / 333.87f) + 21.0f;
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: _init  –  common post-transport initialization sequence
// ──────────────────────────────────────────────────────────────────────────────

void ICM20948::_init()
{
    icm20948_status_e status;

    // Optional GPIO reset before software init
    hardwareReset();

    // Software reset
    status = icm20948_sw_reset(&device);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: software reset failed.");
    usleep(50000); // 50 ms – wait for reset to complete

    // Wake from sleep
    status = icm20948_sleep(&device, false);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to wake from sleep.");

    // Disable low-power mode
    status = icm20948_low_power(&device, false);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to disable low-power mode.");

    // Select best available clock source
    status = icm20948_set_clock_source(&device, CLOCK_AUTO);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to set clock source.");

    // Verify chip identity
    status = icm20948_check_id(&device);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: WHO_AM_I check failed – wrong chip or bad connection.");

    // Set accel + gyro to continuous sampling mode
    status = icm20948_set_sample_mode(
                 &device,
                 static_cast<icm20948_internal_sensor_id_bm>(ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR),
                 SAMPLE_MODE_CONTINUOUS);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to set sample mode.");

    // Full scale: ±2g accelerometer, ±250 dps gyroscope (default / most sensitive)
    icm20948_fss_t fss;
    fss.a          = 0; // 0 → ±2g   (16384 LSB/g)
    fss.g          = 0; // 0 → ±250 dps (131 LSB/dps)
    fss.reserved_0 = 0;
    status = icm20948_set_full_scale(
                 &device,
                 static_cast<icm20948_internal_sensor_id_bm>(ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR),
                 fss);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to set full-scale range.");

    // DLPF configuration – enable filtering on both sensors
    icm20948_dlpcfg_t dlp;
    dlp.a = 7; // accel:  ~246 Hz 3dB bandwidth
    dlp.g = 7; // gyro:   ~361 Hz 3dB bandwidth
    status = icm20948_set_dlpf_cfg(
                 &device,
                 static_cast<icm20948_internal_sensor_id_bm>(ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR),
                 dlp);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to configure DLPF.");

    status = icm20948_enable_dlpf(
                 &device,
                 static_cast<icm20948_internal_sensor_id_bm>(ICM_20948_INTERNAL_ACC | ICM_20948_INTERNAL_GYR),
                 true);
    if (status != ICM_20948_STAT_OK)
        throw ICM20948_exception("ICM20948: failed to enable DLPF.");

    // ── Magnetometer setup ────────────────────────────────────────────────────
    //
    // Per ICM-20948 datasheet §4.11-4.12:
    //   I2C host:  Use Pass-Through mode.  BYPASS_EN connects AUX_CL/AUX_DA
    //              directly to the host's SCL/SDA via an internal analog switch,
    //              so the host can configure the AK09916 (at 0x0C) just like any
    //              other I2C device.  Afterwards disable bypass and enable the
    //              ICM20948's internal I2C master to take over auto-reading.
    //   SPI host:  The bypass mux shares the SPI pins, so pass-through is
    //              unavailable.  Use Slave-4 (periph4) one-shot transactions.

    if (m_Mode == MODE_I2C)
    {
        // ── Pass-Through path (I2C host) ──────────────────────────────────────
        // Step 1: enable bypass mux (I2C_MST_EN must be 0 – reset default).
        status = icm20948_i2c_master_passthrough(&device, true);
        if (status != ICM_20948_STAT_OK)
            throw ICM20948_exception("ICM20948: failed to enable I2C passthrough.");

        usleep(5000); // 5 ms – let the analog switch settle

        // Step 2: soft-reset the AK09916 directly (CNTL3.SRST = 1).
        if (!_i2c_write_direct(MAG_AK09916_I2C_ADDR,
                               static_cast<uint8_t>(AK09916_REG_CNTL3), 0x01))
        {
            std::cerr << "ICM20948: warning – AK09916 soft-reset write failed." << std::endl;
        }
        usleep(1000); // 1 ms – AK09916 needs ~100 µs after SRST

        // Step 3: continuous measurement mode 4 (100 Hz).
        if (!_i2c_write_direct(MAG_AK09916_I2C_ADDR,
                               static_cast<uint8_t>(AK09916_REG_CNTL2), 0x08))
        {
            std::cerr << "ICM20948: warning – could not configure AK09916 magnetometer." << std::endl;
        }

        // Step 4: disable bypass – hand the aux bus back to the ICM20948.
        status = icm20948_i2c_master_passthrough(&device, false);
        if (status != ICM_20948_STAT_OK)
            throw ICM20948_exception("ICM20948: failed to disable I2C passthrough.");

        // Step 5: enable the ICM20948 internal I2C master.
        status = icm20948_i2c_master_enable(&device, true);
        if (status != ICM_20948_STAT_OK)
            throw ICM20948_exception("ICM20948: failed to enable I2C master.");

        usleep(10000); // 10 ms – let the internal master start up
    }
    else
    {
        // ── Periph4 path (SPI host) ───────────────────────────────────────────
        status = icm20948_i2c_master_passthrough(&device, false);
        if (status != ICM_20948_STAT_OK)
            throw ICM20948_exception("ICM20948: failed to disable I2C passthrough.");

        status = icm20948_i2c_master_enable(&device, true);
        if (status != ICM_20948_STAT_OK)
            throw ICM20948_exception("ICM20948: failed to enable I2C master.");

        usleep(10000);

        uint8_t ak_reset = 0x01;
        if (icm20948_i2c_master_single_w(&device, MAG_AK09916_I2C_ADDR,
                                          static_cast<uint8_t>(AK09916_REG_CNTL3),
                                          &ak_reset) != ICM_20948_STAT_OK)
            std::cerr << "ICM20948: warning – AK09916 soft-reset write failed." << std::endl;
        usleep(1000);

        uint8_t mag_mode = 0x08;
        if (icm20948_i2c_master_single_w(&device, MAG_AK09916_I2C_ADDR,
                                          static_cast<uint8_t>(AK09916_REG_CNTL2),
                                          &mag_mode) != ICM_20948_STAT_OK)
            std::cerr << "ICM20948: warning – could not configure AK09916 magnetometer." << std::endl;
    }

    // Configure peripheral 0 to automatically read 9 bytes from the AK09916
    // starting at ST1: ST1 + HXL + HXH + HYL + HYH + HZL + HZH + TMPS + ST2
    status = icm20948_i2c_controller_configure_peripheral(
                 &device,
                 0,                              // peripheral slot 0
                 MAG_AK09916_I2C_ADDR,
                 static_cast<uint8_t>(AK09916_REG_ST1),
                 9,                              // read 9 bytes
                 true,                           // read
                 true,                           // enable
                 false,                          // data_only = false
                 false,                          // grp = false
                 false,                          // swap = false
                 0);                             // dataOut (don't care for reads)
    if (status != ICM_20948_STAT_OK)
        std::cerr << "ICM20948: warning – could not configure magnetometer peripheral." << std::endl;
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: scale factor helpers
// ──────────────────────────────────────────────────────────────────────────────

float ICM20948::accelFssToLsbPerG(uint8_t fss)
{
    switch (fss)
    {
        case 0:
            return 16384.0f;  // ±2g
        case 1:
            return  8192.0f;  // ±4g
        case 2:
            return  4096.0f;  // ±8g
        case 3:
            return  2048.0f;  // ±16g
        default:
            return 16384.0f;
    }
}

float ICM20948::gyroFssToLsbPerDps(uint8_t fss)
{
    switch (fss)
    {
        case 0:
            return 131.0f;    // ±250 dps
        case 1:
            return  65.5f;    // ±500 dps
        case 2:
            return  32.8f;    // ±1000 dps
        case 3:
            return  16.4f;    // ±2000 dps
        default:
            return 131.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: _i2c_write_direct
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Write a single byte to any I2C device on the bus (I2C mode only).
 *
 * Used during magnetometer pass-through init: while BYPASS_EN is asserted the
 * AK09916 (address 0x0C) is electrically connected to the same SCL/SDA lines
 * as the ICM20948, so we can talk to it directly via the same file descriptor.
 */
bool ICM20948::_i2c_write_direct(uint8_t i2c_addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };

    struct i2c_msg msg;
    msg.addr  = i2c_addr;
    msg.flags = 0;          // write
    msg.len   = 2;
    msg.buf   = buf;

    struct i2c_rdwr_ioctl_data msgset;
    msgset.msgs  = &msg;
    msgset.nmsgs = 1;

    return (ioctl(m_I2cConfig.fd, I2C_RDWR, &msgset) >= 0);
}

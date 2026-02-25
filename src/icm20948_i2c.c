/*
 * Linux I2C transport layer for ICM20948
 * Replaces the ESP-IDF driver/i2c.h based implementation.
 *
 * Uses the Linux kernel I2C_RDWR ioctl to perform combined write-then-read
 * transactions (required for register-addressed reads).
 *
 * Licensed under the Apache License, Version 2.0
 */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#include "config.h"
#include "icm20948.h"
#include "icm20948_i2c.h"

/**
 * @brief Write one or more bytes to a register on the ICM20948 via Linux I2C.
 *
 * Performs a single I2C write transaction: [START | ADDR+W | reg | data... | STOP]
 */
icm20948_status_e icm20948_internal_write_i2c(uint8_t reg, uint8_t *data, uint32_t len, void *user)
{
    icm20948_config_i2c_t *cfg = (icm20948_config_i2c_t *)user;

    /* Build the write buffer: register address followed by data */
    uint8_t buf[len + 1];
    buf[0] = reg;
    memcpy(buf + 1, data, len);

    struct i2c_msg msg = {
        .addr  = cfg->i2c_addr,
        .flags = 0,
        .len   = (uint16_t)(len + 1),
        .buf   = buf,
    };

    struct i2c_rdwr_ioctl_data msgset = {
        .msgs  = &msg,
        .nmsgs = 1,
    };

    if (ioctl(cfg->fd, I2C_RDWR, &msgset) < 0)
    {
        return ICM_20948_STAT_ERR;
    }

    return ICM_20948_STAT_OK;
}

/**
 * @brief Read one or more bytes from a register on the ICM20948 via Linux I2C.
 *
 * Performs a combined I2C transaction:
 *   [START | ADDR+W | reg | REPEATED-START | ADDR+R | data... | STOP]
 */
icm20948_status_e icm20948_internal_read_i2c(uint8_t reg, uint8_t *buff, uint32_t len, void *user)
{
    icm20948_config_i2c_t *cfg = (icm20948_config_i2c_t *)user;

    /* Message 1: write the register address */
    struct i2c_msg msgs[2];
    msgs[0].addr  = cfg->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    /* Message 2: read the data */
    msgs[1].addr  = cfg->i2c_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = (uint16_t)len;
    msgs[1].buf   = buff;

    struct i2c_rdwr_ioctl_data msgset = {
        .msgs  = msgs,
        .nmsgs = 2,
    };

    if (ioctl(cfg->fd, I2C_RDWR, &msgset) < 0)
    {
        return ICM_20948_STAT_ERR;
    }

    return ICM_20948_STAT_OK;
}

/* Default serif for single-device I2C use */
static icm20948_serif_t default_i2c_serif = {
    icm20948_internal_write_i2c,
    icm20948_internal_read_i2c,
    NULL,
};

/**
 * @brief Initialize the ICM20948 device struct with the Linux I2C transport.
 */
void icm20948_init_i2c(icm20948_device_t *icm_device, icm20948_config_i2c_t *config)
{
    icm20948_init_struct(icm_device);
    default_i2c_serif.user = (void *)config;
    icm20948_link_serif(icm_device, &default_i2c_serif);

#ifdef ICM20948_USE_DMP
    icm_device->_dmp_firmware_available = true;
#else
    icm_device->_dmp_firmware_available = false;
#endif

    icm_device->_firmware_loaded     = false;
    icm_device->_last_bank            = 255;
    icm_device->_last_mems_bank       = 255;
    icm_device->_gyroSF               = 0;
    icm_device->_gyroSFpll            = 0;
    icm_device->_enabled_Android_0    = 0;
    icm_device->_enabled_Android_1    = 0;
    icm_device->_enabled_Android_intr_0 = 0;
    icm_device->_enabled_Android_intr_1 = 0;
}

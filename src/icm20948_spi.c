/*
 * Linux SPI transport layer for ICM20948
 * Replaces the ESP-IDF driver/spi_master.h based implementation.
 *
 * Uses the Linux kernel spidev ioctl interface (SPI_IOC_MESSAGE).
 * The ICM20948 SPI protocol:
 *   - Write: [reg & 0x7F] followed by data bytes
 *   - Read:  [reg | 0x80] followed by dummy bytes, data returned in rx
 *
 * Licensed under the Apache License, Version 2.0
 */

#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

#include "config.h"
#include "icm20948.h"
#include "icm20948_spi.h"

/**
 * @brief Write one or more bytes to a register on the ICM20948 via Linux SPI.
 */
icm20948_status_e icm20948_internal_write_spi(uint8_t reg, uint8_t *data, uint32_t len, void *user)
{
    icm20948_config_spi_t *cfg = (icm20948_config_spi_t *)user;

    uint32_t total = len + 1;
    uint8_t tx_buf[total];
    tx_buf[0] = (reg & 0x7F); /* MSB=0 → write */
    memcpy(tx_buf + 1, data, len);

    struct spi_ioc_transfer xfer = {
        .tx_buf        = (unsigned long)tx_buf,
        .rx_buf        = (unsigned long)NULL,
        .len           = total,
        .speed_hz      = cfg->speed_hz,
        .bits_per_word = 8,
        .cs_change     = 0,
    };

    if (ioctl(cfg->fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
    {
        return ICM_20948_STAT_ERR;
    }

    return ICM_20948_STAT_OK;
}

/**
 * @brief Read one or more bytes from a register on the ICM20948 via Linux SPI.
 */
icm20948_status_e icm20948_internal_read_spi(uint8_t reg, uint8_t *buff, uint32_t len, void *user)
{
    icm20948_config_spi_t *cfg = (icm20948_config_spi_t *)user;

    uint32_t total = len + 1;
    uint8_t tx_buf[total];
    uint8_t rx_buf[total];

    memset(tx_buf, 0x00, total);
    tx_buf[0] = (reg | 0x80); /* MSB=1 → read */

    struct spi_ioc_transfer xfer = {
        .tx_buf        = (unsigned long)tx_buf,
        .rx_buf        = (unsigned long)rx_buf,
        .len           = total,
        .speed_hz      = cfg->speed_hz,
        .bits_per_word = 8,
        .cs_change     = 0,
    };

    if (ioctl(cfg->fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
    {
        return ICM_20948_STAT_ERR;
    }

    /* Skip the first byte (clocked out while sending the register address) */
    memcpy(buff, rx_buf + 1, len);

    return ICM_20948_STAT_OK;
}

/* Default serif for single-device SPI use */
static icm20948_serif_t default_spi_serif = {
    icm20948_internal_write_spi,
    icm20948_internal_read_spi,
    NULL,
};

/**
 * @brief Initialize the ICM20948 device struct with the Linux SPI transport.
 */
void icm20948_init_spi(icm20948_device_t *icm_device, icm20948_config_spi_t *config)
{
    icm20948_init_struct(icm_device);
    default_spi_serif.user = (void *)config;
    icm20948_link_serif(icm_device, &default_spi_serif);

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

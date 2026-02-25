/*
 * Linux SPI transport layer for ICM20948
 * Replaces the ESP-IDF driver/spi_master.h based implementation.
 *
 * Licensed under the Apache License, Version 2.0
 */

#ifndef _ICM_20948_SPI_H_
#define _ICM_20948_SPI_H_

#include <stdint.h>
#include "icm20948.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for Linux SPI transport.
 */
typedef struct
{
    int fd;             /**< Open file descriptor for the SPI device (e.g. /dev/spidev0.0) */
    uint32_t speed_hz;  /**< SPI clock speed in Hz (default: 7000000) */
} icm20948_config_spi_t;

/**
 * @brief Initialize the ICM20948 device struct with a Linux SPI transport.
 *
 * @param device  Pointer to the device struct to initialize.
 * @param config  Pointer to the SPI configuration (fd + speed).
 */
void icm20948_init_spi(icm20948_device_t *device, icm20948_config_spi_t *config);

/* These functions are exposed to allow custom serif_t setup */
icm20948_status_e icm20948_internal_write_spi(uint8_t reg, uint8_t *data, uint32_t len, void *user);
icm20948_status_e icm20948_internal_read_spi(uint8_t reg, uint8_t *buff, uint32_t len, void *user);

#ifdef __cplusplus
}
#endif

#endif /* _ICM_20948_SPI_H_ */

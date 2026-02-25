/*
 * Linux I2C transport layer for ICM20948
 * Replaces the ESP-IDF driver/i2c.h based implementation.
 *
 * Licensed under the Apache License, Version 2.0
 */

#ifndef _ICM_20948_I2C_H_
#define _ICM_20948_I2C_H_

#include <stdint.h>
#include "icm20948.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for Linux I2C transport.
 */
typedef struct
{
    int fd;          /**< Open file descriptor for the I2C bus (e.g. /dev/i2c-1) */
    uint8_t i2c_addr; /**< 7-bit I2C address of the ICM20948 */
} icm20948_config_i2c_t;

/**
 * @brief Initialize the ICM20948 device struct with a Linux I2C transport.
 *
 * @param device  Pointer to the device struct to initialize.
 * @param config  Pointer to the I2C configuration (fd + address).
 */
void icm20948_init_i2c(icm20948_device_t *device, icm20948_config_i2c_t *config);

/* These functions are exposed to allow custom serif_t setup */
icm20948_status_e icm20948_internal_write_i2c(uint8_t reg, uint8_t *data, uint32_t len, void *user);
icm20948_status_e icm20948_internal_read_i2c(uint8_t reg, uint8_t *buff, uint32_t len, void *user);

#ifdef __cplusplus
}
#endif

#endif /* _ICM_20948_I2C_H_ */

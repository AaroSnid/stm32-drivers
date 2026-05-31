#ifndef LIS3DHTR_H_
#define LIS3DHTR_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LIS3DH_MODE_LOW_POWER       = 0,  // 8-bit data output
    LIS3DH_MODE_NORMAL          = 1,  // 10-bit data output
    LIS3DH_MODE_HIGH_RES        = 2   // 12-bit data output
} lis3dh_op_mode_t;

typedef enum {
    LIS3DH_2                    = 0,  // +-2.0g Measurement Range
    LIS3DH_4                    = 1,  // +-4.0g Measurement Range
    LIS3DH_8                    = 2,  // +-8.0g Measurement Range
    LIS3DH_16                   = 3,  // +-16.0g Measurement Range
} lis3dh_sensitivity_t;

typedef struct {
    void *comms_handle;
    GPIO_TypeDef* chip_select_port;
    uint16_t chip_select_pin;
} lis3dhtr_cfg_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} lis3dh_raw_data_t;

typedef struct {
    float x_g;
    float y_g;
    float z_g;
} lis3dh_g_data_t;


/**
 * @brief Initialize the LIS3DHTR driver interface.
 * @note Sets struct variables, verifies connection through WHO_AM_I
 *
 * @param hw_cfg        Driver configuration structure
 * @param comms_handle  STM32 SPI handle (or I2C handle in future expansion)
 * @param gpio_port     GPIO port for spi CS pin if using SPI
 * @param gpio_pin      GPIO pin number for spi CS pin if using SPI
 *
 * @return 
 */
int lis3dh_init(lis3dhtr_cfg_t *hw_cfg, void *comms_handle, GPIO_TypeDef* cs_port, uint16_t cs_pin);

/**
 * @brief Sets basic sensor data output parameters.
 *
 * @param hw_cfg        Driver configuration structure
 * @param odr_code      Raw bit values for Output Data Rate in CTRL_REG1
 * @param mode          Target resolution (8, 10, or 12-bit output)
 * @param axes_en_mask  Bitmask for X, Y, and Z enable bits in the format 0bZYX
 * 
 * @return
 */
int list3dh_config_data(lis3dhtr_cfg_t *hw_cfg, uint8_t odr_code, lis3dh_op_mode_t mode, uint8_t axes_en_mask);

/**
 * @brief Checks if a new set of data is available
 *
 * @param hw_cfg        Driver configuration structure
 */
bool lis3dh_is_data_ready(lis3dhtr_cfg_t *hw_cfg);

/**
 * @brief Reads raw combined data blocks from acceleration output registers
 *
 * @param hw_cfg        Driver configuration structure
 * @param raw_data      Struct to return raw sensor data
 */
int lis3dh_read_raw_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *raw_data);

/**
 * @brief Reads acceleration data and automatically converts it to units of gravity
 * using runtime tracking of sensitivity and offsets
 *
 * @param hw_cfg        Driver configuration structure
 * @param g_data        Struct to return data in units G (gravity)
 */
int lis3dh_read_g_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_g_data_t *g_data);

/**
 * @brief Performs sensor internal self-test
 *
 * @param hw_cfg        Driver configuration structure
 */
int lis3dh_set_self_test(lis3dhtr_cfg_t *hw_cfg);

/**
 * @brief Configures "Sleep-to-wake" threshold and duration properties
 * Automatically switches to 10Hz Low-Power when acceleration falls below threshold
 *
 * @param hw_cfg        Driver configuration structure
 * @param threshold     Threshold below which the device switches to low power mode
 * @param duration      
 */
int lis3dh_configure_sleep_to_wake(lis3dhtr_cfg_t *hw_cfg, uint8_t threshold, uint8_t duration);

/**
 * @brief Enables 6D or 4D spatial orientation detection configurations
 *
 * @param hw_cfg                Driver configuration structure
 * @param enable_6d             Enable/disable orientation detection
 * @param disable_z_axis_4d     False = 6D orientation, True = 4D orientation
 */
int lis3dh_configure_orientation_detection(lis3dhtr_cfg_t *hw_cfg, bool enable_6d, bool disable_z_axis_4d);

/**
 * @brief Enables embedded 32-level FIFO block
 *
 * @param hw_cfg        Driver configuration structure
 */
int lis3dh_fifo_enable(lis3dhtr_cfg_t *hw_cfg);

/**
 * @brief Disables embedded 32-level FIFO block
 *
 * @param hw_cfg        Driver configuration structure
 */
int lis3dh_fifo_disable(lis3dhtr_cfg_t *hw_cfg);

#endif /* LIS3DHTR_H_ */
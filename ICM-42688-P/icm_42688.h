#ifndef ICM_42688_H_
#define ICM_42688_H_

#include "main.h"
#include <stdint.h>

typedef struct {
    void* comms_handle;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    uint8_t packet_no;
    int16_t accel_calibration[3];
    int16_t gyro_calibration[3];
} icm_42688_cfg_t;

/**
 * @brief Configure the ICM-42688-P driver interface
 *
 * @param hw_cfg        Driver configuration structure
 * @param comms_handle  STM32 SPI handle (or I2C handle in future expansion)
 * @param gpio_port     GPIO port for spi CS pin if using SPI
 * @param gpio_pin      GPIO pin number for spi CS pin if using SPI
 *
 * @return 0 or -1
 */
int icm_42688_config(icm_42688_cfg_t* hw_cfg, void* comms_handle, GPIO_TypeDef* gpio_port, uint16_t gpio_pin);

/**
 * @brief Sets the bank number for next register read/write operations
 *
 * @param hw_cfg        Driver configuration structure
 * @param reg           Register to write
 *
 * @return 0 or -1
 */
int icm_42688_set_bank(icm_42688_cfg_t* hw_cfg, uint8_t bank);

/**
 * @brief Read one byte of data from a register
 *
 * @param hw_cfg        Driver configuration structure
 * @param reg           Register to read
 * @param rx_data       Reference for data
 *
 * @return 0 or -1
 */
int icm_42688_read_reg(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t* rx_data);

/**
 * @brief Write one byte of data from a register. Note that from the datasheet: "unless otherwise noted [the] default value must be maintained even if the values of other register fields are modified by the user"
 *
 * @param hw_cfg        Driver configuration structure
 * @param reg           Register to write
 * @param rx_data       Reference for data
 *
 * @return 0 or -1
 */
int icm_42688_write_reg(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t data);

/**
 * @brief Modify only part of a register
 *
 * @param hw_cfg        Driver configuration structure
 * @param bits_mask     Bit mask for the selected bits
 * @param reg           Register to write
 * @param data          Data to write
 * @param lsb_address   Index for LSB of the selected bits
 *
 * @return 0 or -1
 */
int icm_42688_read_mod_write(icm_42688_cfg_t* hw_cfg, uint8_t bits_mask, uint8_t reg, uint8_t data, uint8_t lsb_address);

/**
 * @brief Reset device to initial values
 *
 * @param hw_cfg        Driver configuration structure
 *
 * @return 0 or -1
 */
int icm_42688_reset_device(icm_42688_cfg_t* hw_cfg);

/**
 * @brief Configure device in standard mode enabling accelerometer and gyroscope
 *
 * @param hw_cfg        Driver configuration structure
 *
 * @return 0 or -1
 */
int icm_42688_configure_device(icm_42688_cfg_t* hw_cfg);

/**
 * @brief Configure accelerometer full scale (precision)
 *
 * @param hw_cfg        Driver configuration structure
 * @param full_scale    Full scale selection
 *
 * @return 0 or -1
 */
int icm_42688_set_accel_fs(icm_42688_cfg_t* hw_cfg, uint8_t full_scale);

/**
 * @brief Read accelerometer full scale (precision)
 *
 * @param hw_cfg        Driver configuration structure
 * @param full_scale    Full scale return data
 *
 * @return 0 or -1
 */
int icm_42688_get_accel_fs(icm_42688_cfg_t* hw_cfg, uint8_t* full_scale);

/**
 * @brief Configure gyroscope full scale (precision)
 *
 * @param hw_cfg        Driver configuration structure
 * @param full_scale    Full scale selection
 *
 * @return 0 or -1
 */
int icm_42688_set_gyro_fs(icm_42688_cfg_t* hw_cfg, uint8_t full_scale);

/**
 * @brief Read gyroscope full scale (precision)
 *
 * @param hw_cfg        Driver configuration structure
 * @param full_scale    Full scale return data
 *
 * @return 0 or -1
 */
int icm_42688_get_gyro_fs(icm_42688_cfg_t* hw_cfg, uint8_t* full_scale);

/**
 * @brief Set accelerometer output data rate
 *
 * @param hw_cfg            Driver configuration structure
 * @param out_data_rate     Output data rate number corresponding to frequency
 *
 * @return 0 or -1
 */
int icm_42688_set_accel_odr(icm_42688_cfg_t* hw_cfg, uint8_t out_data_rate);

/**
 * @brief Set gyroscope output data rate
 *
 * @param hw_cfg            Driver configuration structure
 * @param out_data_rate     Output data rate number corresponding to frequency
 *
 * @return 0 or -1
 */
int icm_42688_set_gyro_odr(icm_42688_cfg_t* hw_cfg, uint8_t out_data_rate);

/**
 * @brief Read accelerometer data in XYZ order format
 *
 * @param hw_cfg       Driver configuration structure
 * @param xyz_data     Array of length 3 minimum to store data
 *
 * @return 0 or -1
 */
int icm_42688_read_accel_xyz(icm_42688_cfg_t* hw_cfg, int16_t* xyz_data);

/**
 * @brief Read gyroscope data in XYZ order format
 *
 * @param hw_cfg       Driver configuration structure
 * @param xyz_data     Array of length 3 minimum to store data
 *
 * @return 0 or -1
 */
int icm_42688_read_gyro_xyz(icm_42688_cfg_t* hw_cfg, int16_t* xyz_data);

/**
 * @brief Get offset of the accelerometer, data stored in hw_cfg
 *
 * @param hw_cfg       Driver configuration structure
 *
 * @return 0 or -1
 */
int icm_42688_calibrate_accel(icm_42688_cfg_t* hw_cfg);

/**
 * @brief Get offset of the gyroscope, data stored in hw_cfg
 *
 * @param hw_cfg       Driver configuration structure
 *
 * @return 0 or -1
 */
int icm_42688_calibrate_gyro(icm_42688_cfg_t* hw_cfg);

/**
 * @brief Set device calibration registers to calibrate return data
 *
 * @param hw_cfg    Driver configuration structure
 * @param accel     Set to 0 does not set accelerometer calibration
 * @param gyro      Set to 0 does not set gyroscope calibration
 *
 * @return 0 or -1
 */
int icm_42688_set_user_offset(icm_42688_cfg_t* hw_cfg, uint8_t accel, uint8_t gyro);

/**
 * @brief Configure FIFO buffer according to predefined packet structure
 *
 * @param hw_cfg            Driver configuration structure
 * @param packet_structure  uint from 1 to 4 to select packet number
 *
 * @return 0 or -1
 */
int icm_42688_config_fifo_register(icm_42688_cfg_t* hw_cfg, uint8_t packet_structure);

/**
 * @brief Read from FIFO buffer based on configured packet structure
 *
 * @param hw_cfg        Driver configuration structure
 * @param gyro_data     Gyroscope return data, pass NULL if none
 * @param accel_data    Accelerometer return data, pass NULL if none
 * @param temp_data     Temperature return data, pass NULL if none
 * @param extened_data  Extended return data, pass NULL if none
 *
 * @return 0 or -1
 */
int icm_42688_read_fifo(icm_42688_cfg_t* hw_cfg, int8_t* gyro_data, int8_t* accel_data, int8_t* temp_data, int8_t* time_data, int8_t* extened_data);

/**
 * @brief Read from the WHO_AM_I register, and compare with expected value
 *
 * @param hw_cfg    Driver configuration structure
 *
 * @return 0 on success, -1 on failure
 */
int icm_42688_test_comms(icm_42688_cfg_t* hw_cfg);

/**
 * @brief Configure APEX raise to wake functionality
 *
 * @param hw_cfg            Driver configuration structure
 * @param interrupt_config  Interrupt number to map (1 or 2), otherwise interrupt not used
 * @param wake_sleep        0 for wake detect configuration, otherwise sleep detect configuration
 *
 * @return 0 or -1
 */
int icm_42688_apex_raise_to_wake(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config, uint8_t wake_sleep);

/**
 * @brief Configure APEX tap detection functionality
 *
 * @param hw_cfg            Driver configuration structure
 * @param performance_mode  uint from 1 to 3 to choose ODR speedm, currently unused
 * @param interrupt_config  Interrupt number to map (1 or 2), otherwise interrupt not used
 *
 * @return 0 or -1
 */
int icm_42688_apex_tap_detection(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config);

/**
 * @brief Configure APEX wake on motion functionality
 *
 * @param hw_cfg            Driver configuration structure
 * @param interrupt_config  Interrupt number to map (1 or 2), otherwise interrupt not used
 *
 * @return 0 or -1
 */
int icm_42688_apex_wake_on_motion(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config);

/**
 * @brief Configure APEX significan motion detection functionality
 *
 * @param hw_cfg            Driver configuration structure
 * @param interrupt_config  Interrupt number to map (1 or 2), otherwise interrupt not used
 *
 * @return 0 or -1
 */
int icm_42688_apex_sig_motion_detect(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config);

#endif
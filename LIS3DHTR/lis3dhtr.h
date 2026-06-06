#ifndef LIS3DHTR_H_
#define LIS3DHTR_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef enum {
    LIS3DH_ODR_POWER_DOWN       = 0,
    LIS3DH_ODR_1HZ              = 1,
    LIS3DH_ODR_10HZ             = 2,
    LIS3DH_ODR_25HZ             = 3,
    LIS3DH_ODR_50HZ             = 4,
    LIS3DH_ODR_100HZ            = 5,
    LIS3DH_ODR_200HZ            = 6,
    LIS3DH_ODR_400HZ            = 7,
    LIS3DH_ODR_1_344KHZ         = 8,
    LIS3DH_ODR_1_620KHZ         = 9,
    LIS3DH_ODR_5_376KHZ         = 10,
} lis3dh_odr_t;

typedef enum {
    LIS3DH_FIFO_BYPASS          = 0,
    LIS3DH_FIFO_MODE            = 1,
    LIS3DH_STREAM_MODE          = 2,
    LIS3DH_STREAM_TO_FIFO_MODE  = 3,
} lis3dh_fifo_mode_t;

typedef struct {
    bool empty;
    bool watermark;
    bool overrun;
    uint8_t fill_level;
} lis3dh_fifo_status_t;

typedef enum {
    LIS3DH_INT_PIN_1            = 1,
    LIS3DH_INT_PIN_2            = 2,
} lis3dh_int_pin_t;

typedef enum {
    LIS3DH_ADC_CHANNEL_1        = 1,
    LIS3DH_ADC_CHANNEL_2        = 2,
    LIS3DH_ADC_CHANNEL_3        = 3,
} lis3dh_adc_channel_t;

typedef enum {
    LIS3DH_HIGH_PASS_MODE_BYPASS      = 0,
    LIS3DH_HIGH_PASS_MODE_NORMAL_RESET = 1,
    LIS3DH_HIGH_PASS_MODE_REFERENCE    = 2,
    LIS3DH_HIGH_PASS_MODE_AUTO_RESET   = 3,
} lis3dh_high_pass_mode_t;

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
 * @brief Reads the device WHO_AM_I identifier.
 *
 * @param hw_cfg        Driver configuration structure
 * @param device_id     Pointer to return the device ID value
 */
int lis3dh_get_device_id(lis3dhtr_cfg_t *hw_cfg, uint8_t *device_id);

/**
 * @brief Configures device output data rate, full scale, operating mode, and enabled axes.
 *
 * @param hw_cfg        Driver configuration structure
 * @param odr           Output Data Rate setting
 * @param full_scale    Full scale selection (+-2g, +-4g, +-8g, +-16g)
 * @param mode          Target resolution mode
 * @param axes_en_mask  Bitmask for X, Y, and Z enable bits in the format 0bZYX
 */
int lis3dh_configure_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_odr_t odr, lis3dh_sensitivity_t full_scale,
                          lis3dh_op_mode_t mode, uint8_t axes_en_mask);

/**
 * @brief Enables or disables the built-in high-pass filter.
 *
 * @param hw_cfg        Driver configuration structure
 * @param enable        Enable or disable the filter
 * @param mode          Selected high-pass filter operating mode
 */
int lis3dh_set_high_pass_filter(lis3dhtr_cfg_t *hw_cfg, bool enable, lis3dh_high_pass_mode_t mode);

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
 * @brief Reads the full sensor status register.
 *
 * @param hw_cfg        Driver configuration structure
 * @param status        Pointer to return status register contents
 */
int lis3dh_get_status(lis3dhtr_cfg_t *hw_cfg, uint8_t *status);

/**
 * @brief Enables or disables the self-test function.
 *
 * @param hw_cfg        Driver configuration structure
 * @param enable        True to enable self-test, false to disable
 * @param positive_sign True for positive self-test sign, false for negative sign
 */
int lis3dh_set_self_test(lis3dhtr_cfg_t *hw_cfg, bool enable, bool positive_sign);

/**
 * @brief Configures sleep-to-wake and return-to-sleep behavior.
 *
 * @param hw_cfg        Driver configuration structure
 * @param enable        True to enable sleep-to-wake behavior
 * @param threshold     Threshold for ACT_THS register
 * @param duration      Duration for ACT_DUR register
 */
int lis3dh_configure_sleep_to_wake(lis3dhtr_cfg_t *hw_cfg, bool enable, uint8_t threshold, uint8_t duration);

/**
 * @brief Enables or disables auxiliary ADC sampling.
 *
 * @param hw_cfg        Driver configuration structure
 * @param enable        True to enable auxiliary ADC
 */
int lis3dh_enable_aux_adc(lis3dhtr_cfg_t *hw_cfg, bool enable);

/**
 * @brief Enables or disables the temperature sensor on ADC channel 3.
 *
 * @param hw_cfg        Driver configuration structure
 * @param enable        True to enable temperature measurement
 */
int lis3dh_enable_temperature_sensor(lis3dhtr_cfg_t *hw_cfg, bool enable);

/**
 * @brief Reads a raw auxiliary ADC channel value.
 *
 * @param hw_cfg        Driver configuration structure
 * @param channel       ADC channel to read
 * @param adc_raw       Pointer to return raw ADC output
 */
int lis3dh_read_adc_channel(lis3dhtr_cfg_t *hw_cfg, lis3dh_adc_channel_t channel, int16_t *adc_raw);

/**
 * @brief Reads temperature in degrees Celsius from the on-chip sensor.
 *
 * @param hw_cfg        Driver configuration structure
 * @param temperature_c Pointer to return temperature in degrees Celsius
 */
int lis3dh_read_temperature(lis3dhtr_cfg_t *hw_cfg, float *temperature_c);

/**
 * @brief Enables 6D or 4D spatial orientation detection configurations
 *
 * @param hw_cfg                Driver configuration structure
 * @param enable_6d             Enable/disable orientation detection
 * @param disable_z_axis_4d     False = 6D orientation, True = 4D orientation
 */
int lis3dh_configure_orientation_detection(lis3dhtr_cfg_t *hw_cfg, bool enable_6d, bool disable_z_axis_4d);

/**
 * @brief Configures FIFO operation, watermark threshold, and overrun interrupt.
 *
 * @param hw_cfg                Driver configuration structure
 * @param mode                  FIFO operating mode
 * @param watermark_level       FIFO watermark threshold (0-31)
 * @param overrun_interrupt     True to enable FIFO overrun interrupt on INT1
 */
int lis3dh_configure_fifo(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_mode_t mode, uint8_t watermark_level,
                          bool overrun_interrupt);

/**
 * @brief Reads FIFO status including fill level, watermark, and overrun.
 *
 * @param hw_cfg        Driver configuration structure
 * @param status        Pointer to return FIFO status fields
 */
int lis3dh_get_fifo_status(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_status_t *status);

/**
 * @brief Reads acceleration samples from the FIFO buffer.
 *
 * @param hw_cfg            Driver configuration structure
 * @param fifo_samples      Array to receive FIFO samples
 * @param samples_to_read   Number of samples requested
 * @param samples_read      Pointer to return number of samples actually read
 */
int lis3dh_read_fifo_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *fifo_samples,
                          uint8_t samples_to_read, uint8_t *samples_read);

/**
 * @brief Configures a programmable interrupt generator on INT1 or INT2.
 *
 * @param hw_cfg        Driver configuration structure
 * @param int_pin       Target interrupt pin
 * @param cfg_bits      Raw INTx_CFG register bits
 * @param threshold     INTx_THS threshold value
 * @param duration      INTx_DURATION duration value
 */
int lis3dh_configure_interrupt(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t cfg_bits,
                                uint8_t threshold, uint8_t duration);

/**
 * @brief Reads the interrupt source register for INT1 or INT2.
 *
 * @param hw_cfg        Driver configuration structure
 * @param int_pin       Target interrupt pin
 * @param source        Pointer to return source register contents
 */
int lis3dh_read_interrupt_source(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t *source);

/**
 * @brief Configures click/double-click recognition.
 *
 * @param hw_cfg        Driver configuration structure
 * @param click_cfg     CLICK_CFG register bits
 * @param click_ths     CLICK_THS threshold value
 * @param time_limit    TIME_LIMIT register value
 * @param time_latency  TIME_LATENCY register value
 * @param time_window   TIME_WINDOW register value
 */
int lis3dh_configure_click(lis3dhtr_cfg_t *hw_cfg, uint8_t click_cfg, uint8_t click_ths,
                           uint8_t time_limit, uint8_t time_latency, uint8_t time_window);

/**
 * @brief Reads the click source register.
 *
 * @param hw_cfg        Driver configuration structure
 * @param click_source  Pointer to return CLICK_SRC register contents
 */
int lis3dh_read_click_source(lis3dhtr_cfg_t *hw_cfg, uint8_t *click_source);

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

#ifdef __cplusplus
}
#endif

#endif /* LIS3DHTR_H_ */
#ifndef SN74HC595_H_
#define SN74HC595_H_

#include <stdint.h>
#include "main.h"

#define SN74HC595_SPI_BLOCKING 1
#define SN74HC595_SPI_IT 2
#define SN74HC595_SPI_DMA 3

typedef HAL_StatusTypeDef (*sn74hc595_transmit_function)(SPI_HandleTypeDef*, uint8_t*, uint16_t);

typedef struct {
    GPIO_TypeDef* rclk_port;
    uint16_t rclk_pin;
    SPI_HandleTypeDef* hspi;
    uint8_t spi_mode;
    sn74hc595_transmit_function transmit_function;
    uint8_t config_run;
} sn74hc595_cfg_t;

/**
 * @brief Configure the SN74HC595 driver interface
 *
 * @param hw_cfg    Driver configuration structure
 * @param hspi      STM32 SPI handle
 * @param GPIOx     GPIO port for latch pin
 * @param GPIO_Pin  GPIO pin number for latch
 * @param spi_mode  SPI transmission mode, one of SN74HC595_SPI_BLOCKING, SN74HC595_SPI_IT, or SN74HC595_SPI_DMA
 *
 * @return 0 or -1
 */
int sn74hc595_config(   sn74hc595_cfg_t* hw_cfg, 
                        SPI_HandleTypeDef* hspi, 
                        GPIO_TypeDef* GPIOx, 
                        uint16_t GPIO_Pin, 
                        uint8_t spi_mode);


/**
 * @brief Latch the data
 *
 * @param hw_cfg    Driver configuration structure
 *
 * @return 0 or -1
 */
int sn74hc595_latch_data(sn74hc595_cfg_t* hw_cfg);

/**
 * @brief Shift a byte into the SN74HC595
 *
 * @param hw_cfg Pointer to driver configuration structure
 * @param data   Byte to transmit
 *
 * @return 0 or -1
 */
int sn74hc595_shift_byte(   sn74hc595_cfg_t* hw_cfg, 
                            uint8_t data);

#endif /* SN74HC595_H_ */
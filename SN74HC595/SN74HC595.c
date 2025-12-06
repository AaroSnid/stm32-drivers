#include "SN74HC595.h"

HAL_StatusTypeDef standard_spi_transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t size) {
    return HAL_SPI_Transmit(hspi, pData, size, HAL_MAX_DELAY);
}

int sn74hc595_config(sn74hc595_cfg_t* hw_cfg, SPI_HandleTypeDef* hspi, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t spi_mode) {
    hw_cfg->rclk_port = GPIOx;
    hw_cfg->rclk_pin = GPIO_Pin;
    hw_cfg->hspi = hspi;

    switch (spi_mode){
        case SN74HC595_SPI_BLOCKING:
        hw_cfg->transmit_function = &standard_spi_transmit;
        hw_cfg->spi_mode = SN74HC595_SPI_BLOCKING;
        break;
        case SN74HC595_SPI_IT:
        hw_cfg->transmit_function = &HAL_SPI_Transmit_IT;
        hw_cfg->spi_mode = SN74HC595_SPI_IT;
        break;
        case SN74HC595_SPI_DMA:
        hw_cfg->transmit_function = &HAL_SPI_Transmit_DMA;
        hw_cfg->spi_mode = SN74HC595_SPI_DMA;
        break;
        default:
        return -1;
    }

    hw_cfg->config_run = 1;
    return 0;
}

int sn74hc595_latch_data(sn74hc595_cfg_t* hw_cfg){
    if (!hw_cfg) return -1;
    HAL_GPIO_WritePin(hw_cfg->rclk_port, hw_cfg->rclk_pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(hw_cfg->rclk_port, hw_cfg->rclk_pin, GPIO_PIN_RESET);
    return 0;
}

int sn74hc595_shift_byte(sn74hc595_cfg_t* hw_cfg, uint8_t data) {
    if (!hw_cfg) return -1;
    if (hw_cfg->config_run != 1) return -1;
    uint8_t tx_data[1] = {data};

    if (hw_cfg->transmit_function(hw_cfg->hspi, tx_data, 1) != HAL_OK) return -1;
    if (hw_cfg->spi_mode == SN74HC595_SPI_BLOCKING){
        sn74hc595_latch_data(hw_cfg);
    }
    return 0;
}
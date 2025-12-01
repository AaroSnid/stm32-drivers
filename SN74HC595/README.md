# SN74HC595 - SPI

A driver for controlling the **SN74HC595 Serial-In / Parallel-Out shift register** through the STM32 HAL SPI interface.

Supports:
- Blocking, Interrupt-driven, DMA SPI Modes
- Multiple shift register instances


## Features

- SPI-based output control  
- Latch pin handling for polling / blocking mode
- Mode control (blocking / interrupt / DMA)  
- Errors propagate through return values
- Clean HAL based API  

## Files

sn74hc595.h → Public API

sn74hc595.c → Driver implementation

## Hardware Connection

| SN74HC595 Pin | STM32 Pin |
|----------------|------------|
| DS (SER)       | SPI MOSI   |
| SHCP (SRCLK)   | SPI SCK    |
| STCP (RCLK)    | GPIO       |
| OE             | GND or GPIO|
| MR             | VCC        |
| VCC / GND      | Power rails|


## Driver Installation

1. Add `sn74hc595.c` to your source folder  
2. Add `sn74hc595.h` to your include path  
3. Enable SPI as Half-Dulpex Master
4. Configure SPI in MODE 0 (CPOL=0, CPHA=0)  
5. Set baud rate under maximum according to datasheet
6. Set data size to 8 bits


## Driver Configuration Structure

```c
typedef struct {
    GPIO_TypeDef*         rclk_port;
    uint16_t              rclk_pin;
    SPI_HandleTypeDef*    hspi;
    sn74hc595_spi_mode_t  spi_mode;
    sn74hc595_tx_fn       transmit_function;
    uint8_t               initialized;
} sn74hc595_cfg_t;
```

### SPI Mode Selection

Use one of:

```c
SN74HC595_SPI_BLOCKING
SN74HC595_SPI_IT
SN74HC595_SPI_DMA
```

## Example usage

#### SPI in Polling Mode

```c
sn74hc595_cfg_t shiftreg;

sn74hc595_config(
    &shiftreg,
    &hspi1,
    GPIOA,
    GPIO_PIN_3,
    SN74HC595_SPI_BLOCKING
);

sn74hc595_shift_byte(&shiftreg, 0b01101001);
```

#### SPI in Interrupt Mode:

Note that latch pin function must be triggered in callback function

```c
sn74hc595_cfg_t shiftreg;

sn74hc595_config(
    &shiftreg,
    &hspi1,
    GPIOA,
    GPIO_PIN_3,
    SN74HC595_SPI_IT
);

sn74hc595_shift_byte(&shiftreg, 0b01101001);

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == hw_cfg->hspi){
        sn74hc595_latch_data(&hw_cfg);
    }
}
```

#### SPI in DMA Mode:

Note that latch pin function must be triggered manually

```c
sn74hc595_cfg_t shiftreg;

sn74hc595_config(
    &shiftreg,
    &hspi1,
    GPIOC,
    GPIO_PIN_2,
    SN74HC595_SPI_DMA
);

sn74hc595_shift_byte(&shiftreg, 0b01101001);
```
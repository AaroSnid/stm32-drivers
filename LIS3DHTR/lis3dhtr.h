#ifndef LIS3DHTR_H_
#define LIS3DHTR_H_

#include "main.h"
#include <stdint.h>

typedef struct {
    void *comms_handle;
    GPIO_TypeDef* chip_select_port;
    uint16_t chip_select_pin;
} lis3dhtr_cfg_t;

#endif /* LIS3DHTR_H_ */
#include "icm_42688.h"
#include "icm_42688_registers.h"

uint8_t packet_no = 0;

#define TX_RX_LENGTH 21     // Max length chosen for SPI fifo read
uint8_t tx_data[TX_RX_LENGTH] = {0};
uint8_t rx_data[TX_RX_LENGTH] = {0};

int icm_42688_config(icm_42688_cfg_t* hw_cfg, void* comms_handle, GPIO_TypeDef* gpio_port, uint16_t gpio_pin){
    hw_cfg->comms_handle = comms_handle;
    hw_cfg->gpio_port = gpio_port;
    hw_cfg->gpio_pin = gpio_pin;
    return 0;
}

int cs_high(icm_42688_cfg_t* hw_cfg){
    if (hw_cfg->gpio_port == NULL) return -1;
    HAL_GPIO_WritePin(hw_cfg->gpio_port, hw_cfg->gpio_pin, GPIO_PIN_SET);
    return 0;
}

int cs_low(icm_42688_cfg_t* hw_cfg){
    if (hw_cfg->gpio_port == NULL) return -1;
    HAL_GPIO_WritePin(hw_cfg->gpio_port, hw_cfg->gpio_pin, GPIO_PIN_RESET);
    return 0;
} 

int build_spi_message(uint8_t* message, uint8_t read_write, uint8_t reg, uint8_t data){
    message[0] = (read_write ? 0x80 : 0x00) | (reg & 0x7F);

    if (read_write == 0){
        message[1] = data;
    } 
    return 0;
}

int spi_read_data(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t no_bytes){
    if (no_bytes < 2) return -1;    // Minimum 2 bytes

    // Fill tx_data with data that will not affect chip
    for (int i = 0; i < TX_RX_LENGTH; i++){
        tx_data[i] = 0xFF;
    }
    build_spi_message(tx_data, 1, reg, 0);

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hw_cfg->comms_handle, tx_data, rx_data, no_bytes + 1, HAL_MAX_DELAY);
    cs_high(hw_cfg);
    if (status != HAL_OK) return -1;
    
    for(int i = 1; i < TX_RX_LENGTH; i++){
        rx_data[i - 1] = rx_data[i];
    }
    return 0;
}

int spi_write_data(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t data){
    build_spi_message(tx_data, 0, reg, data);

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hw_cfg->comms_handle, tx_data, 2, HAL_MAX_DELAY);
    cs_high(hw_cfg);
    if (status != HAL_OK){return -1;}
    return 0;
}

int configure_bank(icm_42688_cfg_t* hw_cfg, uint8_t bank){
    if (bank > 4) return -1; // Invalid selection
    return spi_write_data(hw_cfg, REG_BANK_SEL, (bank & 0x07));
}

int read_mod_write_spi(icm_42688_cfg_t* hw_cfg, uint8_t bits_mask, uint8_t reg, uint8_t data, uint8_t lsb_address){
    // Ex: write to bits [5:4]. bits_mask = 0b11, data = 0bxx, lsb_address = 4.
    if (spi_read_data(hw_cfg, reg, 1) != 0) return -1;
    uint8_t transfer_data = (rx_data[1] & ~(bits_mask << lsb_address)) | (data << lsb_address);
    if (spi_write_data(hw_cfg, reg, transfer_data) != 0) return -1;
    return 0;
}

int reset_device(icm_42688_cfg_t* hw_cfg){
    configure_bank(hw_cfg, 0); // Bank 0 data
    if (spi_write_data(hw_cfg, DEVICE_CONFIG, 0x01) != 0) return -1;
    return 0;
}

int configure_device(icm_42688_cfg_t* hw_cfg){
    reset_device(hw_cfg);
    HAL_Delay(1);

    // Turn on gyroscope and accelerometer
    spi_write_data(hw_cfg, PWR_MGMT0, 0x0F);
    HAL_Delay(45);
}

int configure_fifo_register(icm_42688_cfg_t* hw_cfg, uint8_t packet_structure){
    packet_no = packet_structure;
    configure_bank(hw_cfg, 0); // Bank 0 data

    // Configure FIFO mode, [7:6] -> 01 Stream-to-FIFO Mode
    if (spi_write_data(hw_cfg, FIFO_CONFIG, 0x01) != 0) return -1;

    // Configure data in FIFO
    uint8_t data = 0;
    switch (packet_structure){
        case 1:
        data = 0b00101;
        break;
        case 2:
        data = 0b00110;
        break;
        case 3:
        data = 0b01111;
        break;
        default:
        data = 0b11111;
        break;
    }
    if (spi_write_data(hw_cfg, FIFO_CONFIG1, data) != 0) return -1;
    return 0;
}

int read_fifo(icm_42688_cfg_t* hw_cfg, uint8_t* gyro_data, uint8_t* accel_data, uint8_t* temp_data, uint8_t* time_data, uint8_t* extened_data){
    // This could be changed to read the header data instead of the packet variable (page 37) except for needing different read lengths

    configure_bank(hw_cfg, 0); // Bank 0 data

    if (packet_no == 0){
        return -1; // FIFO unconfigured
    } else if (packet_no == 1){
        if (accel_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        if (spi_read_data(hw_cfg, FIFO_DATA, 8) != 0) return -1;
        for (int i = 1; i < 7; i++){
            accel_data[i - 1] = rx_data[i];
        }
        temp_data[0] = rx_data[7];
    } else if (packet_no == 2){
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        if (spi_read_data(hw_cfg, FIFO_DATA, 8) != 0) return -1;
        for (int i = 1; i < 7; i++){
            gyro_data[i - 1] = rx_data[i];
        }
        temp_data[0] = rx_data[7];
    } else if (packet_no == 3){
        if (accel_data == NULL) return -1;
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        if (spi_read_data(hw_cfg, FIFO_DATA, 16) != 0) return -1;
        for (int i = 1; i < 7; i++){
            accel_data[i - 1] = rx_data[i];
        }
        for (int i = 7; i < 13; i++){
            gyro_data[i - 7] = rx_data[i];
        }
        temp_data[0] = rx_data[13];
        for (int i = 14; i < 16; i++){
            gyro_data[i - 14] = rx_data[i];
        }
    } else if (packet_no == 4){
        if (accel_data == NULL) return -1;
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;
        if (extened_data == NULL) return -1;

        if (spi_read_data(hw_cfg, FIFO_DATA, 20) != 0) return -1;
        for (int i = 1; i < 7; i++){
            accel_data[i - 1] = rx_data[i];
        }
        for (int i = 7; i < 13; i++){
            gyro_data[i - 7] = rx_data[i];
        }
        temp_data[0] = rx_data[13];
        temp_data[0] = rx_data[14];
        for (int i = 15; i < 17; i++){
            gyro_data[i - 15] = rx_data[i];
        }
        for (int i = 17; i < 20; i++){
            gyro_data[i - 17] = rx_data[i];
        }
    } else {
        return -1;
    }
    return 0;
}

int device_self_test(){

}

int configure_interrupts(){


}

int configure_apex_pedometer(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config, uint8_t* settings){
    if ((performance_mode != 1) && (performance_mode != 2)) return -1;    // Invalid selection

    return -1;
}

int configure_apex_tilt_detection(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config){
    if (performance_mode > 2) return -1;    // Invalid selection
    return -1; // Function not yet implemented
}

int configure_apex_raise_to_wake(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config){

    return -1; // Function not yet implemented
}

int configure_apex_tap_detection(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config){
    if (performance_mode > 3) return -1;    // Invalid selection
    return -1; // Function not yet implemented
}

int configure_apex_wake_on_motion(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config){
    
    return -1; // Function not yet implemented
}

int configure_apex_sig_motion_detect(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config){

    return -1; // Function not yet implemented
}
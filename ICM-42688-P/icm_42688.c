#include "icm_42688.h"
#include "icm_42688_registers.h"

#define CALIBRARION_SAMPLES 200

int icm_42688_config(icm_42688_cfg_t* hw_cfg, void* comms_handle, GPIO_TypeDef* gpio_port, uint16_t gpio_pin) { 
    hw_cfg->comms_handle = comms_handle;
    hw_cfg->gpio_port = gpio_port;
    hw_cfg->gpio_pin = gpio_pin;
    return 0;
}

int cs_high(icm_42688_cfg_t* hw_cfg) { 
    if (hw_cfg->gpio_port == NULL) return -1;
    HAL_GPIO_WritePin(hw_cfg->gpio_port, hw_cfg->gpio_pin, GPIO_PIN_SET);
    return 0;
}

int cs_low(icm_42688_cfg_t* hw_cfg) { 
    if (hw_cfg->gpio_port == NULL) return -1;
    HAL_GPIO_WritePin(hw_cfg->gpio_port, hw_cfg->gpio_pin, GPIO_PIN_RESET);
    return 0;
} 

void build_spi_message(uint8_t* message, uint8_t read_write, uint8_t reg, uint8_t data) { 
    message[0] = (read_write ? 0x80 : 0x00) | (reg & 0x7F);
    if (read_write == 0) message[1] = data;
}

int spi_read_data(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t* rx_data, uint8_t no_bytes) { 
    if (no_bytes < 2) return -1;    // Minimum 2 bytes
    uint8_t tx_buf[no_bytes + 1];
    uint8_t rx_buf[no_bytes + 1];

    // Fill tx_data with data that will not affect chip
    for (int i = 0; i <= no_bytes; i++) tx_buf[i] = 0xFF;
    build_spi_message(tx_buf, 1, reg, 0);

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hw_cfg->comms_handle, tx_buf, rx_buf, no_bytes + 1, HAL_MAX_DELAY);
    cs_high(hw_cfg);
    if (status != HAL_OK) return -1;
    
    for(int i = 1; i < no_bytes; i++) { 
        rx_data[i - 1] = rx_buf[i];
    }
    return 0;
}

int icm_42688_read_reg(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t* rx_data) { 
    return spi_read_data(hw_cfg, reg, rx_data, 1);  // In future will use function pointer to allow for i2c comms
}

int spi_write_data(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t data) { 
    uint8_t tx_data[2] = {0xFF, 0xFF};
    build_spi_message(tx_data, 0, reg, data);

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hw_cfg->comms_handle, tx_data, 2, HAL_MAX_DELAY);
    cs_high(hw_cfg);
    if (status != HAL_OK) return -1;
    return 0;
}

int icm_42688_write_reg(icm_42688_cfg_t* hw_cfg, uint8_t reg, uint8_t data) { 
    return spi_write_data(hw_cfg, reg, data);  // In future will use function pointer to allow for i2c comms
}

int icm_42688_set_bank(icm_42688_cfg_t* hw_cfg, uint8_t bank) { 
    if (bank > 4) return -1; // Invalid selection
    return icm_42688_write_reg(hw_cfg, REG_BANK_SEL, (bank & 0x07));
}

int icm_42688_read_mod_write(icm_42688_cfg_t* hw_cfg, uint8_t bits_mask, uint8_t reg, uint8_t data, uint8_t lsb_address) { 
    // Ex: write to bits [5:4]. bits_mask = 0b11, data = 0bxx, lsb_address = 4.
    uint8_t rx_data[2];
    if (icm_42688_read_reg(hw_cfg, reg, rx_data) != 0) return -1;
    uint8_t transfer_data = (rx_data[1] & ~(bits_mask << lsb_address)) | (data << lsb_address);
    if (icm_42688_write_reg(hw_cfg, reg, transfer_data) != 0) return -1;
    return 0;
}

int icm_42688_reset_device(icm_42688_cfg_t* hw_cfg) { 
    icm_42688_set_bank(hw_cfg, 0); // Bank 0 data
    if (icm_42688_write_reg(hw_cfg, DEVICE_CONFIG, 0x01) != 0) return -1;
    return 0;
}

int icm_42688_configure_device(icm_42688_cfg_t* hw_cfg) { 
    icm_42688_reset_device(hw_cfg);
    HAL_Delay(1);

    // Turn on gyroscope and accelerometer
    icm_42688_write_reg(hw_cfg, PWR_MGMT0, 0x0F);
    HAL_Delay(45);
    return 0;
}

int icm_42688_set_accel_fs(icm_42688_cfg_t* hw_cfg, uint8_t full_scale) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_mod_write(hw_cfg, 0b111, ACCEL_CONFIG0, full_scale, 5);
}

int icm_42688_get_accel_fs(icm_42688_cfg_t* hw_cfg, uint8_t* full_scale) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_reg(hw_cfg, ACCEL_CONFIG0, full_scale);
}

int icm_42688_set_gyro_fs(icm_42688_cfg_t* hw_cfg, uint8_t full_scale) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_mod_write(hw_cfg, 0b111, GYRO_CONFIG0, full_scale, 5);
}

int icm_42688_get_gyro_fs(icm_42688_cfg_t* hw_cfg, uint8_t* full_scale) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_reg(hw_cfg, GYRO_CONFIG0, full_scale);
}

int icm_42688_set_accel_odr(icm_42688_cfg_t* hw_cfg, uint8_t out_data_rate) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_mod_write(hw_cfg, 0b1111, ACCEL_CONFIG0, out_data_rate, 0);
}

int icm_42688_set_gyro_odr(icm_42688_cfg_t* hw_cfg, uint8_t out_data_rate) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    return icm_42688_read_mod_write(hw_cfg, 0b1111, GYRO_CONFIG0, out_data_rate, 0);
}

int icm_42688_read_accel_xyz(icm_42688_cfg_t* hw_cfg, uint16_t* xyz_data) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    uint8_t rx_data[6];
    if (spi_read_data(hw_cfg, ACCEL_DATA_X1, rx_data, 6) != 0) return -1;
    xyz_data[0] = (rx_data[0] << 8) | rx_data[1];
    xyz_data[1] = (rx_data[2] << 8) | rx_data[3];
    xyz_data[2] = (rx_data[4] << 8) | rx_data[5];
    return 0;
}

int icm_42688_read_gyro_xyz(icm_42688_cfg_t* hw_cfg, uint16_t* xyz_data) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    uint8_t rx_data[6];
    if (spi_read_data(hw_cfg, GYRO_DATA_X1, rx_data, 6) != 0) return -1;
    xyz_data[0] = (uint16_t)(rx_data[0] << 8) | rx_data[1];
    xyz_data[1] = (uint16_t)(rx_data[2] << 8) | rx_data[3];
    xyz_data[2] = (uint16_t)(rx_data[4] << 8) | rx_data[5];
    return 0;
}

int icm_42688_calibrate_accel(icm_42688_cfg_t* hw_cfg) { 
    uint8_t current_scale = 0;
    if (icm_42688_get_accel_fs(hw_cfg, &current_scale) != 0) return -1;

    // Set higher resolution for calibration
    if (icm_42688_set_accel_fs(hw_cfg, 3) != 0) return -1;

    uint32_t accel_data_xyz[3] = {0};
    uint16_t rx_data[3] = {0};
    for (int i = 0; i < CALIBRARION_SAMPLES; i++) { 
        if (icm_42688_read_accel_xyz(hw_cfg, rx_data) != 0) return -1;
        accel_data_xyz[0] += rx_data[0];
        accel_data_xyz[1] += rx_data[1];
        accel_data_xyz[2] += rx_data[2];
        HAL_Delay(1);
    }
    hw_cfg->accel_calibration[0] = (uint16_t)(accel_data_xyz[0] / CALIBRARION_SAMPLES);
    hw_cfg->accel_calibration[1] = (uint16_t)(accel_data_xyz[1] / CALIBRARION_SAMPLES);
    hw_cfg->accel_calibration[2] = (uint16_t)(accel_data_xyz[2] / CALIBRARION_SAMPLES);

    if (icm_42688_set_accel_fs(hw_cfg, current_scale) != 0) return -1;
    return 0;
}

int icm_42688_calibrate_gyro(icm_42688_cfg_t* hw_cfg) { 
    uint8_t current_scale = 0;
    if (icm_42688_get_gyro_fs(hw_cfg, &current_scale) != 0) return -1;

    // Set higher resolution for calibration
    if (icm_42688_set_gyro_fs(hw_cfg, 3) != 0) return -1;

    uint32_t gyro_data_xyz[3] = {0};
    uint16_t rx_data[3] = {0};
    for (int i = 0; i < CALIBRARION_SAMPLES; i++) { 
        if (icm_42688_read_accel_xyz(hw_cfg, rx_data) != 0) return -1;
        gyro_data_xyz[0] += rx_data[0];
        gyro_data_xyz[1] += rx_data[1];
        gyro_data_xyz[2] += rx_data[2];
        HAL_Delay(1);
    }
    hw_cfg->accel_calibration[0] = (uint16_t)(gyro_data_xyz[0] / CALIBRARION_SAMPLES);
    hw_cfg->accel_calibration[1] = (uint16_t)(gyro_data_xyz[1] / CALIBRARION_SAMPLES);
    hw_cfg->accel_calibration[2] = (uint16_t)(gyro_data_xyz[2] / CALIBRARION_SAMPLES);

    if (icm_42688_set_gyro_fs(hw_cfg, current_scale) != 0) return -1;
    return 0;
}

int icm_42688_set_user_offset(icm_42688_cfg_t* hw_cfg, uint8_t accel, uint8_t gyro) { 
    if (icm_42688_set_bank(hw_cfg, 4) !=  0) return -1;

    uint8_t reg0_data = (uint8_t)(hw_cfg->gyro_calibration[0] & 0xFF);
    uint8_t reg1_data = (uint8_t)(((hw_cfg->gyro_calibration[1] >> 8) & 0xF) << 4) | (uint8_t)((hw_cfg->gyro_calibration[0] >> 8) & 0xF);
    uint8_t reg2_data = (uint8_t)(hw_cfg->gyro_calibration[1] & 0xFF);
    uint8_t reg3_data = (uint8_t)(hw_cfg->gyro_calibration[2] & 0xFF);
    uint8_t reg4_data = (uint8_t)(((hw_cfg->accel_calibration[0] >> 8) & 0xF) << 4) | (uint8_t)((hw_cfg->gyro_calibration[2] >> 8) & 0xF);
    uint8_t reg5_data = (uint8_t)(hw_cfg->accel_calibration[0] & 0xFF);
    uint8_t reg6_data = (uint8_t)(hw_cfg->accel_calibration[1] & 0xFF);
    uint8_t reg7_data = (uint8_t)(((hw_cfg->accel_calibration[2] >> 8) & 0xF) << 4) | (uint8_t)((hw_cfg->accel_calibration[1] >> 8) & 0xF);
    uint8_t reg8_data = (uint8_t)(hw_cfg->accel_calibration[2] & 0xFF);

    if (gyro != 0) { 
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER0, reg0_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER1, reg1_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER2, reg2_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER3, reg3_data) != 0) return -1;
    } else {
        reg4_data = reg4_data & 0xF0;
    }

    if (accel != 0) { 
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER5, reg5_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER6, reg6_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER7, reg7_data) != 0) return -1;
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER8, reg8_data) != 0) return -1;
    } else {
        reg4_data = reg4_data & 0x0F;
    }

    if (reg4_data != 0) { 
        if (icm_42688_write_reg(hw_cfg, OFFSET_USER4, reg4_data) != 0) return -1;
    }
    return 0;
}

int icm_42688_config_fifo_register(icm_42688_cfg_t* hw_cfg, uint8_t packet_structure) { 
    hw_cfg->packet_no = packet_structure;
    icm_42688_set_bank(hw_cfg, 0); // Bank 0 data

    // Configure FIFO mode, [7:6] -> 01 Stream-to-FIFO Mode
    if (icm_42688_write_reg(hw_cfg, FIFO_CONFIG, 0x01) != 0) return -1;

    // Configure data in FIFO
    uint8_t data = 0;
    switch (packet_structure) { 
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
    if (icm_42688_write_reg(hw_cfg, FIFO_CONFIG1, data) != 0) return -1;
    return 0;
}

int icm_42688_read_fifo(icm_42688_cfg_t* hw_cfg, uint8_t* gyro_data, uint8_t* accel_data, uint8_t* temp_data, uint8_t* time_data, uint8_t* extened_data) { 
    // This could be changed to read the header data instead of the packet variable (page 37) except for needing different read lengths

    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1; // Bank 0 data

    if (hw_cfg->packet_no == 0) { 
        return -1; // FIFO unconfigured
    } else if (hw_cfg->packet_no == 1) { 
        if (accel_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        uint8_t rx_data[8];
        if (spi_read_data(hw_cfg, FIFO_DATA, rx_data, 8) != 0) return -1;
        for (int i = 1; i < 7; i++) { 
            accel_data[i - 1] = rx_data[i];
        }
        temp_data[0] = rx_data[7];
    } else if (hw_cfg->packet_no == 2) { 
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        uint8_t rx_data[8];
        if (spi_read_data(hw_cfg, FIFO_DATA, rx_data, 8) != 0) return -1;
        for (int i = 1; i < 7; i++) { 
            gyro_data[i - 1] = rx_data[i];
        }
        temp_data[0] = rx_data[7];
    } else if (hw_cfg->packet_no == 3) { 
        if (accel_data == NULL) return -1;
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;

        uint8_t rx_data[16];
        if (spi_read_data(hw_cfg, FIFO_DATA, rx_data, 16) != 0) return -1;
        for (int i = 1; i < 7; i++) { 
            accel_data[i - 1] = rx_data[i];
        }
        for (int i = 7; i < 13; i++) { 
            gyro_data[i - 7] = rx_data[i];
        }
        temp_data[0] = rx_data[13];
        for (int i = 14; i < 16; i++) { 
            gyro_data[i - 14] = rx_data[i];
        }
    } else if (hw_cfg->packet_no == 4) { 
        if (accel_data == NULL) return -1;
        if (gyro_data == NULL) return -1;
        if (temp_data == NULL) return -1;
        if (extened_data == NULL) return -1;

        uint8_t rx_data[20];
        if (spi_read_data(hw_cfg, FIFO_DATA, rx_data, 20) != 0) return -1;
        for (int i = 1; i < 7; i++) { 
            accel_data[i - 1] = rx_data[i];
        }
        for (int i = 7; i < 13; i++) { 
            gyro_data[i - 7] = rx_data[i];
        }
        temp_data[0] = rx_data[13];
        temp_data[0] = rx_data[14];
        for (int i = 15; i < 17; i++) { 
            gyro_data[i - 15] = rx_data[i];
        }
        for (int i = 17; i < 20; i++) { 
            gyro_data[i - 17] = rx_data[i];
        }
    } else {
        return -1;
    }
    return 0;
}

int icm_42688_test_comms(icm_42688_cfg_t* hw_cfg) { 
    if (icm_42688_reset_device(hw_cfg) == -1) return -1;
    uint8_t rx_data[1];
    if (spi_read_data(hw_cfg, WHO_AM_I, rx_data, 1) == -1) return -1;
    if (rx_data[0] != 0x47) return -1;
    return 0;
}

int icm_42688_apex_pedometer(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config, uint8_t* settings) { 
    if (performance_mode > 2) return -1;    // Invalid selection

    return -1;
}

int icm_42688_apex_tilt_detection(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config) { 
    if (performance_mode > 2) return -1;    // Invalid selection
    return -1; // Function not yet implemented
}

int icm_42688_apex_raise_to_wake(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config, uint8_t wake_sleep) { 
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1111, ACCEL_CONFIG0, 0x0A, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b11, PWR_MGMT0, 0x02, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1, INTF_CONFIG1, 0x0, 3) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b11, APEX_CONFIG0, 0x02, 0) != 0) return -1;
    HAL_Delay(1);

    if (icm_42688_write_reg(hw_cfg, SIGNAL_PATH_RESET, 0x20) != 0) return -1;
    HAL_Delay(1);
    if (icm_42688_set_bank(hw_cfg, 4) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b111, APEX_CONFIG4, 0x07, 3) != 0) return -1;
    HAL_Delay(1);
    if (icm_42688_read_mod_write(hw_cfg, 0b111, APEX_CONFIG5, 0x07, 3) != 0) return -1;
    HAL_Delay(1);
    if (icm_42688_read_mod_write(hw_cfg, 0b111, APEX_CONFIG6, 0x07, 3) != 0) return -1;
    HAL_Delay(1);
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1, SIGNAL_PATH_RESET, 0x01, 6) != 0) return -1;

    if (icm_42688_set_bank(hw_cfg, 4) != 0) return -1;
    uint8_t data = 0b01; 
    if (wake_sleep == 0) { 
        data = 0b10;
    }
    if (interrupt_config == 1) { 
        if (icm_42688_read_mod_write(hw_cfg, data, INT_SOURCE6, data, 1) != 0) return -1;
    } else if (interrupt_config == 1) { 
        if (icm_42688_read_mod_write(hw_cfg, data, INT_SOURCE7, data, 1) != 0) return -1;
    }
    HAL_Delay(50);
    
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1, APEX_CONFIG0, 0x1, 3) != 0) return -1;
    return 0;
}

int icm_42688_apex_tap_detection(icm_42688_cfg_t* hw_cfg, uint8_t performance_mode, uint8_t interrupt_config) { 
    if (performance_mode > 3) return -1;    // Invalid selection

    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    uint8_t rx_data[1];
    if (icm_42688_read_reg(hw_cfg, ACCEL_CONFIG0, rx_data) != 0) return -1;
    uint8_t accel_odr = rx_data[0] & 0x0F;
    if ((accel_odr != 0x7) && (accel_odr != 0xF) && (accel_odr != 0x6)) { // Check for 200Hz, 500Hz, 1kHz
        if (icm_42688_write_reg(hw_cfg, ACCEL_CONFIG0, (rx_data[0] & 0xF0) | 0x0F) != 0) return -1;
        accel_odr = 0x0F;
    }
    if (accel_odr != 0x6) { 
        if (icm_42688_read_mod_write(hw_cfg, 0x11, PWR_MGMT0, 0x02, 0) != 0) return -1;
        if (icm_42688_read_mod_write(hw_cfg, 0b1, INTF_CONFIG1, 0x1, 3) != 0) return -1;
        if (icm_42688_read_mod_write(hw_cfg, 0b11, ACCEL_CONFIG1, 0x2, 1) != 0) return -1;
        if (icm_42688_read_mod_write(hw_cfg, 0b1111, GYRO_ACCEL_CONFIG0, 0x4, 4) != 0) return -1;
    } else {
        if (icm_42688_read_mod_write(hw_cfg, 0x11, PWR_MGMT0, 0x01, 0) != 0) return -1;
        if (icm_42688_read_mod_write(hw_cfg, 0b11, ACCEL_CONFIG1, 0x2, 3) != 0) return -1;
        if (icm_42688_read_mod_write(hw_cfg, 0b1111, GYRO_ACCEL_CONFIG0, 0x0, 4) != 0) return -1;
    }
    HAL_Delay(1);

    if (icm_42688_set_bank(hw_cfg, 4) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, APEX_CONFIG8, 0x5B) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, APEX_CONFIG7, 0x46) != 0) return -1;
    HAL_Delay(1);

    if (interrupt_config == 1) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b1, INT_SOURCE6, 0x01, 0) != 0) return -1;
    } else if (interrupt_config == 2) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b1, INT_SOURCE7, 0x01, 0) != 0) return -1;
    }
    HAL_Delay(50);

    if (icm_42688_read_mod_write(hw_cfg, 0b1, APEX_CONFIG0, 0x01, 6) != 0) return -1;
    return 0;
}

int icm_42688_apex_wake_on_motion(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config) { 
    // Initialize Sensor in a typical configuration
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1111, ACCEL_CONFIG0, 0x09, 0b0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b11, PWR_MGMT0, 0x02, 0b0) != 0) return -1;
    HAL_Delay(1);

    if (icm_42688_set_bank(hw_cfg, 4) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_X_THR, 98) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_Y_THR, 98) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_Z_THR, 98) != 0) return -1;
    HAL_Delay(1);

    // Enable interrupt source
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (interrupt_config == 1) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b111, INT_SOURCE1, 0x07, 0b0) != 0) return -1;
    } else if (interrupt_config == 2) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b111, INT_SOURCE4, 0x07, 0b0) != 0) return -1;
    }
    HAL_Delay(50);
    if (icm_42688_read_mod_write(hw_cfg, 0b1111, SMD_CONFIG, 0b0110, 0b0) != 0) return -1;
    return 0;
}

int icm_42688_apex_sig_motion_detect(icm_42688_cfg_t* hw_cfg, uint8_t interrupt_config) { 
    // Initialize Sensor in a typical configuration
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b1111, ACCEL_CONFIG0, 0x09, 0b0) != 0) return -1;
    if (icm_42688_read_mod_write(hw_cfg, 0b11, PWR_MGMT0, 0x02, 0b0) != 0) return -1;
    HAL_Delay(1);

    if (icm_42688_set_bank(hw_cfg, 4) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_X_THR, 98) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_Y_THR, 98) != 0) return -1;
    if (icm_42688_write_reg(hw_cfg, ACCEL_WOM_Z_THR, 98) != 0) return -1;
    HAL_Delay(1);

    // Enable interrupt source
    if (icm_42688_set_bank(hw_cfg, 0) != 0) return -1;
    if (interrupt_config == 1) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b1, INT_SOURCE1, 0x01, 3) != 0) return -1;
    } else if (interrupt_config == 2) { 
        if (icm_42688_read_mod_write(hw_cfg, 0b1, INT_SOURCE4, 0x01, 3) != 0) return -1;
    }
    HAL_Delay(50);
    if (icm_42688_read_mod_write(hw_cfg, 0b1111, SMD_CONFIG, 0b0111, 0b0) != 0) return -1;
    return 0;
}
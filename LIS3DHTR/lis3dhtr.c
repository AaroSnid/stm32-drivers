#include "lis3dhtr.h"
#include "lis3dhtr_registers.h"

static void cs_high(lis3dhtr_cfg_t* hw_cfg) { 
    if (hw_cfg->cs_port == NULL) return;
    HAL_GPIO_WritePin(hw_cfg->cs_port, hw_cfg->cs_pin, GPIO_PIN_SET);
}

static void cs_low(lis3dhtr_cfg_t* hw_cfg) { 
    if (hw_cfg->cs_port == NULL) return;
    HAL_GPIO_WritePin(hw_cfg->cs_port, hw_cfg->cs_pin, GPIO_PIN_RESET);
} 

static uint8_t build_spi_command(uint8_t register_address, bool read, uint8_t num_bytes) {
    uint8_t command = register_address & 0x3F;
    if (read == true) command |= 0x80;
    if (num_bytes > 1) command |= 0x40;
    return command;
}

static int spi_read_data(lis3dhtr_cfg_t *hw_cfg, uint8_t start_register, uint8_t* rx_data, uint8_t num_bytes){
    if (!hw_cfg || !rx_data || !hw_cfg->comms_handle || hw_cfg->cs_port == NULL || num_bytes == 0) return -1;

    uint8_t tx_buf[num_bytes + 1];
    uint8_t rx_buf[num_bytes + 1];
    tx_buf[0] = build_spi_command(start_register, true, num_bytes);
    for (uint8_t i = 1; i <= num_bytes; ++i) {
        tx_buf[i] = 0xFF;
    }

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive((SPI_HandleTypeDef*)hw_cfg->comms_handle, tx_buf, rx_buf, num_bytes + 1, HAL_MAX_DELAY);
    cs_high(hw_cfg);
    
    if (status != HAL_OK) {
        return -1;
    }

    for (uint8_t i = 0; i < num_bytes; ++i) {
        rx_data[i] = rx_buf[i + 1];
    }

    return 0;
}

static int spi_write_data(lis3dhtr_cfg_t *hw_cfg, uint8_t start_register, uint8_t* tx_data, uint8_t num_bytes){
    if (!hw_cfg || !tx_data || !hw_cfg->comms_handle || hw_cfg->cs_port == NULL || num_bytes == 0) return -1;

    uint8_t tx_buf[num_bytes + 1];
    tx_buf[0] = build_spi_command(start_register, false, num_bytes);
    for (uint8_t i = 0; i < num_bytes; ++i) {
        tx_buf[i + 1] = tx_data[i];
    }

    cs_low(hw_cfg);
    HAL_StatusTypeDef status = HAL_SPI_Transmit((SPI_HandleTypeDef*)hw_cfg->comms_handle, tx_buf, num_bytes + 1, HAL_MAX_DELAY);
    cs_high(hw_cfg);

    if (status != HAL_OK) {
        return -1;
    }

    return 0;
}

int lis3dh_init(lis3dhtr_cfg_t *hw_cfg, void *comms_handle, GPIO_TypeDef* cs_port, uint16_t cs_pin){
    
    // I2C support not yet implemented
    if (cs_port == NULL) return -1;

    hw_cfg->comms_handle = comms_handle;
    hw_cfg->cs_port = cs_port;
    hw_cfg->cs_pin = cs_pin;
    return 0;
}

int lis3dh_get_device_id(lis3dhtr_cfg_t *hw_cfg, uint8_t *device_id){
    if (spi_read_data(hw_cfg, WHO_AM_I, device_id, 1) != 0) return -1;
    return 0;
}

int lis3dh_configure_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_odr_t odr, lis3dh_sensitivity_t full_scale,
                          lis3dh_op_mode_t mode, uint8_t axes_en_mask){
    if (!hw_cfg) return -1;

    uint8_t new_reg_data;
    uint8_t low_power_en_bit = ((uint8_t)mode == 0) ? 1 : 0;

    // Write settings into CTRL_REG1
    new_reg_data =  ((uint8_t)odr       << 4) | 
                    (low_power_en_bit   << 3) | 
                    (axes_en_mask & 0x7)    ; // Ensure only 3 lower bits
    if (spi_write_data(hw_cfg, CTRL_REG1, &new_reg_data, 1) != 0) return -1;

    // Read modify write CTRL_REG4 for full_scale bits
    if (spi_read_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0xCF) | ((uint8_t)full_scale << 4);
    if (spi_write_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;

    return 0;
}

int lis3dh_set_high_pass_filter(lis3dhtr_cfg_t *hw_cfg, bool enable, lis3dh_high_pass_mode_t mode){
    if (!hw_cfg) return -1;
    return -1;
}

bool lis3dh_is_data_ready(lis3dhtr_cfg_t *hw_cfg){
    return false;
}

int lis3dh_read_raw_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *raw_data){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_read_g_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_g_data_t *g_data){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_get_status(lis3dhtr_cfg_t *hw_cfg, uint8_t *status){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_set_self_test(lis3dhtr_cfg_t *hw_cfg, bool enable, bool positive_sign){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_configure_sleep_to_wake(lis3dhtr_cfg_t *hw_cfg, bool enable, uint8_t threshold, uint8_t duration){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_enable_aux_adc(lis3dhtr_cfg_t *hw_cfg, bool enable){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_enable_temperature_sensor(lis3dhtr_cfg_t *hw_cfg, bool enable){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_read_adc_channel(lis3dhtr_cfg_t *hw_cfg, lis3dh_adc_channel_t channel, int16_t *adc_raw){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_read_temperature(lis3dhtr_cfg_t *hw_cfg, float *temperature_c){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_configure_orientation_detection(lis3dhtr_cfg_t *hw_cfg, bool enable_6d, bool disable_z_axis_4d){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_configure_fifo(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_mode_t mode, uint8_t watermark_level,
                          bool overrun_interrupt){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_get_fifo_status(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_status_t *status){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_read_fifo_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *fifo_samples,
                          uint8_t samples_to_read, uint8_t *samples_read){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_configure_interrupt(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t cfg_bits,
                                uint8_t threshold, uint8_t duration){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_read_interrupt_source(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t *source){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_configure_click(lis3dhtr_cfg_t *hw_cfg, uint8_t click_cfg, uint8_t click_ths,
                           uint8_t time_limit, uint8_t time_latency, uint8_t time_window){
    if (!hw_cfg) return -1;
                            return -1;
}

int lis3dh_read_click_source(lis3dhtr_cfg_t *hw_cfg, uint8_t *click_source){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_fifo_enable(lis3dhtr_cfg_t *hw_cfg){
    if (!hw_cfg) return -1;
    return -1;
}

int lis3dh_fifo_disable(lis3dhtr_cfg_t *hw_cfg){
    if (!hw_cfg) return -1;
    return -1;
}
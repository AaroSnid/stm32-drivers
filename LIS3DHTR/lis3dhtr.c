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
    uint8_t low_power_en_bit = ((uint8_t)mode == LIS3DH_MODE_LOW_POWER) ? 1U : 0U;
    uint8_t high_resolution_bit = ((uint8_t)mode == LIS3DH_MODE_HIGH_RES) ? 1U : 0U;

    // Write settings into CTRL_REG1
    new_reg_data =  ((uint8_t)odr       << 4) | 
                    (low_power_en_bit   << 3) | 
                    (axes_en_mask       & 0x7U); // Ensure only 3 lower bits
    if (spi_write_data(hw_cfg, CTRL_REG1, &new_reg_data, 1) != 0) return -1;

    // Read-modify-write CTRL_REG4 for FS[1:0] and HR bits.
    if (spi_read_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0xC7U) |
                   (((uint8_t)full_scale & 0x3U) << 4) |
                   (high_resolution_bit << 3);
    if (spi_write_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;

    return 0;
}

int lis3dh_set_high_pass_filter(lis3dhtr_cfg_t *hw_cfg, bool enable, lis3dh_high_pass_mode_t mode){
    if (!hw_cfg) return -1;

    uint8_t reg_data = 0;
    if (spi_read_data(hw_cfg, CTRL_REG2, &reg_data, 1) != 0) return -1;

    reg_data &= 0x1FU;
    reg_data |= ((uint8_t)mode & 0x3U) << 6;
    if (enable) {
        reg_data |= 0x20U;
    }

    if (spi_write_data(hw_cfg, CTRL_REG2, &reg_data, 1) != 0) return -1;
    return 0;
}

bool lis3dh_is_data_ready(lis3dhtr_cfg_t *hw_cfg){
    uint8_t data = 0;
    if (lis3dh_get_status(hw_cfg, &data) != 0) return false;

    return ((data & 0xF) > 0) ? true : false;
}

int lis3dh_read_raw_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *raw_data){
    if ((!hw_cfg) || (!raw_data)) return -1;

    uint8_t rx_buffer[6];
    if (spi_read_data(hw_cfg, OUT_X_L, rx_buffer, 6) != 0) return -1;

    raw_data->x = (int16_t)(((uint16_t)rx_buffer[1] << 8) | (uint16_t)rx_buffer[0]);
    raw_data->y = (int16_t)(((uint16_t)rx_buffer[3] << 8) | (uint16_t)rx_buffer[2]);
    raw_data->z = (int16_t)(((uint16_t)rx_buffer[5] << 8) | (uint16_t)rx_buffer[4]);

    return 0;
}

int lis3dh_read_g_acceleration(lis3dhtr_cfg_t *hw_cfg, lis3dh_g_data_t *g_data){
    if (!hw_cfg || !g_data) return -1;

    lis3dh_raw_data_t raw_data;
    uint8_t ctrl_reg4 = 0;
    const float sensitivity_mg_per_lsb[4] = {1.0f, 2.0f, 4.0f, 12.0f};

    if (lis3dh_read_raw_acceleration(hw_cfg, &raw_data) != 0) return -1;

    if (spi_read_data(hw_cfg, CTRL_REG4, &ctrl_reg4, 1) != 0) return -1;

    const uint8_t full_scale_bits = (ctrl_reg4 >> 4) & 0x03;
    const float sensitivity_g_per_lsb = sensitivity_mg_per_lsb[full_scale_bits] / 1000.0f;

    g_data->x_g = (float)raw_data.x * sensitivity_g_per_lsb;
    g_data->y_g = (float)raw_data.y * sensitivity_g_per_lsb;
    g_data->z_g = (float)raw_data.z * sensitivity_g_per_lsb;

    return 0;
}

int lis3dh_get_status(lis3dhtr_cfg_t *hw_cfg, uint8_t *status){
    if (spi_read_data(hw_cfg, STATUS_REG, status, 1) != 0) return -1;
    return 0;
}

int lis3dh_get_aux_status(lis3dhtr_cfg_t *hw_cfg, uint8_t *status){
    if (spi_read_data(hw_cfg, STATUS_REG_AUX, status, 1) != 0) return -1;
    return 0;
}

int lis3dh_set_self_test(lis3dhtr_cfg_t *hw_cfg, bool enable, bool positive_sign){
    if (!hw_cfg) return -1;

    uint8_t reg_data = 0;
    if (spi_read_data(hw_cfg, CTRL_REG4, &reg_data, 1) != 0) return -1;

    reg_data &= 0xF9U;
    if (enable) {
        reg_data |= (positive_sign ? 0x04U : 0x02U);
    }

    if (spi_write_data(hw_cfg, CTRL_REG4, &reg_data, 1) != 0) return -1;
    return 0;
}

int lis3dh_configure_sleep_to_wake(lis3dhtr_cfg_t *hw_cfg, bool enable, uint8_t threshold, uint8_t duration){
    if (!hw_cfg) return -1;

    if (enable == true){
        uint8_t lower_threshold_bits = threshold & 0x7F;

        // Write user settings
        if (spi_write_data(hw_cfg, ACT_THS, &lower_threshold_bits, 1) != 0) return -1;
        if (spi_write_data(hw_cfg, ACT_DUR, &duration, 1) != 0) return -1;

    } else {
        uint8_t empty_bits = 0;

        // Write default value into registers
        if (spi_write_data(hw_cfg, ACT_THS, &empty_bits, 1) != 0) return -1;
        if (spi_write_data(hw_cfg, ACT_DUR, &empty_bits, 1) != 0) return -1;
    }
    return 0;
}

int lis3dh_enable_aux_adc(lis3dhtr_cfg_t *hw_cfg, bool enable){

    uint8_t new_reg_data;
    if (spi_read_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0x7FU) | ((uint8_t)enable << 7);
    if (spi_write_data(hw_cfg, CTRL_REG4, &new_reg_data, 1) != 0) return -1;

    if (spi_read_data(hw_cfg, TEMP_CFG_REG, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0x7FU) | ((uint8_t)enable << 7);
    if (spi_write_data(hw_cfg, TEMP_CFG_REG, &new_reg_data, 1) != 0) return -1;

    return 0;
}

int lis3dh_enable_temperature_sensor(lis3dhtr_cfg_t *hw_cfg, bool enable){
    
    uint8_t new_reg_data;

    if (spi_read_data(hw_cfg, TEMP_CFG_REG, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0xBFU) | ((uint8_t)enable << 6);
    if (spi_write_data(hw_cfg, TEMP_CFG_REG, &new_reg_data, 1) != 0) return -1;

    return 0;
}

int lis3dh_read_adc_channel(lis3dhtr_cfg_t *hw_cfg, lis3dh_adc_channel_t channel, int16_t *adc_raw){
    if (!hw_cfg || !adc_raw) return -1;

    uint8_t rx_buffer[2];

    if (spi_read_data(hw_cfg, (OUT_ADC1_L + (uint8_t)channel), rx_buffer, 2) != 0) return -1;

    *adc_raw = (int16_t)((((uint16_t)rx_buffer[1]) << 8) | (uint16_t)rx_buffer[0]) >> 6;
    return 0;
}

int lis3dh_read_temperature(lis3dhtr_cfg_t *hw_cfg, float *temperature_c){
    if (!hw_cfg || !temperature_c) return -1;

    uint8_t rx_buffer[2];
    if (spi_read_data(hw_cfg, OUT_ADC3_L, rx_buffer, 2) != 0) return -1;

    // Resolves raw left-justified ADC register value, conversions step dynamic mapping goes here
    int16_t raw_temp = (int16_t)((((uint16_t)rx_buffer[1]) << 8) | (uint16_t)rx_buffer[0]) >> 6;
    *temperature_c = (float)raw_temp; 
    return 0;
}

int lis3dh_configure_orientation_detection(lis3dhtr_cfg_t *hw_cfg, bool enable_6d, bool disable_z_axis_4d){
    if (!hw_cfg) return -1;

    uint8_t reg_data = 0;
    if (spi_read_data(hw_cfg, CTRL_REG5, &reg_data, 1) != 0) return -1;

    reg_data &= 0xF7U;
    if (enable_6d) {
        reg_data |= 0x08U;
    }
    if (disable_z_axis_4d) {
        reg_data |= 0x10U;
    }

    if (spi_write_data(hw_cfg, CTRL_REG5, &reg_data, 1) != 0) return -1;
    return 0;
}

int lis3dh_configure_fifo(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_mode_t mode, uint8_t watermark_level,
                          bool overrun_interrupt){
    uint8_t new_reg_data = ((uint8_t)mode << 6) | ((uint8_t)overrun_interrupt << 5) | (watermark_level & 0x1F);
    if (spi_write_data(hw_cfg, FIFO_CTRL_REG, &new_reg_data, 1) != 0) return -1;
    return 0;
}

int lis3dh_get_fifo_status(lis3dhtr_cfg_t *hw_cfg, lis3dh_fifo_status_t *status){
    if (!hw_cfg || !status) return -1;

    uint8_t reg_read_data = 0;
    if (spi_read_data(hw_cfg, FIFO_SRC_REG, &reg_read_data, 1) != 0) return -1;

    status->watermark = (reg_read_data & 0x80) ? true : false;
    status->overrun   = (reg_read_data & 0x40) ? true : false;
    status->empty     = (reg_read_data & 0x20) ? true : false;
    status->fill_level = reg_read_data & 0x1F;

    return 0;
}

int lis3dh_read_raw_fifo_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_raw_data_t *fifo_samples,
                             uint8_t samples_to_read, uint8_t *samples_read){
    if (!hw_cfg || !fifo_samples || !samples_read) return -1;

    lis3dh_fifo_status_t fifo_status = {0};
    if (lis3dh_get_fifo_status(hw_cfg, &fifo_status) != 0) return -1;
    
    if (fifo_status.empty) {
        *samples_read = 0;
        return 0;
    }

    uint8_t sample_number = 0;
    while ((sample_number < fifo_status.fill_level) && (sample_number < samples_to_read)) {
        if (lis3dh_read_raw_acceleration(hw_cfg, (fifo_samples + sample_number)) != 0) return -1;
        sample_number++;
    }
    *samples_read = sample_number;

    return 0;
}

int lis3dh_read_g_fifo_data(lis3dhtr_cfg_t *hw_cfg, lis3dh_g_data_t *fifo_samples,
                           uint8_t samples_to_read, uint8_t *samples_read){
    if (!hw_cfg || !fifo_samples || !samples_read) return -1;

    lis3dh_fifo_status_t fifo_status = {0};
    if (lis3dh_get_fifo_status(hw_cfg, &fifo_status) != 0) return -1;
    
    if (fifo_status.empty) {
        *samples_read = 0;
        return 0;
    }

    uint8_t sample_number = 0;
    while ((sample_number < fifo_status.fill_level) && (sample_number < samples_to_read)) {
        if (lis3dh_read_g_acceleration(hw_cfg, (fifo_samples + sample_number)) != 0) return -1;
        sample_number++;
    }
    *samples_read = sample_number;

    return 0;
}

int lis3dh_configure_interrupt(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t cfg_bits,
                               uint8_t threshold, uint8_t duration){
    if (int_pin == LIS3DH_INT_PIN_1){

        if (spi_write_data(hw_cfg, INT1_CFG, &cfg_bits, 1) != 0) return -1;

        if (spi_write_data(hw_cfg, INT1_THS, &threshold, 1) != 0) return -1;
        if (spi_write_data(hw_cfg, INT1_DURATION, &duration, 1) != 0) return -1;

    } else {

        if (spi_write_data(hw_cfg, INT2_CFG, &cfg_bits, 1) != 0) return -1;

        if (spi_write_data(hw_cfg, INT2_THS, &threshold, 1) != 0) return -1;
        if (spi_write_data(hw_cfg, INT2_DURATION, &duration, 1) != 0) return -1;
    }

    return 0;
}

int lis3dh_read_interrupt_source(lis3dhtr_cfg_t *hw_cfg, lis3dh_int_pin_t int_pin, uint8_t *source){
    
    if (int_pin == LIS3DH_INT_PIN_1){

        if (spi_read_data(hw_cfg, INT1_SRC, source, 1) != 0) return -1;

    } else {

        if (spi_read_data(hw_cfg, INT2_SRC, source, 1) != 0) return -1;
    }

    return 0;
}

int lis3dh_configure_click(lis3dhtr_cfg_t *hw_cfg, uint8_t click_cfg, uint8_t click_ths,
                           uint8_t time_limit, uint8_t time_latency, uint8_t time_window){
    
    uint8_t new_reg_data = click_cfg & 0x3F;

    if (spi_write_data(hw_cfg, CLICK_CFG, &new_reg_data, 1) != 0) return -1;

    new_reg_data = (click_ths & 0x7FU);
    if (spi_write_data(hw_cfg, CLICK_THS, &new_reg_data, 1) != 0) return -1;

    new_reg_data = (time_limit & 0x7FU);
    if (spi_write_data(hw_cfg, TIME_LIMIT, &new_reg_data, 1) != 0) return -1;

    if (spi_write_data(hw_cfg, TIME_LATENCY, &time_latency, 1) != 0) return -1;

    if (spi_write_data(hw_cfg, TIME_WINDOW, &time_window, 1) != 0) return -1;
    
    return 0;
}

int lis3dh_read_click_source(lis3dhtr_cfg_t *hw_cfg, uint8_t *click_source){
    if (spi_read_data(hw_cfg, CLICK_SRC, click_source, 1) != 0) return -1;
    return 0;
}

int lis3dh_fifo_control(lis3dhtr_cfg_t *hw_cfg, bool enable){
    uint8_t new_reg_data;

    if (spi_read_data(hw_cfg, CTRL_REG5, &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0xBFU) | ((uint8_t)enable << 6);
    if (spi_write_data(hw_cfg, CTRL_REG5, &new_reg_data, 1) != 0) return -1;

    return 0;
}

int lis3dh_control_pull_up(lis3dhtr_cfg_t *hw_cfg, bool enable){
    uint8_t new_reg_data = 0;
    if (spi_read_data(hw_cfg, CTRL_REG0 , &new_reg_data, 1) != 0) return -1;
    new_reg_data = (new_reg_data & 0x7FU) | ((uint8_t)enable << 7);
    if (spi_write_data(hw_cfg, CTRL_REG0 , &new_reg_data, 1) != 0) return -1;

    return 0;
}
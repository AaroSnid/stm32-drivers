#include "W25Q64JV_registers.h"
#include "W25Q64JV.h"

typedef struct {
    void* comms_handle;

} icm_42688_cfg_t;

int write_enable();
int volatile_sr_write_enable();
int write_disable();
int release_power_down_id();
int manufacturer_device_id();
int jedec_id();
int read_unique_id();
int read_data();
int fast_read();
int page_program();
int sector_erase_4KB();
int block_erase_32KB();
int block_erase_64KB();
int chip_erase();
int read_status_register_1();
int write_status_register_1();
int read_status_register_2();
int write_status_register_2();
int read_status_register_3();
int write_status_register_3();
int read_sfdp_register();
int erase_security_register();
int program_security_register();
int read_security_register();
int global_block_lock();
int global_block_unlock();
int read_block_lock();
int individual_block_lock();
int individual_block_unlock();
int erase_program_suspend();
int erase_program_resume();
int power_down();
int enable_reset();
int reset_device();

int fast_read_dual_output();
int fast_read_dual_io();
int mftr_device_id_dual_io();
int quad_input_page_program();
int fast_read_quad_output();
int mftr_device_id_quad_io();
int fast_read_quad_io();
int set_burst_with_wrap();
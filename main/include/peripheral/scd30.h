#pragma once
#ifndef _SCD30_H_
#define _SCD30_H_

#include "I2CMaster.h"
#include "definition.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCD30_I2C_ADDR          0x61

#define SCD30_CMD_TRIGGER_CONTINUOUS_MEASUREMENT    0x0010
#define SCD30_CMD_STOP_CONTINUOUS_MEASUREMENT       0x0104
#define SCD30_CMD_SET_MEASUREMENT_INTERVAL          0x4600
#define SCD30_CMD_GET_DATA_READY_STATUS             0x0202
#define SCD30_CMD_READ_MEASUREMENT                  0x0300
#define SCD30_CMD_SET_TEMPERATURE_OFFSET            0x5403
#define SCD30_CMD_ALTITUDE_COMPENSATION             0x5102
#define SCD30_CMD_READ_FIRMWARE_VERSION             0xD100
#define SCD30_CMD_SOFT_RESET                        0xD304

class CScd30Ctrl
{
public:
    CScd30Ctrl();
    virtual ~CScd30Ctrl();
    static CScd30Ctrl* Instance();

public:
    bool initialize(CI2CMaster *i2c_master, bool self_test = false);
    bool release();

    bool start_periodic_measure(uint16_t pressure_compensation = 0);
    bool stop_periodic_measure();
    
    bool set_measurement_interval(uint16_t interval_seconds);
    bool get_measurement_interval(uint16_t *interval_seconds);
    
    bool set_temperature_offset(float offset_celsius);
    bool get_temperature_offset(float *offset_celsius);
    
    bool set_altitude_compensation(uint16_t altitude_meters);
    
    bool read_firmware_version(uint16_t *version);
    bool soft_reset();

    bool read_measurement(float *co2, float *temperature, float *humidity);
    bool is_measurement_data_ready();

private:
    static CScd30Ctrl *_instance;
    CI2CMaster *m_i2c_master;
    bool m_initialized;

    uint8_t calculate_crc8(const uint8_t *data, size_t count);
    bool send_command(uint16_t cmd, uint16_t *args = nullptr, size_t arg_count = 0);
    bool read_response(uint16_t *data, size_t word_count);
    bool execute_command(uint16_t cmd, uint16_t *args, size_t arg_count, 
                        uint16_t *response, size_t response_count, uint32_t delay_ms = 0);
};

inline CScd30Ctrl* GetScd30Ctrl() {
    return CScd30Ctrl::Instance();
}

#ifdef __cplusplus
}
#endif
#endif

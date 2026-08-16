#include "scd30.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

CScd30Ctrl* CScd30Ctrl::_instance = nullptr;

CScd30Ctrl::CScd30Ctrl()
{
    m_i2c_master = nullptr;
    m_initialized = false;
}

CScd30Ctrl::~CScd30Ctrl()
{

}

CScd30Ctrl* CScd30Ctrl::Instance()
{
    if (!_instance) {
        _instance = new CScd30Ctrl();
    }

    return _instance;
}

uint8_t CScd30Ctrl::calculate_crc8(const uint8_t *data, size_t count)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < count; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

bool CScd30Ctrl::send_command(uint16_t cmd, uint16_t *args, size_t arg_count)
{
    uint8_t buf[2 + arg_count * 3];
    buf[0] = (uint8_t)(cmd >> 8);
    buf[1] = (uint8_t)(cmd & 0xFF);

    for (size_t i = 0; i < arg_count; i++) {
        uint8_t *p = buf + 2 + i * 3;
        p[0] = (uint8_t)(args[i] >> 8);
        p[1] = (uint8_t)(args[i] & 0xFF);
        p[2] = calculate_crc8(p, 2);
    }

    return m_i2c_master->write_bytes(SCD30_I2C_ADDR, buf, sizeof(buf));
}

bool CScd30Ctrl::read_response(uint16_t *data, size_t word_count)
{
    uint8_t buf[word_count * 3];
    if (!m_i2c_master->read_bytes(SCD30_I2C_ADDR, buf, sizeof(buf))) {
        return false;
    }

    for (size_t i = 0; i < word_count; i++) {
        uint8_t *p = buf + i * 3;
        uint8_t crc = calculate_crc8(p, 2);
        if (crc != p[2]) {
            GetLogger(eLogType::Error)->Log("CRC mismatch: expected 0x%02X, got 0x%02X", crc, p[2]);
            return false;
        }
        data[i] = ((uint16_t)p[0] << 8) | p[1];
    }

    return true;
}

bool CScd30Ctrl::execute_command(uint16_t cmd, uint16_t *args, size_t arg_count,
                                  uint16_t *response, size_t response_count, uint32_t delay_ms)
{
    if (!m_i2c_master) {
        GetLogger(eLogType::Error)->Log("I2C master not initialized");
        return false;
    }

    if (!send_command(cmd, args, arg_count)) {
        return false;
    }

    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    if (response && response_count > 0) {
        return read_response(response, response_count);
    }

    return true;
}

bool CScd30Ctrl::initialize(CI2CMaster *i2c_master, bool self_test)
{
    m_i2c_master = i2c_master;

#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Warning)->Log("SCD30 initialized in DUMMY DATA mode");
    m_initialized = true;
    return true;
#else
    uint16_t fw_version = 0;
    if (!read_firmware_version(&fw_version)) {
        GetLogger(eLogType::Error)->Log("Failed to read firmware version");
        return false;
    }
    GetLogger(eLogType::Info)->Log("SCD30 firmware version: %d.%d", 
                                   (fw_version >> 8) & 0xFF, fw_version & 0xFF);

    if (!soft_reset()) {
        GetLogger(eLogType::Warning)->Log("Soft reset failed");
    }

    m_initialized = true;
    GetLogger(eLogType::Info)->Log("SCD30 initialized successfully");
    return true;
#endif
}

bool CScd30Ctrl::release()
{
    stop_periodic_measure();
    m_initialized = false;
    return true;
}

bool CScd30Ctrl::start_periodic_measure(uint16_t pressure_compensation)
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Started periodic measurement (dummy)");
    return true;
#else
    if (!m_initialized) {
        GetLogger(eLogType::Error)->Log("Not initialized");
        return false;
    }

    if (pressure_compensation != 0 && (pressure_compensation < 700 || pressure_compensation > 1400)) {
        GetLogger(eLogType::Error)->Log("Invalid pressure compensation: %d (must be 0 or 700-1400)", pressure_compensation);
        return false;
    }

    return execute_command(SCD30_CMD_TRIGGER_CONTINUOUS_MEASUREMENT, &pressure_compensation, 1, nullptr, 0);
#endif
}

bool CScd30Ctrl::stop_periodic_measure()
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Stopped periodic measurement (dummy)");
    return true;
#else
    if (!m_initialized) {
        return false;
    }
    return execute_command(SCD30_CMD_STOP_CONTINUOUS_MEASUREMENT, nullptr, 0, nullptr, 0, 1);
#endif
}

bool CScd30Ctrl::set_measurement_interval(uint16_t interval_seconds)
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Set measurement interval to %d seconds (dummy)", interval_seconds);
    return true;
#else
    if (interval_seconds < 2 || interval_seconds > 1800) {
        GetLogger(eLogType::Error)->Log("Invalid interval: %d (must be 2-1800)", interval_seconds);
        return false;
    }
    return execute_command(SCD30_CMD_SET_MEASUREMENT_INTERVAL, &interval_seconds, 1, nullptr, 0);
#endif
}

bool CScd30Ctrl::get_measurement_interval(uint16_t *interval_seconds)
{
#if SCD30_USE_DUMMY_DATA
    if (interval_seconds) *interval_seconds = 2;
    return true;
#else
    if (!interval_seconds) return false;
    return execute_command(SCD30_CMD_SET_MEASUREMENT_INTERVAL, nullptr, 0, interval_seconds, 1);
#endif
}

bool CScd30Ctrl::set_temperature_offset(float offset_celsius)
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Set temperature offset to %.2f C (dummy)", offset_celsius);
    return true;
#else
    uint16_t offset_ticks = (uint16_t)(offset_celsius * 100.0f);
    return execute_command(SCD30_CMD_SET_TEMPERATURE_OFFSET, &offset_ticks, 1, nullptr, 0);
#endif
}

bool CScd30Ctrl::get_temperature_offset(float *offset_celsius)
{
#if SCD30_USE_DUMMY_DATA
    if (offset_celsius) *offset_celsius = 0.0f;
    return true;
#else
    if (!offset_celsius) return false;
    uint16_t offset_ticks;
    if (!execute_command(SCD30_CMD_SET_TEMPERATURE_OFFSET, nullptr, 0, &offset_ticks, 1)) {
        return false;
    }
    *offset_celsius = (float)offset_ticks / 100.0f;
    return true;
#endif
}

bool CScd30Ctrl::set_altitude_compensation(uint16_t altitude_meters)
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Set altitude to %d meters (dummy)", altitude_meters);
    return true;
#else
    return execute_command(SCD30_CMD_ALTITUDE_COMPENSATION, &altitude_meters, 1, nullptr, 0);
#endif
}

bool CScd30Ctrl::read_firmware_version(uint16_t *version)
{
#if SCD30_USE_DUMMY_DATA
    if (version) *version = 0x0142;
    return true;
#else
    if (!version) return false;
    return execute_command(SCD30_CMD_READ_FIRMWARE_VERSION, nullptr, 0, version, 1);
#endif
}

bool CScd30Ctrl::soft_reset()
{
#if SCD30_USE_DUMMY_DATA
    GetLogger(eLogType::Info)->Log("Soft reset (dummy)");
    return true;
#else
    return execute_command(SCD30_CMD_SOFT_RESET, nullptr, 0, nullptr, 0);
#endif
}

bool CScd30Ctrl::is_measurement_data_ready()
{
#if SCD30_USE_DUMMY_DATA
    static uint32_t last_tick = 0;
    uint32_t current_tick = xTaskGetTickCount();
    if ((current_tick - last_tick) >= pdMS_TO_TICKS(60000)) {
        last_tick = current_tick;
        return true;
    }
    return false;
#else
    if (!m_initialized) return false;
    
    uint16_t status;
    if (!execute_command(SCD30_CMD_GET_DATA_READY_STATUS, nullptr, 0, &status, 1)) {
        return false;
    }
    return status != 0;
#endif
}

bool CScd30Ctrl::read_measurement(float *co2, float *temperature, float *humidity)
{
#if SCD30_USE_DUMMY_DATA
    static float dummy_co2 = 450.0f;
    static float dummy_temp = 23.5f;
    static float dummy_hum = 45.0f;

    dummy_co2 += ((float)(rand() % 20) - 10.0f) * 0.5f;
    if (dummy_co2 < 400.0f) dummy_co2 = 400.0f;
    if (dummy_co2 > 2000.0f) dummy_co2 = 2000.0f;

    dummy_temp += ((float)(rand() % 10) - 5.0f) * 0.02f;
    if (dummy_temp < 15.0f) dummy_temp = 15.0f;
    if (dummy_temp > 35.0f) dummy_temp = 35.0f;

    dummy_hum += ((float)(rand() % 10) - 5.0f) * 0.1f;
    if (dummy_hum < 20.0f) dummy_hum = 20.0f;
    if (dummy_hum > 80.0f) dummy_hum = 80.0f;

    if (co2) *co2 = dummy_co2;
    if (temperature) *temperature = dummy_temp;
    if (humidity) *humidity = dummy_hum;

    return true;
#else
    if (!m_initialized) {
        GetLogger(eLogType::Error)->Log("Not initialized");
        return false;
    }

    uint16_t buf[6];
    if (!execute_command(SCD30_CMD_READ_MEASUREMENT, nullptr, 0, buf, 6)) {
        return false;
    }

    union {
        uint32_t u32;
        float f;
    } temp;

    if (co2) {
        temp.u32 = ((uint32_t)buf[0] << 16) | buf[1];
        *co2 = temp.f;
    }
    if (temperature) {
        temp.u32 = ((uint32_t)buf[2] << 16) | buf[3];
        *temperature = temp.f;
    }
    if (humidity) {
        temp.u32 = ((uint32_t)buf[4] << 16) | buf[5];
        *humidity = temp.f;
    }

    return true;
#endif
}

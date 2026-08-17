#include "temperaturesensor.h"
#include "system.h"
#include "logger.h"

CTemperatureSensor::CTemperatureSensor()
{
    m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
}

bool CTemperatureSensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::temperature_sensor::config_t config_endpoint;
    config_endpoint.temperature_measurement.measured_value = (int16_t)2350;       //  23.50 C (initial)
    config_endpoint.temperature_measurement.min_measured_value = (int16_t)-4000;  // -40.00 C (SCD30 range)
    config_endpoint.temperature_measurement.max_measured_value = (int16_t)7000;   //  70.00 C
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::temperature_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }
    return CDevice::matter_init_endpoint();
}

void CTemperatureSensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::TemperatureMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_tempmeasure_attr_measureval) {
                m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
            }
        }
    }
}

void CTemperatureSensor::update_measured_value_temperature(float value)
{
    m_measured_value_temperature = (int16_t)(value * 100.f);
    if (m_measured_value_temperature_prev != m_measured_value_temperature) {
        GetLogger(eLogType::Info)->Log("Update measured temperature value as %g", value);
        matter_update_clus_tempmeasure_attr_measureval();
    }
    m_measured_value_temperature_prev = m_measured_value_temperature;
}

void CTemperatureSensor::matter_update_clus_tempmeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_int16(m_measured_value_temperature);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::TemperatureMeasurement::Id,
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_tempmeasure_attr_measureval,
        force_update
    );
}
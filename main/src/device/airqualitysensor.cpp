#include "airqualitysensor.h"
#include "system.h"
#include "logger.h"

CAirQualitySensor::CAirQualitySensor()
{
    m_matter_update_by_client_clus_co2measure_attr_measureval = false;
    m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
    m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
    m_air_quality = 1; // Good
}

bool CAirQualitySensor::matter_init_endpoint()
{
    esp_matter::node_t *root = GetSystem()->get_root_node();
    esp_matter::endpoint::air_quality_sensor::config_t config_endpoint;
    config_endpoint.air_quality.air_quality = 1; // Good (initial value)
    uint8_t flags = esp_matter::ENDPOINT_FLAG_DESTROYABLE;
    m_endpoint = esp_matter::endpoint::air_quality_sensor::create(root, &config_endpoint, flags, nullptr);
    if (!m_endpoint) {
        GetLogger(eLogType::Error)->Log("Failed to create endpoint");
        return false;
    }
    return CDevice::matter_init_endpoint();
}

bool CAirQualitySensor::matter_config_attributes()
{
    esp_matter::cluster_t *cluster;

    // enable all air quality level features (required by Matter spec
    // to use Fair/Moderate/VeryPoor/ExtremelyPoor enum values)
    cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::AirQuality::Id);
    if (cluster) {
        esp_matter::cluster::air_quality::feature::fair::add(cluster);
        esp_matter::cluster::air_quality::feature::moderate::add(cluster);
        esp_matter::cluster::air_quality::feature::very_poor::add(cluster);
        esp_matter::cluster::air_quality::feature::extremely_poor::add(cluster);
        GetLogger(eLogType::Info)->Log("AirQuality cluster configured with all level features");
    } else {
        GetLogger(eLogType::Warning)->Log("AirQuality cluster not found");
    }

    if (!create_temperature_measurement_cluster()) {
        GetLogger(eLogType::Error)->Log("Failed to create temperature cluster");
    }
    if (!create_relative_humidity_measurement_cluster()) {
        GetLogger(eLogType::Error)->Log("Failed to create humidity cluster");
    }
    if (!create_carbon_dioxide_concentration_measurement_cluster()) {
        GetLogger(eLogType::Error)->Log("Failed to create CO2 cluster");
    }

    return true;
}

bool CAirQualitySensor::create_temperature_measurement_cluster()
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::TemperatureMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::temperature_measurement::config_t cfg_tempmeasure_cluster;
        cfg_tempmeasure_cluster.measured_value = (int16_t)2350;       //  23.50 C (initial)
        cfg_tempmeasure_cluster.min_measured_value = (int16_t)-4000;  // -40.00 C (SCD30 range)
        cfg_tempmeasure_cluster.max_measured_value = (int16_t)7000;   //  70.00 C
        cluster = esp_matter::cluster::temperature_measurement::create(m_endpoint, &cfg_tempmeasure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Temperature Measurement> cluster");
            return false;
        }
    }

    return true;
}

bool CAirQualitySensor::create_relative_humidity_measurement_cluster()
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::RelativeHumidityMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::relative_humidity_measurement::config_t cfg_relhummeasure_cluster;
        cfg_relhummeasure_cluster.measured_value = (uint16_t)4500;     // 45.00 % (initial)
        cfg_relhummeasure_cluster.min_measured_value = (uint16_t)0;      // 0.00 %
        cfg_relhummeasure_cluster.max_measured_value = (uint16_t)10000;  // 100.00 %
        cluster = esp_matter::cluster::relative_humidity_measurement::create(m_endpoint, &cfg_relhummeasure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Relative Humidity Measurement> cluster");
            return false;
        }
    }

    return true;
}

bool CAirQualitySensor::create_carbon_dioxide_concentration_measurement_cluster()
{
    esp_matter::cluster_t *cluster;

    cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        esp_matter::cluster::carbon_dioxide_concentration_measurement::config_t cfg_co2measure_cluster;
        cfg_co2measure_cluster.feature_flags = esp_matter::cluster::carbon_dioxide_concentration_measurement::feature::numeric_measurement::get_id();
        cluster = esp_matter::cluster::carbon_dioxide_concentration_measurement::create(m_endpoint, &cfg_co2measure_cluster, esp_matter::cluster_flags::CLUSTER_FLAG_SERVER);
        if (!cluster) {
            GetLogger(eLogType::Error)->Log("Failed to create <Carbon Dioxide Concentration Measurement> cluster");
            return false;
        }
        // numeric_measurement feature auto-creates MeasuredValue, MinMeasuredValue,
        // MaxMeasuredValue and MeasurementUnit attributes
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_min_measured_value(float value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MinMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MinMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_float(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MinMeasuredValue attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_max_measured_value(float value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MaxMeasuredValue::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MaxMeasuredValue attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_nullable_float(value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Warning)->Log("Failed to set MaxMeasuredValue attribute value (ret: %d)", ret);
    }

    return true;
}

bool CAirQualitySensor::set_carbon_dioxide_concentration_measurement_measurement_unit(int value)
{
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(m_endpoint, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id);
    if (!cluster) {
        GetLogger(eLogType::Error)->Log("Failed to get CarbonDioxideConcentrationMeasurement cluster");
        return false;
    }
    esp_matter::attribute_t *attribute = esp_matter::attribute::get(cluster, chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasurementUnit::Id);
    if (!attribute) {
        GetLogger(eLogType::Error)->Log("Failed to get MeasurementUnit attribute");
        return false;
    }
    esp_matter_attr_val_t val = esp_matter_enum8((uint8_t)value);
    esp_err_t ret = esp_matter::attribute::set_val(attribute, &val);
    if (ret != ESP_OK) {
        GetLogger(eLogType::Error)->Log("Failed to set MeasurementUnit attribute value (ret: %d)", ret);
        return false;
    }

    return true;
}

void CAirQualitySensor::matter_on_change_attribute_value(esp_matter::attribute::callback_type_t type, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *value)
{
    if (cluster_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_co2measure_attr_measureval) {
                m_matter_update_by_client_clus_co2measure_attr_measureval = false;
            }
        }
    } else if (cluster_id == chip::app::Clusters::TemperatureMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_tempmeasure_attr_measureval) {
                m_matter_update_by_client_clus_tempmeasure_attr_measureval = false;
            }
        }
    } else if (cluster_id == chip::app::Clusters::RelativeHumidityMeasurement::Id) {
        if (attribute_id == chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id) {
            if (m_matter_update_by_client_clus_relhummeasure_attr_measureval) {
                m_matter_update_by_client_clus_relhummeasure_attr_measureval = false;
            }
        }
    }
}

void CAirQualitySensor::matter_update_all_attribute_values()
{
    matter_update_clus_co2measure_attr_measureval();
    matter_update_clus_tempmeasure_attr_measureval();
    matter_update_clus_relhummeasure_attr_measureval();
}

void CAirQualitySensor::update_measured_value_co2ppm(float value)
{
    m_measured_value_co2ppm = value;
    if (m_measured_value_co2ppm != m_measured_value_co2ppm_prev) {
        GetLogger(eLogType::Info)->Log("Update measured CO2 concentration value as %g", value);
        matter_update_clus_co2measure_attr_measureval();
        update_air_quality_from_co2(value);
    }
    m_measured_value_co2ppm_prev = m_measured_value_co2ppm;
}

void CAirQualitySensor::update_air_quality_from_co2(float co2_ppm)
{
    // Matter AirQualityEnum: 0=Unknown 1=Good 2=Fair 3=Moderate 4=Poor 5=VeryPoor 6=ExtremelyPoor
    uint8_t aq;
    if (co2_ppm < 800.f)        aq = 1; // Good
    else if (co2_ppm < 1000.f)  aq = 2; // Fair
    else if (co2_ppm < 1500.f)  aq = 3; // Moderate
    else if (co2_ppm < 2000.f)  aq = 4; // Poor
    else if (co2_ppm < 3000.f)  aq = 5; // Very Poor
    else                        aq = 6; // Extremely Poor

    if (aq == m_air_quality)
        return;
    m_air_quality = aq;

    esp_matter_attr_val_t val = esp_matter_enum8(aq);
    esp_err_t ret = esp_matter::attribute::update(
        m_endpoint_id,
        chip::app::Clusters::AirQuality::Id,
        chip::app::Clusters::AirQuality::Attributes::AirQuality::Id,
        &val
    );
    if (ret == ESP_OK) {
        GetLogger(eLogType::Info)->Log("Air quality updated to %d (CO2: %g ppm)", aq, co2_ppm);
    } else {
        GetLogger(eLogType::Warning)->Log("Failed to update air quality (ret: %d)", ret);
    }
}

void CAirQualitySensor::update_measured_value_temperature(float value)
{
    m_measured_value_temperature = (int16_t)(value * 100.f);
    if (m_measured_value_temperature_prev != m_measured_value_temperature) {
        GetLogger(eLogType::Info)->Log("Update measured temperature value as %g", value);
        matter_update_clus_tempmeasure_attr_measureval();
    }
    m_measured_value_temperature_prev = m_measured_value_temperature;
}

void CAirQualitySensor::update_measured_value_humidity(float value)
{
    m_measured_value_humidity = (uint16_t)(value * 100.f);
    if (m_measured_value_humidity_prev != m_measured_value_humidity) {
        GetLogger(eLogType::Info)->Log("Update measured relative humidity value as %g", value);
        matter_update_clus_relhummeasure_attr_measureval();
    }
    m_measured_value_humidity_prev = m_measured_value_humidity;
}

void CAirQualitySensor::matter_update_clus_co2measure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_float(m_measured_value_co2ppm);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Id,
        chip::app::Clusters::CarbonDioxideConcentrationMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_co2measure_attr_measureval,
        force_update
    );
}

void CAirQualitySensor::matter_update_clus_tempmeasure_attr_measureval(bool force_update/*=false*/)
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

void CAirQualitySensor::matter_update_clus_relhummeasure_attr_measureval(bool force_update/*=false*/)
{
    esp_matter_attr_val_t target_value = esp_matter_nullable_uint16(m_measured_value_humidity);
    matter_update_cluster_attribute_common(
        m_endpoint_id,
        chip::app::Clusters::RelativeHumidityMeasurement::Id,
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
        target_value,
        &m_matter_update_by_client_clus_relhummeasure_attr_measureval,
        force_update
    );
}
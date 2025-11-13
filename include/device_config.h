#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <string>
#include <vector>

struct DeviceInfo {
    std::string version;
    std::string serial_number;
    std::string model_type;
    std::string firmware_version;
    std::string hardware_id;
    std::string manufacturer;
    std::string device_type;
    std::string hardware_revision;
    std::string production_date;
    std::string warranty_period;
    std::string support_contact;
    std::string documentation_url;
    std::string build_date;
    std::string mode;
    std::string system_uuid;
    std::string endpoint_port;  // Added for registration
};

struct DeviceStatus {
    long long uptime_seconds;
    bool detector_configured;
};

/**
 * Load device configuration from file or use defaults
 */
void load_device_config();

/**
 * Reload device configuration from file (force reload)
 */
void reload_device_config();

/**
 * Get device information
 */
DeviceInfo get_device_info();

/**
 * Get device status
 */
DeviceStatus get_device_status();

/**
 * Get device instances
 */
std::vector<std::string> get_device_instances();

/**
 * Set device instances
 */
void set_device_instances(const std::vector<std::string>& instances);

/**
 * Read system UUID from Linux system
 */
std::string read_system_uuid();

/**
 * Get build date string
 */
std::string get_build_date();

/**
 * Update device configuration from JSON string
 * Only updates fields marked for registration
 */
bool update_device_config_from_json(const std::string& json_str);

/**
 * Save device configuration to file
 */
bool save_device_config();

/**
 * Get endpoint port
 */
std::string get_endpoint_port();

#endif // DEVICE_CONFIG_H


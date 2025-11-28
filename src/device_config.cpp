#include "device_config.h"
#include "json_utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <ctime>

// Build date macros (set by compiler)
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__ " " __TIME__
#endif

static DeviceInfo g_device_info;
static bool g_config_loaded = false;
static std::vector<std::string> g_device_instances;
static std::string g_cached_system_uuid; // Cache system UUID (doesn't change)
static time_t g_last_instances_file_mtime = 0; // Track file modification time for instances

// Default device configuration
static DeviceInfo get_default_device_info() {
    DeviceInfo info;
    info.version = "1.0.0";
    info.serial_number = "X99EINTE2314";
    info.model_type = "GTR_PRO";
    info.firmware_version = "1.0.0";
    info.hardware_id = "unknown";
    info.manufacturer = "CVEDIX";
    info.device_type = "AI_VISION_SYSTEM";
    info.hardware_revision = "REV_A";
    info.production_date = "2024-01-01";
    info.warranty_period = "24";
    info.support_contact = "support@cvedix.com";
    info.documentation_url = "https://docs.cvedix.com";
    info.build_date = BUILD_DATE;
    info.mode = "local";
    info.system_uuid = "";  // Will be read from system
    info.endpoint_port = "3546";  // Default endpoint port
    return info;
}

std::string read_system_uuid() {
    // Return cached value if available (system UUID doesn't change)
    if (!g_cached_system_uuid.empty()) {
        return g_cached_system_uuid;
    }
    
    // Try to read from /etc/machine-id first (systemd) - fast
    std::ifstream machine_id_file("/etc/machine-id");
    if (machine_id_file.is_open()) {
        std::string machine_id;
        std::getline(machine_id_file, machine_id);
        machine_id_file.close();
        if (!machine_id.empty()) {
            // Format as UUID: convert 32-char hex to UUID format
            if (machine_id.length() >= 32) {
                std::string uuid = machine_id.substr(0, 8) + "-" +
                                  machine_id.substr(8, 4) + "-" +
                                  machine_id.substr(12, 4) + "-" +
                                  machine_id.substr(16, 4) + "-" +
                                  machine_id.substr(20, 12);
                g_cached_system_uuid = uuid;
                return uuid;
            }
            g_cached_system_uuid = machine_id;
            return machine_id;
        }
    }
    
    // Try /sys/class/dmi/id/product_uuid (DMI) - fast, no command execution
    std::ifstream dmi_file("/sys/class/dmi/id/product_uuid");
    if (dmi_file.is_open()) {
        std::string uuid;
        std::getline(dmi_file, uuid);
        dmi_file.close();
        if (!uuid.empty() && uuid != "00000000-0000-0000-0000-000000000000") {
            g_cached_system_uuid = uuid;
            return uuid;
        }
    }
    
    // Fallback: try dmidecode command (slow, only if needed)
    FILE* pipe = popen("dmidecode -s system-uuid 2>/dev/null", "r");
    if (pipe != nullptr) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            pclose(pipe);
            std::string uuid(buffer);
            // Remove newline
            if (!uuid.empty() && uuid.back() == '\n') {
                uuid.pop_back();
            }
            if (!uuid.empty() && uuid != "Not Specified" && uuid != "00000000-0000-0000-0000-000000000000") {
                g_cached_system_uuid = uuid;
                return uuid;
            }
        } else {
            pclose(pipe);
        }
    }
    
    // Last resort: return default and cache it
    g_cached_system_uuid = "0fca8dd9-68be-26d9-3cf3-aa4625bac670";
    return g_cached_system_uuid;
}

std::string get_build_date() {
    return BUILD_DATE;
}

// Helper function to extract JSON string value
static std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    // Check if it's a string (starts with ")
    if (json[pos] != '"') {
        // Try to extract number or boolean
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n') end++;
        std::string value = json.substr(pos, end - pos);
        // Trim
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        return value;
    }
    
    pos++; // Skip opening quote
    size_t end = pos;
    while (end < json.length() && json[end] != '"' && json[end] != '\n') {
        if (json[end] == '\\' && end + 1 < json.length()) {
            end += 2; // Skip escaped character
        } else {
            end++;
        }
    }
    
    if (end >= json.length() || json[end] != '"') return "";
    
    return json.substr(pos, end - pos);
}

// Helper function to extract JSON array
static std::vector<std::string> extract_json_array(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) {
        std::cout << "DEBUG extract_json_array: Key \"" << key << "\" not found in JSON" << std::endl;
        return result;
    }
    
    std::cout << "DEBUG extract_json_array: Found key \"" << key << "\" at position " << pos << std::endl;
    
    pos = json.find("[", pos);
    if (pos == std::string::npos) {
        std::cout << "DEBUG extract_json_array: '[' not found after key" << std::endl;
        return result;
    }
    
    pos++; // Skip [
    size_t end = json.find("]", pos);
    if (end == std::string::npos) {
        std::cout << "DEBUG extract_json_array: ']' not found" << std::endl;
        return result;
    }
    
    std::string array_content = json.substr(pos, end - pos);
    std::cout << "DEBUG extract_json_array: Array content = \"" << array_content << "\"" << std::endl;
    
    // Parse array elements
    size_t elem_start = 0;
    while (elem_start < array_content.length()) {
        // Skip whitespace and commas
        while (elem_start < array_content.length() && 
               (array_content[elem_start] == ' ' || 
                array_content[elem_start] == ',' || 
                array_content[elem_start] == '\t' ||
                array_content[elem_start] == '\n')) {
            elem_start++;
        }
        
        if (elem_start >= array_content.length()) break;
        
        if (array_content[elem_start] == '"') {
            elem_start++; // Skip opening quote
            size_t elem_end = elem_start;
            while (elem_end < array_content.length() && array_content[elem_end] != '"') {
                if (array_content[elem_end] == '\\' && elem_end + 1 < array_content.length()) {
                    elem_end += 2;
                } else {
                    elem_end++;
                }
            }
            if (elem_end < array_content.length()) {
                std::string elem = array_content.substr(elem_start, elem_end - elem_start);
                result.push_back(elem);
                std::cout << "DEBUG extract_json_array: Extracted element = \"" << elem << "\"" << std::endl;
                elem_start = elem_end + 1;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    std::cout << "DEBUG extract_json_array: Total extracted " << result.size() << " elements" << std::endl;
    return result;
}

void load_device_config() {
    if (g_config_loaded) {
        return;
    }
    
    // Initialize with defaults
    g_device_info = get_default_device_info();
    
    // FIRST: Try to load from device_registered.json (saved from POST)
    // This takes priority over defaults and environment variables
    // Try current directory first, then /etc
    std::string config_path = "./device_registered.json";
    std::ifstream saved_config(config_path);
    
    if (!saved_config.is_open()) {
        config_path = "/etc/device_registered.json";
        saved_config.open(config_path);
    }
    
    if (saved_config.is_open()) {
        std::string content((std::istreambuf_iterator<char>(saved_config)),
                           std::istreambuf_iterator<char>());
        saved_config.close();
        
        // Parse all registered fields from saved config
        std::string version = extract_json_string(content, "version");
        if (!version.empty()) g_device_info.version = version;
        
        std::string serial = extract_json_string(content, "serial_number");
        if (!serial.empty()) g_device_info.serial_number = serial;
        
        std::string model = extract_json_string(content, "model_type");
        if (!model.empty()) g_device_info.model_type = model;
        
        std::string device_type = extract_json_string(content, "device_type");
        if (!device_type.empty()) g_device_info.device_type = device_type;
        
        std::string hw_rev = extract_json_string(content, "hardware_revision");
        if (!hw_rev.empty()) g_device_info.hardware_revision = hw_rev;
        
        std::string prod_date = extract_json_string(content, "production_date");
        if (!prod_date.empty()) g_device_info.production_date = prod_date;
        
        std::string warranty = extract_json_string(content, "warranty_period");
        if (!warranty.empty()) g_device_info.warranty_period = warranty;
        
        std::string build_date = extract_json_string(content, "build_date");
        if (!build_date.empty()) g_device_info.build_date = build_date;
        
        std::string mode = extract_json_string(content, "mode");
        if (!mode.empty()) g_device_info.mode = mode;
        
        std::string port = extract_json_string(content, "endpoint_port");
        if (!port.empty()) g_device_info.endpoint_port = port;
        
        // Load instances
        std::vector<std::string> instances = extract_json_array(content, "instances");
        if (!instances.empty()) {
            set_device_instances(instances);
        }
    }
    
    // Override with environment variables if available (only if not loaded from file)
    const char* env_version = std::getenv("DEVICE_VERSION");
    if (env_version) g_device_info.version = env_version;
    
    const char* env_serial = std::getenv("DEVICE_SERIAL_NUMBER");
    if (env_serial) g_device_info.serial_number = env_serial;
    
    const char* env_model = std::getenv("DEVICE_MODEL_TYPE");
    if (env_model) g_device_info.model_type = env_model;
    
    const char* env_firmware = std::getenv("DEVICE_FIRMWARE_VERSION");
    if (env_firmware) g_device_info.firmware_version = env_firmware;
    
    const char* env_hw_id = std::getenv("DEVICE_HARDWARE_ID");
    if (env_hw_id) g_device_info.hardware_id = env_hw_id;
    
    const char* env_manufacturer = std::getenv("DEVICE_MANUFACTURER");
    if (env_manufacturer) g_device_info.manufacturer = env_manufacturer;
    
    const char* env_device_type = std::getenv("DEVICE_TYPE");
    if (env_device_type) g_device_info.device_type = env_device_type;
    
    const char* env_hw_rev = std::getenv("DEVICE_HARDWARE_REVISION");
    if (env_hw_rev) g_device_info.hardware_revision = env_hw_rev;
    
    const char* env_prod_date = std::getenv("DEVICE_PRODUCTION_DATE");
    if (env_prod_date) g_device_info.production_date = env_prod_date;
    
    const char* env_warranty = std::getenv("DEVICE_WARRANTY_PERIOD");
    if (env_warranty) g_device_info.warranty_period = env_warranty;
    
    const char* env_support = std::getenv("DEVICE_SUPPORT_CONTACT");
    if (env_support) g_device_info.support_contact = env_support;
    
    const char* env_docs = std::getenv("DEVICE_DOCUMENTATION_URL");
    if (env_docs) g_device_info.documentation_url = env_docs;
    
    const char* env_mode = std::getenv("DEVICE_MODE");
    if (env_mode) g_device_info.mode = env_mode;
    
    const char* env_port = std::getenv("DEVICE_ENDPOINT_PORT");
    if (env_port && g_device_info.endpoint_port.empty()) {
        g_device_info.endpoint_port = env_port;
    }
    
    // Read system UUID (cached, only read once)
    g_device_info.system_uuid = read_system_uuid();
    
    g_config_loaded = true;
}

void reload_device_config() {
    g_config_loaded = false;
    g_device_instances.clear();
    g_last_instances_file_mtime = 0; // Reset modification time tracking
    
    // Load config (this will also load instances from file)
    load_device_config();
    
    // Ensure instances are loaded from file
    std::string config_path = "./device_registered.json";
    std::ifstream saved_config(config_path);
    
    if (!saved_config.is_open()) {
        config_path = "/etc/device_registered.json";
        saved_config.open(config_path);
    }
    
    if (saved_config.is_open()) {
        std::string content((std::istreambuf_iterator<char>(saved_config)),
                           std::istreambuf_iterator<char>());
        saved_config.close();
        std::vector<std::string> instances = extract_json_array(content, "instances");
        if (!instances.empty()) {
            g_device_instances = instances;
        }
        
        // Update modification time tracking
        struct stat file_stat;
        if (stat(config_path.c_str(), &file_stat) == 0) {
            g_last_instances_file_mtime = file_stat.st_mtime;
        }
    }
}

DeviceInfo get_device_info() {
    if (!g_config_loaded) {
        load_device_config();
    }
    return g_device_info;
}

DeviceStatus get_device_status() {
    DeviceStatus status;
    
    // Read uptime from /proc/uptime
    std::ifstream uptime_file("/proc/uptime");
    if (uptime_file.is_open()) {
        double uptime_seconds;
        uptime_file >> uptime_seconds;
        uptime_file.close();
        status.uptime_seconds = (long long)uptime_seconds;
    } else {
        status.uptime_seconds = 0;
    }
    
    // Check detector configuration (can be extended to check actual config)
    // For now, default to false
    status.detector_configured = false;
    
    // Can check from environment variable
    const char* env_detector = std::getenv("DETECTOR_CONFIGURED");
    if (env_detector) {
        status.detector_configured = (std::string(env_detector) == "true" || std::string(env_detector) == "1");
    }
    
    return status;
}

std::vector<std::string> get_device_instances() {
    // Check if file has been modified since last load
    std::string config_path = "./device_registered.json";
    struct stat file_stat;
    bool file_exists = (stat(config_path.c_str(), &file_stat) == 0);
    
    if (!file_exists) {
        config_path = "/etc/device_registered.json";
        file_exists = (stat(config_path.c_str(), &file_stat) == 0);
    }
    
    // If file exists and has been modified, clear cache and reload
    if (file_exists) {
        time_t current_mtime = file_stat.st_mtime;
        if (current_mtime != g_last_instances_file_mtime) {
            // File has been modified, clear cache
            g_device_instances.clear();
            g_last_instances_file_mtime = current_mtime;
        }
    }
    
    // If instances are already cached, return them (fast path)
    if (!g_device_instances.empty()) {
        return g_device_instances;
    }
    
    // Try to read from saved config file
    // Try current directory first, then /etc
    std::vector<std::string> instances;
    config_path = "./device_registered.json";
    std::ifstream saved_config(config_path);
    
    if (!saved_config.is_open()) {
        config_path = "/etc/device_registered.json";
        saved_config.open(config_path);
    }
    
    if (saved_config.is_open()) {
        std::string content((std::istreambuf_iterator<char>(saved_config)),
                           std::istreambuf_iterator<char>());
        saved_config.close();
        instances = extract_json_array(content, "instances");
        
        // Update modification time tracking
        if (stat(config_path.c_str(), &file_stat) == 0) {
            g_last_instances_file_mtime = file_stat.st_mtime;
        }
    }
    
    // If loaded from file, cache and return
    if (!instances.empty()) {
        g_device_instances = instances;
        return instances;
    }
    
    // Try to read from environment variable (comma-separated)
    const char* env_instances = std::getenv("DEVICE_INSTANCES");
    if (env_instances) {
        std::istringstream iss(env_instances);
        std::string instance;
        while (std::getline(iss, instance, ',')) {
            // Trim whitespace
            instance.erase(0, instance.find_first_not_of(" \t"));
            instance.erase(instance.find_last_not_of(" \t") + 1);
            if (!instance.empty()) {
                instances.push_back(instance);
            }
        }
    }
    
    // Default instance (use system UUID if no instances configured)
    if (instances.empty()) {
        DeviceInfo info = get_device_info();
        instances.push_back(info.system_uuid);
    }
    
    g_device_instances = instances;
    return instances;
}

void set_device_instances(const std::vector<std::string>& instances) {
    g_device_instances = instances;
}

bool update_device_config_from_json(const std::string& json_str) {
    if (!g_config_loaded) {
        load_device_config();
    }
    
    // Extract device object
    size_t device_start = json_str.find("\"device\"");
    if (device_start == std::string::npos) return false;
    
    device_start = json_str.find("{", device_start);
    if (device_start == std::string::npos) return false;
    
    // Find matching closing brace
    int brace_count = 0;
    size_t device_end = device_start;
    for (size_t i = device_start; i < json_str.length(); i++) {
        if (json_str[i] == '{') brace_count++;
        if (json_str[i] == '}') {
            brace_count--;
            if (brace_count == 0) {
                device_end = i + 1;
                break;
            }
        }
    }
    
    if (device_end <= device_start) return false;
    
    std::string device_json = json_str.substr(device_start, device_end - device_start);
    
    // Update fields marked for registration (# đăng ký)
    std::string version = extract_json_string(device_json, "version");
    if (!version.empty()) g_device_info.version = version;
    
    std::string serial = extract_json_string(device_json, "serial_number");
    if (!serial.empty()) g_device_info.serial_number = serial;
    
    std::string model = extract_json_string(device_json, "model_type");
    if (!model.empty()) g_device_info.model_type = model;
    
    std::string device_type = extract_json_string(device_json, "device_type");
    if (!device_type.empty()) g_device_info.device_type = device_type;
    
    std::string hw_rev = extract_json_string(device_json, "hardware_revision");
    if (!hw_rev.empty()) g_device_info.hardware_revision = hw_rev;
    
    std::string prod_date = extract_json_string(device_json, "production_date");
    if (!prod_date.empty()) g_device_info.production_date = prod_date;
    
    std::string warranty = extract_json_string(device_json, "warranty_period");
    if (!warranty.empty()) g_device_info.warranty_period = warranty;
    
    std::string build_date = extract_json_string(device_json, "build_date");
    if (!build_date.empty()) g_device_info.build_date = build_date;
    
    std::string mode = extract_json_string(device_json, "mode");
    if (!mode.empty()) g_device_info.mode = mode;
    
    // Extract endpoint_port (at root level)
    std::string port = extract_json_string(json_str, "endpoint_port");
    if (!port.empty()) g_device_info.endpoint_port = port;
    
    // Extract instances (at root level)
    std::vector<std::string> instances = extract_json_array(json_str, "instances");
    std::cout << "DEBUG: Extracted " << instances.size() << " instances from JSON" << std::endl;
    for (size_t i = 0; i < instances.size(); ++i) {
        std::cout << "DEBUG: Instance[" << i << "] = " << instances[i] << std::endl;
    }
    if (!instances.empty()) {
        set_device_instances(instances);
        std::cout << "DEBUG: Set device instances successfully" << std::endl;
    } else {
        std::cout << "DEBUG: WARNING - No instances found in JSON or extraction failed" << std::endl;
    }
    
    return true;
}

bool save_device_config() {
    // Save registered fields to file
    // Try current directory first (for development), then /etc (for production)
    std::string config_path = "./device_registered.json";
    std::ofstream config_file(config_path);
    
    if (!config_file.is_open()) {
        config_path = "/etc/device_registered.json";
        config_file.open(config_path);
    }
    
    if (!config_file.is_open()) {
        return false;
    }
    
    config_file << "{\n";
    config_file << "  \"version\": \"" << escape_json(g_device_info.version) << "\",\n";
    config_file << "  \"serial_number\": \"" << escape_json(g_device_info.serial_number) << "\",\n";
    config_file << "  \"model_type\": \"" << escape_json(g_device_info.model_type) << "\",\n";
    config_file << "  \"device_type\": \"" << escape_json(g_device_info.device_type) << "\",\n";
    config_file << "  \"hardware_revision\": \"" << escape_json(g_device_info.hardware_revision) << "\",\n";
    config_file << "  \"production_date\": \"" << escape_json(g_device_info.production_date) << "\",\n";
    config_file << "  \"warranty_period\": \"" << escape_json(g_device_info.warranty_period) << "\",\n";
    config_file << "  \"build_date\": \"" << escape_json(g_device_info.build_date) << "\",\n";
    config_file << "  \"mode\": \"" << escape_json(g_device_info.mode) << "\",\n";
    config_file << "  \"endpoint_port\": \"" << escape_json(g_device_info.endpoint_port) << "\",\n";
    
    // Save instances - ALWAYS use g_device_instances if set, don't read from file
    // because we're saving the NEW values, not the old ones from file
    std::vector<std::string> instances = g_device_instances;
    std::cout << "DEBUG: Saving " << instances.size() << " instances to file (g_device_instances.size() = " << g_device_instances.size() << ")" << std::endl;
    for (size_t i = 0; i < instances.size(); ++i) {
        std::cout << "DEBUG: Saving instance[" << i << "] = " << instances[i] << std::endl;
    }
    
    // If instances is empty, it means no instances were set in the POST request
    // In this case, we should keep the existing instances from file
    if (instances.empty()) {
        std::cout << "DEBUG: No instances in memory, reading from file to preserve existing values" << std::endl;
        instances = get_device_instances();
    }
    config_file << "  \"instances\": [\n";
    for (size_t i = 0; i < instances.size(); ++i) {
        config_file << "    \"" << escape_json(instances[i]) << "\"";
        if (i < instances.size() - 1) config_file << ",";
        config_file << "\n";
    }
    config_file << "  ]\n";
    
    config_file << "}\n";
    config_file.flush(); // Ensure data is written immediately
    
    // Verify file was written successfully BEFORE closing
    if (!config_file.good()) {
        std::cerr << "Error: Failed to write device_registered.json to " << config_path << std::endl;
        config_file.close();
        return false;
    }
    
    config_file.close();
    
    // Update modification time tracking after successful save
    struct stat file_stat;
    if (stat(config_path.c_str(), &file_stat) == 0) {
        g_last_instances_file_mtime = file_stat.st_mtime;
        std::cout << "Successfully saved device configuration to " << config_path << std::endl;
        std::cout << "File modification time: " << file_stat.st_mtime << std::endl;
    } else {
        std::cerr << "Warning: Could not stat saved file " << config_path << std::endl;
    }
    
    return true;
}

std::string get_endpoint_port() {
    if (!g_config_loaded) {
        load_device_config();
    }
    // Always return the current value from g_device_info (which should be loaded from file)
    return g_device_info.endpoint_port.empty() ? "3546" : g_device_info.endpoint_port;
}


#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

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

// Helper function to extract JSON integer value
static int extract_json_int(const std::string& json, const std::string& key, int default_value = 0) {
    std::string value = extract_json_string(json, key);
    if (value.empty()) return default_value;
    
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

// Helper function to extract nested JSON object
static std::string extract_json_object(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find("{", pos);
    if (pos == std::string::npos) return "";
    
    // Find matching closing brace
    int brace_count = 0;
    size_t start = pos;
    for (size_t i = pos; i < json.length(); i++) {
        if (json[i] == '{') brace_count++;
        if (json[i] == '}') {
            brace_count--;
            if (brace_count == 0) {
                return json.substr(start, i - start + 1);
            }
        }
    }
    
    return "";
}

AppConfig get_default_config() {
    AppConfig config;
    
    // Server defaults
    config.server.port = 8080;
    config.server.host = "0.0.0.0";
    
    // Authentication defaults
    config.authentication.username = "cvedix";
    config.authentication.password = "cvedix";
    
    // Device config paths
    config.device.config_file = "/etc/device_registered.json";
    config.device.fallback_config_file = "./device_registered.json";
    
    // Logging defaults
    config.logging.level = "info";
    
    return config;
}

AppConfig load_config(const std::string& config_path) {
    AppConfig config = get_default_config();
    
    // Try to read config file
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        // Try alternative paths
        config_file.open("./config.json");
        if (!config_file.is_open()) {
            config_file.open("/etc/metrics_monitor_system/config.json");
        }
    }
    
    if (!config_file.is_open()) {
        std::cerr << "Warning: config.json not found, using default configuration" << std::endl;
        return config;
    }
    
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(config_file)),
                       std::istreambuf_iterator<char>());
    config_file.close();
    
    // Parse server config
    std::string server_json = extract_json_object(content, "server");
    if (!server_json.empty()) {
        int port = extract_json_int(server_json, "port", config.server.port);
        std::string host = extract_json_string(server_json, "host");
        
        if (port > 0 && port < 65536) {
            config.server.port = port;
        }
        if (!host.empty()) {
            config.server.host = host;
        }
    }
    
    // Parse authentication config
    std::string auth_json = extract_json_object(content, "authentication");
    if (!auth_json.empty()) {
        std::string username = extract_json_string(auth_json, "username");
        std::string password = extract_json_string(auth_json, "password");
        
        if (!username.empty()) {
            config.authentication.username = username;
        }
        if (!password.empty()) {
            config.authentication.password = password;
        }
    }
    
    // Parse device config paths
    std::string device_json = extract_json_object(content, "device");
    if (!device_json.empty()) {
        std::string config_file = extract_json_string(device_json, "config_file");
        std::string fallback_file = extract_json_string(device_json, "fallback_config_file");
        
        if (!config_file.empty()) {
            config.device.config_file = config_file;
        }
        if (!fallback_file.empty()) {
            config.device.fallback_config_file = fallback_file;
        }
    }
    
    // Parse logging config
    std::string logging_json = extract_json_object(content, "logging");
    if (!logging_json.empty()) {
        std::string level = extract_json_string(logging_json, "level");
        if (!level.empty()) {
            config.logging.level = level;
        }
    }
    
    return config;
}


#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct ServerConfig {
    int port;
    std::string host;
};

struct AuthConfig {
    std::string username;
    std::string password;
};

struct DeviceConfigPaths {
    std::string config_file;
    std::string fallback_config_file;
};

struct LoggingConfig {
    std::string level;
};

struct AppConfig {
    ServerConfig server;
    AuthConfig authentication;
    DeviceConfigPaths device;
    LoggingConfig logging;
};

/**
 * Load configuration from config.json file
 * @param config_path Path to config.json file (default: "./config.json")
 * @return AppConfig structure with loaded configuration
 */
AppConfig load_config(const std::string& config_path = "./config.json");

/**
 * Get default configuration
 */
AppConfig get_default_config();

#endif // CONFIG_H


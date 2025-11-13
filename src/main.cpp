#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include "httplib.h"
#include "system_info.h"
#include "system_status.h"
#include "device_config.h"
#include "config.h"

using namespace httplib;

// Helper function to enable CORS
void enable_cors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// GET /v1/core/system/info - Returns detailed system hardware information
void handle_system_info(const Request& req, Response& res) {
    enable_cors(res);
    res.set_header("Content-Type", "application/json");
    
    try {
        std::string json_info = get_system_info_json();
        res.set_content(json_info, "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"error": "Failed to get system info", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

// GET /v1/core/system/status - Returns system status (CPU, RAM, etc.)
void handle_system_status(const Request& req, Response& res) {
    enable_cors(res);
    res.set_header("Content-Type", "application/json");
    
    try {
        std::string json_status = get_system_status_json();
        res.set_content(json_status, "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"error": "Failed to get system status", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

// POST /v1/core/system/reboot - Reboots the system
void handle_system_reboot(const Request& req, Response& res) {
    enable_cors(res);
    res.set_header("Content-Type", "application/json");
    
    // Check if user has permission (in production, add authentication)
    // For now, we'll just return a message. Uncomment the actual reboot code if needed.
    
    try {
        // Return success message
        res.set_content(R"({"status": "success", "message": "System reboot initiated"})", "application/json");
        
        // WARNING: Uncommenting the following will actually reboot the system!
        // This requires root privileges and should be protected by authentication
        /*
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            system("sudo reboot");
        }).detach();
        */
        
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"error": "Failed to reboot system", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

// Handle OPTIONS for CORS preflight
void handle_options(const Request& req, Response& res) {
    enable_cors(res);
    res.status = 200;
}

// Global config (loaded at startup)
static AppConfig g_app_config;

// Check Basic Authentication (with explicit credentials)
static bool check_basic_auth_impl(const Request& req, const std::string& username, const std::string& password) {
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.empty()) {
        return false;
    }
    
    // Check if it's Basic auth
    if (auth_header.find("Basic ") != 0) {
        return false;
    }
    
    // Decode base64 (simple implementation)
    std::string encoded = auth_header.substr(6); // Skip "Basic "
    
    // Remove whitespace
    encoded.erase(std::remove(encoded.begin(), encoded.end(), ' '), encoded.end());
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '\n'), encoded.end());
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '\r'), encoded.end());
    
    // For simplicity, we'll use a basic base64 decode
    // In production, use a proper base64 library
    std::string decoded;
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int val = 0, valb = -8;
    for (size_t i = 0; i < encoded.length(); i++) {
        char c = encoded[i];
        if (c == '=') break;
        size_t idx = chars.find(c);
        if (idx == std::string::npos) continue;
        val = (val << 6) + idx;
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    // Check format: username:password
    size_t colon_pos = decoded.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string req_username = decoded.substr(0, colon_pos);
    std::string req_password = decoded.substr(colon_pos + 1);
    
    return (req_username == username && req_password == password);
}

// POST /v1/core/system/info - Register/Update device information
void handle_post_system_info(const Request& req, Response& res) {
    enable_cors(res);
    res.set_header("Content-Type", "application/json");
    
    // Check Basic Authentication
    if (!check_basic_auth_impl(req, g_app_config.authentication.username, g_app_config.authentication.password)) {
        res.status = 401;
        res.set_header("WWW-Authenticate", "Basic realm=\"Device Registration\"");
        res.set_content(R"({"error": "Unauthorized", "message": "Invalid credentials"})", "application/json");
        return;
    }
    
    try {
        // Get JSON body
        std::string json_body = req.body;
        
        if (json_body.empty()) {
            res.status = 400;
            res.set_content(R"({"error": "Bad Request", "message": "JSON body is required"})", "application/json");
            return;
        }
        
        // Update device configuration from JSON
        if (!update_device_config_from_json(json_body)) {
            res.status = 400;
            res.set_content(R"({"error": "Bad Request", "message": "Failed to parse JSON or invalid format"})", "application/json");
            return;
        }
        
        // Save configuration to file (device_registered.json)
        if (!save_device_config()) {
            res.status = 500;
            res.set_content(R"({"error": "Internal Server Error", "message": "Failed to save configuration"})", "application/json");
            return;
        }
        
        // Reload config from file to ensure memory is updated
        // This ensures subsequent GET requests will return the saved values
        reload_device_config();
        
        // Return success response
        res.status = 200;
        res.set_content(R"({"status": "success", "message": "Device information registered successfully"})", "application/json");
        
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"error": "Internal Server Error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

int main(int argc, char** argv) {
    // Load configuration
    std::string config_path = "./config.json";
    bool config_loaded = false;
    
    if (argc > 1) {
        // First argument can be config file path or port (for backward compatibility)
        std::string arg1 = argv[1];
        if (arg1.find(".json") != std::string::npos || arg1.find("/") != std::string::npos) {
            config_path = arg1;
        } else {
            // Legacy: treat as port number
            std::cerr << "Warning: Port as first argument is deprecated. Use config.json instead." << std::endl;
            g_app_config = get_default_config();
            g_app_config.server.port = std::stoi(arg1);
            config_loaded = true;
        }
    }
    
    if (!config_loaded) {
        g_app_config = load_config(config_path);
    }
    
    // Override with command line arguments if provided (for backward compatibility)
    if (argc > 2) {
        g_app_config.server.port = std::stoi(argv[2]);
    }
    
    std::cout << "Loading configuration from: " << config_path << std::endl;
    std::cout << "Server will listen on: " << g_app_config.server.host << ":" << g_app_config.server.port << std::endl;
    
    Server svr;
    
    // API endpoints
    svr.Get("/v1/core/system/info", handle_system_info);
    svr.Post("/v1/core/system/info", handle_post_system_info);
    svr.Get("/v1/core/system/status", handle_system_status);
    svr.Post("/v1/core/system/reboot", handle_system_reboot);
    svr.Options("/v1/core/system/.*", handle_options);
    
    // Health check endpoint
    svr.Get("/health", [](const Request& req, Response& res) {
        res.set_content(R"({"status": "ok"})", "application/json");
    });
    
    // Root endpoint
    svr.Get("/", [](const Request& req, Response& res) {
        std::ostringstream json;
        json << "{\"service\": \"Metrics Monitor System\", \"version\": \"1.0.0\", \"endpoints\": {";
        json << "\"system_info\": \"GET /v1/core/system/info\", ";
        json << "\"system_info_register\": \"POST /v1/core/system/info (Basic Auth required)\", ";
        json << "\"system_status\": \"GET /v1/core/system/status\", ";
        json << "\"system_reboot\": \"POST /v1/core/system/reboot\"}}";
        res.set_content(json.str(), "application/json");
    });
    
    std::cout << "Server starting..." << std::endl;
    std::cout << "API endpoints:" << std::endl;
    std::cout << "  GET  /v1/core/system/info" << std::endl;
    std::cout << "  POST /v1/core/system/info (Basic Auth: " 
              << g_app_config.authentication.username << "/" 
              << g_app_config.authentication.password << ")" << std::endl;
    std::cout << "  GET  /v1/core/system/status" << std::endl;
    std::cout << "  POST /v1/core/system/reboot" << std::endl;
    std::cout << "  GET  /health" << std::endl;
    
    if (!svr.listen(g_app_config.server.host.c_str(), g_app_config.server.port)) {
        std::cerr << "Failed to start server on " << g_app_config.server.host 
                  << ":" << g_app_config.server.port << std::endl;
        return 1;
    }
    
    return 0;
}


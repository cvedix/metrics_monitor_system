#include "system_info.h"
#include "device_config.h"
#include "json_utils.h"
#include <hwinfo/hwinfo.h>
#include <hwinfo/cpu.h>
#include <hwinfo/gpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/os.h>
#include <hwinfo/ram.h>
#include <sstream>
#include <iomanip>
#include <vector>

std::string get_system_info_json() {
    // Load device config if not already loaded
    // Only reload if needed (after POST, config is already reloaded)
    load_device_config();
    
    std::ostringstream json;
    json << "{\n";
    
    // Device Information
    DeviceInfo device = get_device_info();
    json << "  \"device\": {\n";
    json << "    \"version\": \"" << escape_json(device.version) << "\",\n";
    json << "    \"serial_number\": \"" << escape_json(device.serial_number) << "\",\n";
    json << "    \"model_type\": \"" << escape_json(device.model_type) << "\",\n";
    json << "    \"firmware_version\": \"" << escape_json(device.firmware_version) << "\",\n";
    json << "    \"hardware_id\": \"" << escape_json(device.hardware_id) << "\",\n";
    json << "    \"manufacturer\": \"" << escape_json(device.manufacturer) << "\",\n";
    json << "    \"device_type\": \"" << escape_json(device.device_type) << "\",\n";
    json << "    \"hardware_revision\": \"" << escape_json(device.hardware_revision) << "\",\n";
    json << "    \"production_date\": \"" << escape_json(device.production_date) << "\",\n";
    json << "    \"warranty_period\": \"" << escape_json(device.warranty_period) << "\",\n";
    json << "    \"support_contact\": \"" << escape_json(device.support_contact) << "\",\n";
    json << "    \"documentation_url\": \"" << escape_json(device.documentation_url) << "\",\n";
    json << "    \"build_date\": \"" << escape_json(device.build_date) << "\",\n";
    json << "    \"mode\": \"" << escape_json(device.mode) << "\",\n";
    json << "    \"system_uuid\": \"" << escape_json(device.system_uuid) << "\"\n";
    json << "  },\n";
    
    // Status Information
    DeviceStatus status = get_device_status();
    json << "  \"status\": {\n";
    json << "    \"uptime_seconds\": " << status.uptime_seconds << ",\n";
    json << "    \"detector_configured\": " << (status.detector_configured ? "true" : "false") << "\n";
    json << "  },\n";
    
    // Endpoint Port
    json << "  \"endpoint_port\": \"" << escape_json(get_endpoint_port()) << "\",\n";
    
    // Instances
    std::vector<std::string> instances = get_device_instances();
    json << "  \"instances\": [\n";
    for (size_t i = 0; i < instances.size(); ++i) {
        json << "    \"" << escape_json(instances[i]) << "\"";
        if (i < instances.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // CPU Information
    json << "  \"cpu\": [\n";
    auto cpus = hwinfo::getAllCPUs();
    for (size_t i = 0; i < cpus.size(); ++i) {
        const auto& cpu = cpus[i];
        json << "    {\n";
        json << "      \"socket\": " << i << ",\n";
        json << "      \"vendor\": \"" << escape_json(cpu.vendor()) << "\",\n";
        json << "      \"model\": \"" << escape_json(cpu.modelName()) << "\",\n";
        json << "      \"physical_cores\": " << cpu.numPhysicalCores() << ",\n";
        json << "      \"logical_cores\": " << cpu.numLogicalCores() << ",\n";
        json << "      \"max_frequency_mhz\": " << cpu.maxClockSpeed_MHz() << ",\n";
        json << "      \"regular_frequency_mhz\": " << cpu.regularClockSpeed_MHz() << ",\n";
        auto current_freqs = cpu.currentClockSpeed_MHz();
        int64_t current_freq = current_freqs.empty() ? 0 : current_freqs[0];
        json << "      \"current_frequency_mhz\": " << current_freq << ",\n";
        int64_t cache_size = cpu.L1CacheSize_Bytes() + cpu.L2CacheSize_Bytes() + cpu.L3CacheSize_Bytes();
        json << "      \"cache_size_bytes\": " << cache_size << "\n";
        json << "    }";
        if (i < cpus.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // RAM Information
    json << "  \"ram\": {\n";
    hwinfo::Memory ram;
    auto modules = ram.modules();
    if (!modules.empty()) {
        const auto& m = modules[0];
        json << "    \"vendor\": \"" << escape_json(m.vendor) << "\",\n";
        json << "    \"model\": \"" << escape_json(m.model) << "\",\n";
        json << "    \"name\": \"" << escape_json(m.name) << "\",\n";
        json << "    \"serial_number\": \"" << escape_json(m.serial_number) << "\",\n";
        json << "    \"total_size_mib\": " << ram.total_Bytes() / (1024 * 1024) << ",\n";
        json << "    \"free_size_mib\": " << ram.free_Bytes() / (1024 * 1024) << ",\n";
        json << "    \"available_size_mib\": " << ram.available_Bytes() / (1024 * 1024) << "\n";
    } else {
        json << "    \"total_size_mib\": " << ram.total_Bytes() / (1024 * 1024) << ",\n";
        json << "    \"free_size_mib\": " << ram.free_Bytes() / (1024 * 1024) << ",\n";
        json << "    \"available_size_mib\": " << ram.available_Bytes() / (1024 * 1024) << "\n";
    }
    json << "  },\n";
    
    // GPU Information
    json << "  \"gpu\": [\n";
    auto gpus = hwinfo::getAllGPUs();
    for (size_t i = 0; i < gpus.size(); ++i) {
        const auto& gpu = gpus[i];
        json << "    {\n";
        json << "      \"id\": " << i << ",\n";
        json << "      \"vendor\": \"" << escape_json(gpu.vendor()) << "\",\n";
        json << "      \"model\": \"" << escape_json(gpu.name()) << "\",\n";
        json << "      \"driver_version\": \"" << escape_json(gpu.driverVersion()) << "\",\n";
        json << "      \"memory_mib\": " << gpu.memory_Bytes() / (1024 * 1024) << ",\n";
        json << "      \"frequency_mhz\": " << gpu.frequency_MHz() << "\n";
        json << "    }";
        if (i < gpus.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // Mainboard Information
    json << "  \"mainboard\": {\n";
    hwinfo::MainBoard mainboard;
    json << "    \"vendor\": \"" << escape_json(mainboard.vendor()) << "\",\n";
    json << "    \"name\": \"" << escape_json(mainboard.name()) << "\",\n";
    json << "    \"version\": \"" << escape_json(mainboard.version()) << "\",\n";
    json << "    \"serial_number\": \"" << escape_json(mainboard.serialNumber()) << "\"\n";
    json << "  },\n";
    
    // Disk Information
    json << "  \"disks\": [\n";
    auto disks = hwinfo::getAllDisks();
    for (size_t i = 0; i < disks.size(); ++i) {
        const auto& disk = disks[i];
        json << "    {\n";
        json << "      \"id\": " << i << ",\n";
        json << "      \"vendor\": \"" << escape_json(disk.vendor()) << "\",\n";
        json << "      \"model\": \"" << escape_json(disk.model()) << "\",\n";
        json << "      \"serial_number\": \"" << escape_json(disk.serialNumber()) << "\",\n";
        json << "      \"size_bytes\": " << disk.size_Bytes() << ",\n";
        json << "      \"free_size_bytes\": " << disk.free_size_Bytes() << ",\n";
        json << "      \"volumes\": [\n";
        auto volumes = disk.volumes();
        for (size_t j = 0; j < volumes.size(); ++j) {
            json << "        \"" << escape_json(volumes[j]) << "\"";
            if (j < volumes.size() - 1) json << ",";
            json << "\n";
        }
        json << "      ]\n";
        json << "    }";
        if (i < disks.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // OS Information
    json << "  \"os\": {\n";
    hwinfo::OS os;
    json << "    \"name\": \"" << escape_json(os.name()) << "\",\n";
    json << "    \"version\": \"" << escape_json(os.version()) << "\",\n";
    json << "    \"kernel\": \"" << escape_json(os.kernel()) << "\",\n";
    json << "    \"architecture_bits\": " << (os.is64bit() ? 64 : (os.is32bit() ? 32 : 0)) << ",\n";
    json << "    \"endianess\": \"" << (os.isLittleEndian() ? "little" : "big") << "\"\n";
    json << "  }\n";
    
    json << "}";
    return json.str();
}


#include "system_status.h"
#include "json_utils.h"
#include <hwinfo/hwinfo.h>
#include <hwinfo/cpu.h>
#include <hwinfo/gpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/ram.h>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

// Read CPU usage from /proc/stat (Linux)
double get_cpu_usage() {
    static long long last_idle = 0, last_total = 0;
    
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        return -1.0;
    }
    
    std::string line;
    std::getline(stat_file, line);
    stat_file.close();
    
    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    
    long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long long total_idle = idle + iowait;
    
    if (last_total == 0) {
        last_idle = total_idle;
        last_total = total;
        return -1.0; // First call, need second measurement
    }
    
    long long total_diff = total - last_total;
    long long idle_diff = total_idle - last_idle;
    
    last_idle = total_idle;
    last_total = total;
    
    if (total_diff == 0) return 0.0;
    
    return 100.0 * (1.0 - (double)idle_diff / total_diff);
}

std::string get_system_status_json() {
    std::ostringstream json;
    json << "{\n";
    
    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    json << "  \"timestamp\": \"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\",\n";
    
    // CPU Status
    json << "  \"cpu\": {\n";
    auto cpus = hwinfo::getAllCPUs();
    if (!cpus.empty()) {
        const auto& cpu = cpus[0];
        double cpu_usage = get_cpu_usage();
        auto current_freqs = cpu.currentClockSpeed_MHz();
        int64_t current_freq = current_freqs.empty() ? 0 : current_freqs[0];
        json << "    \"current_frequency_mhz\": " << current_freq << ",\n";
        json << "    \"max_frequency_mhz\": " << cpu.maxClockSpeed_MHz() << ",\n";
        json << "    \"usage_percent\": " << (cpu_usage >= 0 ? cpu_usage : -1) << ",\n";
        json << "    \"physical_cores\": " << cpu.numPhysicalCores() << ",\n";
        json << "    \"logical_cores\": " << cpu.numLogicalCores() << "\n";
    } else {
        json << "    \"error\": \"No CPU information available\"\n";
    }
    json << "  },\n";
    
    // RAM Status
    json << "  \"ram\": {\n";
    hwinfo::Memory ram;
    long long total_mib = ram.total_Bytes() / (1024 * 1024);
    long long free_mib = ram.free_Bytes() / (1024 * 1024);
    long long available_mib = ram.available_Bytes() / (1024 * 1024);
    long long used_mib = total_mib - available_mib;
    double usage_percent = total_mib > 0 ? (100.0 * used_mib / total_mib) : 0.0;
    
    json << "    \"total_mib\": " << total_mib << ",\n";
    json << "    \"used_mib\": " << used_mib << ",\n";
    json << "    \"free_mib\": " << free_mib << ",\n";
    json << "    \"available_mib\": " << available_mib << ",\n";
    json << "    \"usage_percent\": " << std::fixed << std::setprecision(2) << usage_percent << "\n";
    json << "  },\n";
    
    // Disk Status
    json << "  \"disks\": [\n";
    auto disks = hwinfo::getAllDisks();
    for (size_t i = 0; i < disks.size(); ++i) {
        const auto& disk = disks[i];
        long long total_bytes = disk.size_Bytes();
        long long free_bytes = disk.free_size_Bytes();
        long long used_bytes = total_bytes - free_bytes;
        double usage_percent = total_bytes > 0 ? (100.0 * used_bytes / total_bytes) : 0.0;
        
        json << "    {\n";
        json << "      \"id\": " << i << ",\n";
        json << "      \"model\": \"" << escape_json(disk.model()) << "\",\n";
        json << "      \"total_bytes\": " << total_bytes << ",\n";
        json << "      \"used_bytes\": " << used_bytes << ",\n";
        json << "      \"free_bytes\": " << free_bytes << ",\n";
        json << "      \"usage_percent\": " << std::fixed << std::setprecision(2) << usage_percent << "\n";
        json << "    }";
        if (i < disks.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // GPU Status
    json << "  \"gpu\": [\n";
    auto gpus = hwinfo::getAllGPUs();
    for (size_t i = 0; i < gpus.size(); ++i) {
        const auto& gpu = gpus[i];
        json << "    {\n";
        json << "      \"id\": " << i << ",\n";
        json << "      \"model\": \"" << escape_json(gpu.name()) << "\",\n";
        json << "      \"memory_mib\": " << gpu.memory_Bytes() / (1024 * 1024) << ",\n";
        json << "      \"frequency_mhz\": " << gpu.frequency_MHz() << "\n";
        json << "    }";
        if (i < gpus.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    
    // System Uptime (Linux)
    json << "  \"uptime\": {\n";
    std::ifstream uptime_file("/proc/uptime");
    if (uptime_file.is_open()) {
        double uptime_seconds;
        uptime_file >> uptime_seconds;
        uptime_file.close();
        
        int days = (int)(uptime_seconds / 86400);
        int hours = (int)((uptime_seconds - days * 86400) / 3600);
        int minutes = (int)((uptime_seconds - days * 86400 - hours * 3600) / 60);
        
        json << "    \"seconds\": " << (long long)uptime_seconds << ",\n";
        json << "    \"days\": " << days << ",\n";
        json << "    \"hours\": " << hours << ",\n";
        json << "    \"minutes\": " << minutes << "\n";
    } else {
        json << "    \"error\": \"Unable to read uptime\"\n";
    }
    json << "  }\n";
    
    json << "}";
    return json.str();
}


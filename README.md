# Metrics Monitor System

Hệ thống giám sát phần cứng và trạng thái hệ thống sử dụng thư viện [hwinfo](https://github.com/lfreist/hwinfo) - một thư viện C++ cross-platform để lấy thông tin phần cứng.

## Tính năng

- **GET /v1/core/system/info**: Lấy thông tin chi tiết về phần cứng hệ thống (Device info, Status, Instances, CPU, RAM, GPU, Disk, Mainboard, OS)
- **POST /v1/core/system/info**: Đăng ký/cập nhật thông tin device (yêu cầu Basic Auth: cvedix/cvedix)
- **GET /v1/core/system/status**: Lấy trạng thái hiện tại của hệ thống (CPU usage, RAM usage, Disk usage, Uptime)
- **POST /v1/core/system/reboot**: Khởi động lại hệ thống (cần quyền root và xác thực)

## Yêu cầu

- CMake >= 3.15
- C++ compiler hỗ trợ C++17 (g++, clang++, hoặc MSVC)
- Git
- wget (để tải dependencies)

## Cài đặt và Build

### 1. Setup Dependencies

Chạy script setup để tải các dependencies:

```bash
chmod +x setup_dependencies.sh
./setup_dependencies.sh
```

Script này sẽ:
- Clone repository hwinfo vào `third_party/hwinfo`
- Tải header file cpp-httplib vào `third_party/cpp-httplib`

### 2. Build Project

```bash
mkdir build
cd build
cmake ..
cmake --build . -j$(nproc)
```

### 3. Cấu hình ứng dụng

Tạo file `config.json` từ template:

```bash
cp config.json.example config.json
```

Chỉnh sửa `config.json` theo nhu cầu:

```json
{
  "server": {
    "port": 8080,
    "host": "0.0.0.0"
  },
  "authentication": {
    "username": "cvedix",
    "password": "cvedix"
  },
  "device": {
    "config_file": "/etc/device_registered.json",
    "fallback_config_file": "./device_registered.json"
  },
  "logging": {
    "level": "info"
  }
}
```

**Cấu hình Server:**
- `port`: Port để server lắng nghe (mặc định: 8080)
- `host`: 
  - `"0.0.0.0"` - Cho phép truy cập từ mọi địa chỉ IP (public)
  - `"127.0.0.1"` hoặc `"localhost"` - Chỉ cho phép truy cập local

**Cấu hình Authentication:**
- `username`: Username cho Basic Auth
- `password`: Password cho Basic Auth

### 4. Chạy ứng dụng

```bash
# Chạy với config.json mặc định (./config.json)
./metrics_monitor_system

# Chỉ định đường dẫn config.json khác
./metrics_monitor_system /path/to/config.json

# Legacy: Chỉ định port trực tiếp (deprecated)
./metrics_monitor_system 9000
```

## API Endpoints

### GET /v1/core/system/info

Trả về thông tin chi tiết về phần cứng hệ thống.

**Response Example:**
```json
{
  "device": {
    "version": "1.0.0",
    "serial_number": "X99EINTE2314",
    "model_type": "GTR_PRO",
    "firmware_version": "1.0.0",
    "hardware_id": "unknown",
    "manufacturer": "CVEDIX",
    "device_type": "AI_VISION_SYSTEM",
    "hardware_revision": "REV_A",
    "production_date": "2024-01-01",
    "warranty_period": "24",
    "support_contact": "support@cvedix.com",
    "documentation_url": "https://docs.cvedix.com",
    "build_date": "Oct 16 2025 09:08:23",
    "mode": "local",
    "system_uuid": "0fca8dd9-68be-26d9-3cf3-aa4625bac670"
  },
  "status": {
    "uptime_seconds": 1760606660,
    "detector_configured": false
  },
  "instances": ["0fca8dd9-68be-26d9-3cf3-aa4625bac670"],
  "cpu": [
    {
      "socket": 0,
      "vendor": "GenuineIntel",
      "model": "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz",
      "physical_cores": 8,
      "logical_cores": 16,
      "max_frequency_mhz": 3792,
      "regular_frequency_mhz": 3792,
      "min_frequency_mhz": 800,
      "current_frequency_mhz": 3792,
      "cache_size_bytes": 16777216
    }
  ],
  "ram": {
    "vendor": "Corsair",
    "model": "CMK32GX4M2Z3600C18",
    "name": "Physical Memory",
    "serial_number": "***",
    "total_size_mib": 65437,
    "free_size_mib": 54405,
    "available_size_mib": 54405
  },
  "gpu": [...],
  "mainboard": {...},
  "disks": [...],
  "os": {...}
}
```

### POST /v1/core/system/info

Đăng ký hoặc cập nhật thông tin device. Yêu cầu Basic Authentication.

**Authentication:**
- Username: `cvedix`
- Password: `cvedix`

**Request Body:**
```json
{
  "device": {
    "version": "1.0.0",
    "serial_number": "X99EINTE2314",
    "model_type": "GTR_PRO",
    "firmware_version": "1.0.0",
    "hardware_id": "unknown",
    "manufacturer": "CVEDIX",
    "device_type": "AI_VISION_SYSTEM",
    "hardware_revision": "REV_A",
    "production_date": "2024-01-01",
    "warranty_period": "24",
    "support_contact": "support@cvedix.com",
    "documentation_url": "https://docs.cvedix.com",
    "build_date": "Oct 16 2025 09:08:23",
    "mode": "local",
    "system_uuid": "0fca8dd9-68be-26d9-3cf3-aa4625bac670"
  },
  "status": {
    "uptime_seconds": 1760606660,
    "detector_configured": false
  },
  "endpoint_port": "3546",
  "instances": ["0fca8dd9-68be-26d9-3cf3-aa4625bachagd"]
}
```

**Lưu ý:** Chỉ các trường được đánh dấu "# đăng ký" trong comment sẽ được lưu và cập nhật:
- `device.version`
- `device.serial_number`
- `device.model_type`
- `device.device_type`
- `device.hardware_revision`
- `device.production_date`
- `device.warranty_period`
- `device.build_date`
- `device.mode`
- `endpoint_port`
- `instances`

**Response (Success):**
```json
{
  "status": "success",
  "message": "Device information registered successfully"
}
```

**Response (Error - Unauthorized):**
```json
{
  "error": "Unauthorized",
  "message": "Invalid credentials"
}
```

**Example using curl:**
```bash
curl -X POST http://localhost:8080/v1/core/system/info \
  -u cvedix:cvedix \
  -H "Content-Type: application/json" \
  -d '{
    "device": {
      "version": "1.0.0",
      "serial_number": "X99EINTE2314",
      "model_type": "GTR_PRO",
      "device_type": "AI_VISION_SYSTEM",
      "hardware_revision": "REV_A",
      "production_date": "2024-01-01",
      "warranty_period": "24",
      "build_date": "Oct 16 2025 09:08:23",
      "mode": "local"
    },
    "endpoint_port": "3546",
    "instances": ["instance1", "instance2"]
  }'
```

### GET /v1/core/system/status

Trả về trạng thái hiện tại của hệ thống.

**Response Example:**
```json
{
  "timestamp": "2025-01-27 10:30:45",
  "cpu": {
    "current_frequency_mhz": 3792,
    "max_frequency_mhz": 3792,
    "usage_percent": 25.5,
    "physical_cores": 8,
    "logical_cores": 16
  },
  "ram": {
    "total_mib": 65437,
    "used_mib": 11032,
    "free_mib": 54405,
    "available_mib": 54405,
    "usage_percent": 16.85
  },
  "disks": [...],
  "gpu": [...],
  "uptime": {
    "seconds": 86400,
    "days": 1,
    "hours": 0,
    "minutes": 0
  }
}
```

### POST /v1/core/system/reboot

Khởi động lại hệ thống.

**⚠️ CẢNH BÁO**: Endpoint này có thể khởi động lại hệ thống. Hiện tại code đã được comment để an toàn. Để kích hoạt, cần:
1. Uncomment code trong `src/main.cpp`
2. Đảm bảo ứng dụng chạy với quyền root hoặc có quyền sudo
3. Thêm xác thực để bảo vệ endpoint

**Response:**
```json
{
  "status": "success",
  "message": "System reboot initiated"
}
```

## Cấu trúc Project

```
metrics_monitor_system/
├── CMakeLists.txt          # CMake build configuration
├── setup_dependencies.sh   # Script setup dependencies
├── test_api.sh            # Script test API endpoints
├── config.json.example     # Example configuration file
├── config.json            # Configuration file (tạo từ example)
├── device_config.example.json  # Example device configuration
├── README.md              # Tài liệu này
├── include/               # Header files
│   ├── system_info.h
│   ├── system_status.h
│   ├── device_config.h
│   └── config.h
├── src/                   # Source files
│   ├── main.cpp           # Main application và HTTP server
│   ├── system_info.cpp    # Implementation cho system info
│   ├── system_status.cpp  # Implementation cho system status
│   ├── device_config.cpp  # Device configuration management
│   └── config.cpp         # Application configuration loader
└── third_party/           # Dependencies
    ├── hwinfo/            # hwinfo library
    └── cpp-httplib/       # cpp-httplib header
```

## Testing

Sau khi build và chạy ứng dụng, bạn có thể test các endpoints:

```bash
# Test system info (GET)
curl http://localhost:8080/v1/core/system/info

# Register device info (POST with Basic Auth)
curl -X POST http://localhost:8080/v1/core/system/info \
  -u cvedix:cvedix \
  -H "Content-Type: application/json" \
  -d '{"device":{"version":"1.0.0","serial_number":"X99EINTE2314"},"endpoint_port":"3546","instances":["instance1"]}'

# Test system status
curl http://localhost:8080/v1/core/system/status

# Test reboot (POST)
curl -X POST http://localhost:8080/v1/core/system/reboot

# Health check
curl http://localhost:8080/health
```

## Cấu hình

### Cấu hình ứng dụng (config.json)

File `config.json` chứa các cấu hình chính của ứng dụng:

- **Server**: Port và host để lắng nghe
  - `host: "0.0.0.0"` - Public access (cho phép truy cập từ mạng)
  - `host: "127.0.0.1"` - Local only (chỉ truy cập từ máy local)
  
- **Authentication**: Username và password cho Basic Auth

- **Device**: Đường dẫn đến file device config

- **Logging**: Mức độ logging

### Cấu hình Device

Thông tin device có thể được cấu hình thông qua:

### 1. Environment Variables

Bạn có thể set các biến môi trường để override giá trị mặc định:

```bash
export DEVICE_VERSION="1.0.0"
export DEVICE_SERIAL_NUMBER="X99EINTE2314"
export DEVICE_MODEL_TYPE="GTR_PRO"
export DEVICE_MANUFACTURER="CVEDIX"
export DEVICE_MODE="local"
export DETECTOR_CONFIGURED="false"
export DEVICE_INSTANCES="instance1,instance2,instance3"
```

### 2. Config File (Tương lai)

Có thể đặt file `device_config.json` tại `/etc/device_config.json` hoặc `./device_config.json`. Xem `device_config.example.json` để biết format.

### 3. System UUID

System UUID được tự động đọc từ:
- `/etc/machine-id` (systemd)
- `/sys/class/dmi/id/product_uuid` (DMI)
- `dmidecode -s system-uuid` (fallback)

Nếu không tìm thấy, sẽ sử dụng giá trị mặc định.

## Lưu ý

1. **Quyền truy cập**: Một số thông tin phần cứng có thể yêu cầu quyền root để đọc đầy đủ.

2. **Bảo mật**: Endpoint `/v1/core/system/reboot` rất nguy hiểm. Trong môi trường production, cần:
   - Thêm authentication/authorization
   - Sử dụng HTTPS
   - Rate limiting
   - Logging và monitoring

3. **Platform Support**: hwinfo hỗ trợ Linux, Windows, và macOS. Một số tính năng có thể khác nhau giữa các platform.

4. **Device Configuration**: Thông tin device có thể được tùy chỉnh thông qua environment variables hoặc config file.

## License

MIT License

## Tham khảo

- [hwinfo GitHub](https://github.com/lfreist/hwinfo)
- [cpp-httplib GitHub](https://github.com/yhirose/cpp-httplib)


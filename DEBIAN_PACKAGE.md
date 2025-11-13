# Building Debian Package for arm64

Hướng dẫn tạo package .deb cho hệ thống arm64.

## Yêu cầu

- Debian/Ubuntu system (có thể cross-compile từ x86_64)
- Build dependencies: `debhelper`, `fakeroot`, `cmake`, `g++`, `git`, `build-essential`
- Dependencies sẽ được tự động cài đặt khi chạy script `build_deb.sh`
- Hoặc cài đặt thủ công: `sudo apt-get install -y debhelper fakeroot cmake g++ git build-essential`

## Cách build

### Trên hệ thống arm64 (native build)
sudo 
```bash
./build_deb.sh
```

### Cross-compile từ x86_64

```bash
# Cài đặt cross-compiler
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build package
dpkg-buildpackage -b -us -uc -aarm64
```

## Cấu trúc package

Sau khi build, package sẽ chứa:

- `/opt/cvedix/monitor/metrics_monitor_system` - Binary executable
- `/opt/cvedix/monitor/config.json.example` - Example config
- `/opt/cvedix/monitor/device_config.example.json` - Example device config
- `/usr/bin/metrics_monitor_system` - Symlink to binary (for easy access)
- `/usr/share/doc/metrics-monitor-system/README.md` - Documentation

## Cài đặt package

### Trên máy đích (arm64)

```bash
# Copy file .deb đến máy đích
scp metrics-monitor-system_*.deb user@target:/tmp/

# Cài đặt
sudo dpkg -i /tmp/metrics-monitor-system_*.deb

# Cài đặt dependencies nếu thiếu
sudo apt-get install -f

# Tạo config file từ example
sudo cp /opt/cvedix/monitor/config.json.example /opt/cvedix/monitor/config.json
sudo nano /opt/cvedix/monitor/config.json

# Chạy service (sử dụng symlink)
metrics_monitor_system

# Hoặc chạy trực tiếp
/opt/cvedix/monitor/metrics_monitor_system
```

## Cấu hình sau khi cài đặt

1. **Tạo config file:**
   ```bash
   sudo cp /opt/cvedix/monitor/config.json.example /opt/cvedix/monitor/config.json
   sudo nano /opt/cvedix/monitor/config.json
   ```

2. **Chạy service:**
   ```bash
   # Sử dụng symlink (recommended)
   metrics_monitor_system
   
   # Hoặc chạy trực tiếp
   /opt/cvedix/monitor/metrics_monitor_system
   
   # Hoặc chạy với config tùy chỉnh
   /opt/cvedix/monitor/metrics_monitor_system /opt/cvedix/monitor/config.json
   ```

3. **Kiểm tra service:**
   ```bash
   curl http://localhost:8080/health
   curl http://localhost:8080/v1/core/system/info
   ```

## Gỡ cài đặt

```bash
# Gỡ cài đặt (giữ lại config)
sudo apt-get remove metrics-monitor-system

# Gỡ cài đặt và xóa config
sudo apt-get purge metrics-monitor-system
```

## Troubleshooting

### Lỗi build

- Đảm bảo đã chạy `./setup_dependencies.sh` trước khi build
- Kiểm tra dependencies: `dpkg-checkbuilddeps`

### Lỗi cài đặt

- Kiểm tra architecture: `dpkg --print-architecture` (phải là arm64)
- Cài đặt dependencies: `sudo apt-get install -f`

### Service không chạy

- Kiểm tra config: `/opt/cvedix/monitor/config.json`
- Kiểm tra permissions: `ls -l /opt/cvedix/monitor/metrics_monitor_system`
- Kiểm tra symlink: `ls -l /usr/bin/metrics_monitor_system`
- Kiểm tra logs: `journalctl -u metrics-monitor-system` (nếu có systemd service)


#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <string>

/**
 * Get current system status in JSON format
 * @return JSON string containing CPU usage, RAM usage, disk usage, etc.
 */
std::string get_system_status_json();

#endif // SYSTEM_STATUS_H


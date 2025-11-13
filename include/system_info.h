#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>

/**
 * Get detailed system hardware information in JSON format
 * @return JSON string containing CPU, RAM, GPU, Disk, Mainboard, OS information
 */
std::string get_system_info_json();

#endif // SYSTEM_INFO_H


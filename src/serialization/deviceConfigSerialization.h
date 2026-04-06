#ifndef deviceConfig_h
#define deviceConfig_h

#include "../simulator/device.h"

std::optional<DeviceConfig> loadDeviceConfig(std::string file);
void saveDeviceConfig(std::string file, const DeviceConfig& deviceConfig);

#endif /* deviceConfig_h */

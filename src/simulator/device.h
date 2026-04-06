#ifndef device_h
#define device_h

#include "impl/compressedStorage.h"
#include "impl/compressor.h"
#include "impl/decompressor.h"
#include "impl/dramDataCache.h"
#include "impl/loggerBlock.h"
#include "impl/metadataTable.h"
#include "impl/sramCache.h"
#include "router.h"

struct DeviceConfig {
	// Router
	Router router;
	// Limits
};

struct SDL_Window;
struct SDL_Renderer;

class Device {
public:
	void read(const Request& request);
	void write(const Request& request);
	Device() :
		compressedStorage(*this),
		compressor(*this),
		decompressor(*this),
		dramCache(*this),
		loggerBlock(*this),
		metadataTable(*this),
		sramCache(*this) {}

	void update() {
		compressedStorage.update();
		compressor.update();
		decompressor.update();
		dramCache.update();
		loggerBlock.update();
		metadataTable.update();
		sramCache.update();
	}
	void debugPrint() const {
		printf("------------------------------------\n");
		compressedStorage.debugPrint();
		compressor.debugPrint();
		decompressor.debugPrint();
		dramCache.debugPrint();
		loggerBlock.debugPrint();
		metadataTable.debugPrint();
		sramCache.debugPrint();
		printf("------------------------------------\n");
	}

	DeviceConfig& getDeviceConfig() { return deviceConfig; }
	const DeviceConfig& getDeviceConfig() const { return deviceConfig; }
	void setDeviceConfig(DeviceConfig&& deviceConfig) { this->deviceConfig = deviceConfig; }

	// these are public for the router
	CompressedStorage compressedStorage;
	Compressor compressor;
	Decompressor decompressor;
	DramDataCache dramCache;
	LoggerBlock<1> loggerBlock;
	MetadataTable metadataTable;
	SramCache sramCache;
private:
	DeviceConfig deviceConfig;
};

#endif /* device_h */

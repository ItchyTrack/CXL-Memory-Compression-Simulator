#ifndef device_h
#define device_h

#include "impl/compressedStorage.h"
#include "impl/compressor.h"
#include "impl/decompressor.h"
#include "impl/dramDataCache.h"
#include "impl/loggerBlock.h"
#include "impl/metadataTable.h"
#include "impl/sramCache.h"

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
		sramCache.update();
		loggerBlock.update();
	}

	// these are public for the router
	CompressedStorage compressedStorage;
	Compressor compressor;
	Decompressor decompressor;
	DramDataCache dramCache;
	LoggerBlock<1> loggerBlock;
	MetadataTable metadataTable;
	SramCache sramCache;
};

#endif /* device_h */

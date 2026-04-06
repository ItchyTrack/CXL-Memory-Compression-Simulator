#include "block.h"
#include "device.h"
#include <cassert>

template <string_litteral NAME>
bool OutputRouter<NAME>::route(const Request& request, const RouteArgs& args) {
	std::optional<BlockType> blockType = stringToBlockType(NAME.value);
	assert(blockType.has_value());
	std::vector<std::tuple<BlockType, unsigned int, Request>> whereRouteTo = device.getDeviceConfig().router.router(blockType.value(), request, args);
	for (const auto& [blockType, port, mutatedRequest] : whereRouteTo) {
		switch (blockType) {
		case COMPRESSED_STORAGE: device.compressedStorage.getIputInterface().pushRequest(port, mutatedRequest); break;
		case COMPRESSOR: device.compressor.getIputInterface().pushRequest(port, mutatedRequest); break;
		case DECOMPRESSOR: device.decompressor.getIputInterface().pushRequest(port, mutatedRequest); break;
		case DRAM_DATA_CACHE: device.dramCache.getIputInterface().pushRequest(port, mutatedRequest); break;
		case METADATA_TABLE: device.metadataTable.getIputInterface().pushRequest(port, mutatedRequest); break;
		case SRAM_CACHE: device.sramCache.getIputInterface().pushRequest(port, mutatedRequest); break;
		case LOGGER_BLOCK: device.loggerBlock.getIputInterface().pushRequest(port, mutatedRequest); break;
		}
	}
}

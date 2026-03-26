#include "../device.h"

#include <assert.h>
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define unreachable() assert(false && "unreachable code hit");

// all the routing happens in here because then it is easier to find and update everything

bool CompressedStorageRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { } break;
		case WRITE: { } break;
		case CACHE_EVICT: { } break;
		case COMPRESS: { } break;
	}
	device.decompressor.getIputInterface().pushRequest(0, request);
	return true;
}

bool CompressorRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { unreachable(); } break;
		case WRITE: { unreachable(); } break;
		case CACHE_EVICT: { } break;
		case COMPRESS: { } break;
	}
	device.compressedStorage.getIputInterface().pushRequest(1, request);
	return true;
}

bool DecompressorRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { } break;
		case WRITE: { } break;
		case CACHE_EVICT: { } break;
		case COMPRESS: { unreachable(); } break;
	}
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}

bool DramDataCacheRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { } break;
		case WRITE: { } break;
		case CACHE_EVICT: { } break;
		case COMPRESS: { } break;
	}
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}

bool MetadataTableRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { } break;
		case WRITE: { } break;
		case CACHE_EVICT: { } break;
		case COMPRESS: { } break;
	}
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}

bool SramCacheRouter::route(const Request& request, bool found) {
	switch (request.action) {
		case READ: {
			if (found) {
				device.dramCache.getIputInterface().pushRequest(0, request);
			} else {
				device.metadataTable.getIputInterface().pushRequest(0, request);
			}
		} break;
		case WRITE: {
			if (found) {
				device.dramCache.getIputInterface().pushRequest(0, request);
			} else {
				device.metadataTable.getIputInterface().pushRequest(0, request);
			}
		} break;
		case CACHE_EVICT: { unreachable(); } break;
		case COMPRESS: { unreachable(); } break;
	}
	return true;
}

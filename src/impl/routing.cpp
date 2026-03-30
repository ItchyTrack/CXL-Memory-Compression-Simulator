#include "../device.h"

#include <assert.h>
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define unreachable() assert(false && "unreachable code hit");

// all the routing happens in here because then it is easier to find and update everything

bool CompressedStorageRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { printf("CompressedStorageRouter READ: TODO!\n"); } break;
		case WRITE: { printf("CompressedStorageRouter WRITE: TODO!\n"); } break;
		case CACHE_EVICT: { printf("CompressedStorageRouter CACHE_EVICT: TODO!\n"); } break;
		case COMPRESS: { printf("CompressedStorageRouter COMPRESS: TODO!\n"); } break;
	}
	device.decompressor.getIputInterface().pushRequest(0, request);
	return true;
}

bool CompressorRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { unreachable(); } break;
		case WRITE: { unreachable(); } break;
		case CACHE_EVICT: { printf("CompressorRouter CACHE_EVICT: TODO!\n"); } break;
		case COMPRESS: { printf("CompressorRouter COMPRESS: TODO!\n"); } break;
	}
	device.compressedStorage.getIputInterface().pushRequest(1, request);
	return true;
}

bool DecompressorRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { printf("DecompressorRouter READ: TODO!\n"); } break;
		case WRITE: { printf("DecompressorRouter WRITE: TODO!\n"); } break;
		case CACHE_EVICT: { printf("DecompressorRouter CACHE_EVICT: TODO!\n"); } break;
		case COMPRESS: { unreachable(); } break;
	}
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}

bool DramDataCacheRouter::route(const Request& request) {
	switch (request.action) {
		case READ: { printf("DramDataCacheRouter READ: TODO!\n"); } break;
		case WRITE: { printf("DramDataCacheRouter WRITE: TODO!\n"); } break;
		case CACHE_EVICT: { printf("DramDataCacheRouter CACHE_EVICT: TODO!\n"); } break;
		case COMPRESS: { printf("DramDataCacheRouter COMPRESS: TODO!\n"); } break;
	}
	device.loggerBlock.getIputInterface().pushRequest(0, request);
	return true;
}

bool MetadataTableRouter::route(const Request& request, bool DRC_valid, bool CSA_valid) {
	switch (request.action) {
		case READ: {
			if (DRC_valid) {
				device.dramCache.getIputInterface().pushRequest(0, request);
			} else {
				// CSA_valid should be true in this case but because we are randomly settting these that might not be true
				if (!CSA_valid) {
					printf("MetadataTable READ: Promote to SL2: Rd-Decomp-Wr to DRC\n");
				}
			}
		} break;
		case WRITE: {
			if (DRC_valid) {
				printf("MetadataTable WRITE: Promote Block to SL1\n");
			} else {
				if (CSA_valid) {
					printf("MetadataTable WRITE: Promote to SL2: Rd-Decomp-Wr to DRC\n");
				} else {
					printf("MetadataTable WRITE: Promote to SL2: Set DRC_valid\n");
				}
			}
		} break;
		case CACHE_EVICT: { printf("MetadataTable CACHE_EVICT: TODO!\n"); } break;
		case COMPRESS: { printf("MetadataTable COMPRESS: TODO!\n"); } break;
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

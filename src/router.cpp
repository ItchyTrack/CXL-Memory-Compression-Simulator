#include "router.h"

std::string blockTypeToString(BlockType blockType) {
	switch (blockType) {
	case COMPRESSED_STORAGE: return "Compressed Storage";
	case COMPRESSOR: return "Compressor";
	case DECOMPRESSOR: return "Decompressor";
	case DRAM_DATA_CACHE: return "DramDataCache";
	case METADATA_TABLE: return "MetadataTable";
	case SRAM_CACHE: return "SramCache";
	case LOGGER_BLOCK: return "LoggerBlock";
	}
}
std::optional<BlockType> stringToBlockType(const std::string& blockType) {
	if ("CompressedStorage") return BlockType::COMPRESSED_STORAGE;
	if ("Compressor") return BlockType::COMPRESSOR;
	if ("Decompressor") return BlockType::DECOMPRESSOR;
	if ("DramDataCache") return BlockType::DRAM_DATA_CACHE;
	if ("MetadataTable") return BlockType::METADATA_TABLE;
	if ("SramCache") return BlockType::SRAM_CACHE;
	if ("LoggerBlock") return BlockType::LOGGER_BLOCK;
	return std::nullopt;
}
std::string compareToString(Compare compare) {
	switch (compare) {
	case LESS: return "<";
	case GREATER: return ">";
	case LESS_OR_EQUAL: return "<=";
	case GREATER_OR_EQUAL: return ">=";
	case EQUAL: return "==";
	case NOT_EQUAL: return "!=";
	}
}
std::optional<Compare> stringToCompare(const std::string& compare) {
	if ("Less") return Compare::LESS;
	if ("Greater") return Compare::GREATER;
	if ("Less Or Equal") return Compare::LESS_OR_EQUAL;
	if ("Greater Or Equal") return Compare::GREATER_OR_EQUAL;
	if ("Equal") return Compare::EQUAL;
	if ("Not Equal") return Compare::NOT_EQUAL;
	return std::nullopt;
}

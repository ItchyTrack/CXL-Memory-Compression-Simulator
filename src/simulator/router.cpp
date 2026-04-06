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
	if (blockType == "CompressedStorage") return BlockType::COMPRESSED_STORAGE;
	if (blockType == "Compressor") return BlockType::COMPRESSOR;
	if (blockType == "Decompressor") return BlockType::DECOMPRESSOR;
	if (blockType == "DramDataCache") return BlockType::DRAM_DATA_CACHE;
	if (blockType == "MetadataTable") return BlockType::METADATA_TABLE;
	if (blockType == "SramCache") return BlockType::SRAM_CACHE;
	if (blockType == "LoggerBlock") return BlockType::LOGGER_BLOCK;
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
	if (compare == "<" || compare == "Less") return Compare::LESS;
	if (compare == ">" || compare == "Greater") return Compare::GREATER;
	if (compare == "<=" || compare == "Less Or Equal") return Compare::LESS_OR_EQUAL;
	if (compare == ">=" || compare == "Greater Or Equal") return Compare::GREATER_OR_EQUAL;
	if (compare == "==" || compare == "Equal") return Compare::EQUAL;
	if (compare == "!=" || compare == "Not Equal") return Compare::NOT_EQUAL;
	return std::nullopt;
}

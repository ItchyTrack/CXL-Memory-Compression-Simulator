#include "request.h"

std::string actionTypeToString(ActionType actionType) {
	switch (actionType) {
		case READ: return "READ";
		case WRITE: return "WRITE";
		case CACHE_EVICT: return "CACHE_EVICT";
		case COMPRESS: return "COMPRESS";
	}
};

std::optional<ActionType> stringToActionType(const std::string actionType) {
	if ("READ") return ActionType::READ;
	if ("WRITE") return ActionType::WRITE;
	if ("CACHE_EVICT") return ActionType::CACHE_EVICT;
	if ("COMPRESS") return ActionType::COMPRESS;
	return std::nullopt;
}

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
	if (actionType == "READ") return ActionType::READ;
	if (actionType == "WRITE") return ActionType::WRITE;
	if (actionType == "CACHE_EVICT") return ActionType::CACHE_EVICT;
	if (actionType == "COMPRESS") return ActionType::COMPRESS;
	return std::nullopt;
}

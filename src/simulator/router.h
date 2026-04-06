#ifndef router_h
#define router_h

#include "request.h"
#include <map>

// args is a map that the blocks construct before passing them out of the block
// we can do fancy stuff with the string to make it faster later

enum BlockType {
	COMPRESSED_STORAGE,
	COMPRESSOR,
	DECOMPRESSOR,
	DRAM_DATA_CACHE,
	METADATA_TABLE,
	SRAM_CACHE,
	LOGGER_BLOCK,
};

std::string blockTypeToString(BlockType compare);
std::optional<BlockType> stringToBlockType(const std::string& compare);

enum Compare {
	LESS,
	GREATER,
	LESS_OR_EQUAL,
	GREATER_OR_EQUAL,
	EQUAL,
	NOT_EQUAL,
};

std::string compareToString(Compare blockType);
std::optional<Compare> stringToCompare(const std::string& blockType);

typedef std::unordered_map<std::string, unsigned int> RouteArgs;

struct Condition {
	bool check(const RouteArgs& args) const {
		for (const auto& condition : conditions) {
			auto argsIter = args.find(std::get<0>(condition));
			if (argsIter == args.end()) {
				printf("ERROR: Condition arg %s not found args. Check returning false.", std::get<0>(condition).c_str());
				return false;
			}
			switch (std::get<1>(condition)) {
			case LESS: {
				if (std::get<2>(condition) <= argsIter->second) return false;
			} break;
			case GREATER: {
				if (std::get<2>(condition) >= argsIter->second) return false;
			} break;
			case LESS_OR_EQUAL: {
				if (std::get<2>(condition) < argsIter->second) return false;
			} break;
			case GREATER_OR_EQUAL: {
				if (std::get<2>(condition) > argsIter->second) return false;
			} break;
			case EQUAL: {
				if (std::get<2>(condition) != argsIter->second) return false;
			} break;
			case NOT_EQUAL: {
				if (std::get<2>(condition) == argsIter->second) return false;
			} break;
			}
		}
		return true;
	}
	std::vector<std::tuple<std::string, Compare, int>> conditions;
};

struct RequestMutator {
	Request mutate(const Request& request, const RouteArgs& args) const {
		Request outRequest = request;
		if (setActionType.has_value()) outRequest.action = setActionType.value();
		return outRequest;
	}
	std::optional<ActionType> setActionType;
};

struct RouterOption {
	Condition condition;
	RequestMutator mutator;
	std::pair<BlockType, int> output; // (block, port)
};

typedef std::map<BlockType, std::map<ActionType, std::vector<RouterOption>>> RouterData;

struct SDL_Window;
struct SDL_Renderer;

class Router {
	friend class RouterEditor;
public:
	const RouterData& getData() const { return routerData; }
	void setData(RouterData&& routerData) { this->routerData = routerData; }
	std::vector<std::tuple<BlockType, unsigned int, Request>> router(BlockType blockType, const Request& request, const RouteArgs& args) const {
		auto routerDataIter = routerData.find(blockType);
		if (routerDataIter == routerData.end()) {
			printf("ERROR: BlockType %s not found in routerData.", blockTypeToString(blockType).c_str());
			// std::flush(std::cout);
			// exit(EXIT_FAILURE); // maybe just return empty vector
			return {};
		}
		auto actionsIter = routerDataIter->second.find(request.action);
		if (actionsIter == routerDataIter->second.end()) {
			printf("ERROR: ActionType %s not found in routerData for BlockType %s.", actionTypeToString(request.action).c_str(), blockTypeToString(blockType).c_str());
			// std::flush(std::cout);
			// exit(EXIT_FAILURE); // maybe just return empty vector
			return {};
		}
		std::vector<std::tuple<BlockType, unsigned int, Request>> toSend;
		for (const RouterOption& routerOption : actionsIter->second) {
			if (routerOption.condition.check(args)) {
				toSend.emplace_back(routerOption.output.first, routerOption.output.second, routerOption.mutator.mutate(request, args));
			}
		}
		return toSend; // valid to be empty if no more work need to be done
	}

private:
	RouterData routerData;
};

#endif /* router_h */

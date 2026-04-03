#include "deviceConfigSerialization.h"
#include "../external/json.hpp"

#include <fstream>
using json = nlohmann::json;
/*
{
	"Routing": {
		"BlockType": {
			"ActionType": [
				{
					"Condition": [
						{
							"Type": Always "Int Compare",
							For compare:
							"Value": Int value
							"Compare": "Compare",
							"Arg": "Arg",
						}
					],
					"Mutator" { // if none will do nothing
						"SetActionType": "Which ActionType to set it to"
					},
					"Output" {
						"BlockType": BlockType
						"Port": port number
					}
				}
			]
		}
	}
	"Limits": {
		There are none right now
	}
}
*/

template<typename T>
std::optional<T> jsonGet(const nlohmann::json& json, const std::string& key) {
	auto iter = json.find(key);
	if (iter != json.end() && !iter->is_null()) {
		if constexpr (std::is_same_v<T, int>) {
			if (iter->is_number_integer()) {
				return iter->get<int>();
			}
		} else if constexpr (std::is_same_v<T, std::string>) {
			if (iter->is_string()) {
				return iter->get<std::string>();
			}
		} else if constexpr (std::is_same_v<T, nlohmann::json>) {
			return *iter;
		}
	}
	return std::nullopt;
}

std::optional<DeviceConfig> loadDeviceConfig(std::string filePath) {
	std::fstream file(filePath);
	if (!file.is_open()) {
		printf("ERROR: Failed to find DeviceConfig file %s.\n", filePath.c_str());
		return std::nullopt;
	}

	json jsonData = json::parse(file, nullptr, false, true, true);
	file.close();

	RouterData routerData;
	if (auto routing = jsonGet<json>(jsonData, "Routing"); routing.has_value()) {
		for (auto& blockTypeIter : routing->items()) {
			auto blockType = stringToBlockType(blockTypeIter.key());
			if (!blockType.has_value()) continue;

			for (auto& actionTypeIter : blockTypeIter.value().items()) {
				std::optional<ActionType> actionType = stringToActionType(actionTypeIter.key());
				if (!actionType.has_value()) continue;
				std::vector<RouterOption> routerOptions;
				for (auto& optionJson : actionTypeIter.value()) {
					RouterOption routerOption;
					if (auto conditions = jsonGet<json>(optionJson, "Condition"); conditions.has_value() && conditions->is_array()) {
						for (auto& condition : *conditions) {
							if (auto typeString = jsonGet<std::string>(condition, "Type"); typeString.has_value()) {
								if (typeString.value() == "Int Compare") {
									std::optional<std::string> argString = jsonGet<std::string>(condition, "Arg");
									std::optional<int> valueInt = jsonGet<int>(condition, "Value");
									std::optional<std::string> compareString = jsonGet<std::string>(condition, "Compare");
									if (compareString.has_value() && argString.has_value() && valueInt.has_value()) {
										std::optional<Compare> compare = stringToCompare(compareString.value());
										if (compare.has_value()) {
											routerOption.condition.conditions.emplace_back(argString.value(), compare.value(), valueInt.value());
										}
									}
								}
							}
						}
					}
					if (auto mutator = jsonGet<json>(optionJson, "Mutator"); mutator.has_value()) {
						if (auto setActionType = jsonGet<std::string>(*mutator, "SetActionType"); setActionType.has_value()) {
							routerOption.mutator.setActionType = stringToActionType(setActionType.value());
						} else {
							routerOption.mutator.setActionType = std::nullopt;
						}
					}
					if (auto output = jsonGet<json>(optionJson, "Output"); output.has_value()) {
						if (auto blockString = jsonGet<std::string>(*output, "Block"); blockString.has_value()) {
							if (auto portInt = jsonGet<int>(*output, "Port"); portInt.has_value()) {
								auto outputBlock = stringToBlockType(blockString.value());
								if (outputBlock.has_value()) {
									routerOption.output = { outputBlock.value(), portInt.value() };
								}
							}
						}
					}
					routerOptions.push_back(routerOption);
				}
				routerData[blockType.value()][actionType.value()] = routerOptions;
			}
		}
	}

	DeviceConfig deviceConfig;
	deviceConfig.router.setData(std::move(routerData));

	return deviceConfig;
}

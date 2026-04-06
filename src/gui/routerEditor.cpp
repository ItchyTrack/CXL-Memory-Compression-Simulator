#include "routerEditor.h"
#include "../external/imgui.h"
#include "../external/imgui_internal.h"
#include "../external/nodes/imgui_node_editor.h"
#include <SDL3/SDL_dialog.h>
#include <algorithm>
#include <cstring>
#include "../simulator/device.h"
#include "../serialization/deviceConfigSerialization.h"

namespace nodeEditor = ax::NodeEditor;

// ── ID helpers ────────────────────────────────────────────────────────────────
int RouterEditor::linkId(BlockType src, ActionType action, int idx) { return ((int)src * 100 + (int)action) * 1000 + idx; }

// ── Layout & colour ───────────────────────────────────────────────────────────
ImVec2 RouterEditor::nodeLayout(BlockType blockType) {
	static const ImVec2 pos[] = {
		{ 700, 250 }, // COMPRESSED_STORAGE
		{ 460, 100 }, // COMPRESSOR
		{ 460, 350 }, // DECOMPRESSOR
		{ 230, 230 }, // DRAM_DATA_CACHE
		{ 230, 430 }, // METADATA_TABLE
		{ 0, 100 },	  // SRAM_CACHE
	};
	static_assert(sizeof(pos) / sizeof(pos[0]) == 6, "nodeLayout: entry count must match BlockType count");
	return pos[(int)blockType];
}

ImU32 RouterEditor::linkColor(const RouterOption& opt) {
	if (opt.mutator.setActionType.has_value()) return IM_COL32(220, 80, 80, 255); // red   – mutating
	if (!opt.condition.conditions.empty()) return IM_COL32(230, 160, 40, 255);	  // orange – conditional
	return IM_COL32(80, 200, 120, 255);											  // green  – always
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────
RouterEditor::RouterEditor(
	SDL_Window* window,
	Device& device,
	std::function<void(BlockType)> onBlockSelected
) : sdlWindow(window), device(device), onBlockSelected(std::move(onBlockSelected)) {
	nodeEditor::Config cfg;
	cfg.SettingsFile = "RouterEditor.json";
	cfg.EnableSmoothZoom = true;
	ctx = nodeEditor::CreateEditor(&cfg);
}

RouterEditor::~RouterEditor() { nodeEditor::DestroyEditor(ctx); }

// ── File-dialog callbacks (may be called on a worker thread by SDL3) ──────────
void SDLCALL RouterEditor::fileDialogLoadCB(void* userdata, const char* const* filelist, int /*filter*/) {
	auto* p = static_cast<PendingPath*>(userdata);
	if (!filelist || !filelist[0]) return;
	std::lock_guard<std::mutex> lk(p->mtx);
	p->path = filelist[0];
	p->ready = true;
}

void SDLCALL RouterEditor::fileDialogSaveCB(void* userdata, const char* const* filelist, int /*filter*/) {
	auto* p = static_cast<PendingPath*>(userdata);
	if (!filelist || !filelist[0]) return;
	std::lock_guard<std::mutex> lk(p->mtx);
	p->path = filelist[0];
	p->ready = true;
}

void RouterEditor::flushPendingPaths() {
	{
		std::lock_guard<std::mutex> lk(pendingLoad.mtx);
		if (pendingLoad.ready) {
			/*onLoad=*/
			if (auto cfg = loadDeviceConfig(pendingLoad.path))
				if (cfg.has_value()) device.setDeviceConfig(std::move(cfg.value()));
			pendingLoad.path.clear();
			pendingLoad.ready = false;
		}
	}
	{
		std::lock_guard<std::mutex> lk(pendingSave.mtx);
		if (pendingSave.ready) {
			saveDeviceConfig(pendingLoad.path, device.getDeviceConfig());
		pendingSave.path.clear();
		pendingSave.ready = false;
		}
	}
}

// ── Top-level render ──────────────────────────────────────────────────────────
void RouterEditor::render() {
	flushPendingPaths();
	renderToolbar();
	ImGui::Separator();

	rebuildLinkMeta();

	constexpr float kInspectorWidth = 340.0f;
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 graphSize = { avail.x - kInspectorWidth - 8.0f, avail.y };

	ImGui::BeginChild("##graph", graphSize);
	renderGraph();
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("##inspector", { kInspectorWidth, 0 }, ImGuiChildFlags_Borders);
	renderInspector();
	ImGui::EndChild();
}

// ── Toolbar ───────────────────────────────────────────────────────────────────
void RouterEditor::renderToolbar() {
	static const SDL_DialogFileFilter kJsonFilter[] = {
		{ "Device config (*.json)", "json" },
		{ "All files", "*" },
	};

	if (ImGui::Button("Load Config")) {
		SDL_ShowOpenFileDialog(fileDialogLoadCB, &pendingLoad, sdlWindow, kJsonFilter, 2, nullptr, false);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save Config")) {
		SDL_ShowSaveFileDialog(fileDialogSaveCB, &pendingSave, sdlWindow, kJsonFilter, 2, nullptr);
	}
	ImGui::SameLine(0, 20);
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine(0, 20);

	ImGui::TextUnformatted("Action layer:");
	ImGui::SameLine();

	if (ImGui::BeginTabBar("##actionTabs")) {
		for (ActionType at : { READ, WRITE, CACHE_EVICT, COMPRESS }) {
			const std::string label = actionTypeToString(at) + "##tab" + std::to_string((int)at);
			if (ImGui::BeginTabItem(label.c_str())) {
				if (viewedAction != at) {
					viewedAction = at;
					selectedLink = std::nullopt;
				}
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	// Legend
	ImGui::SameLine(0, 40);
	ImGui::ColorButton("##cGreen", { 0.31f, 0.78f, 0.47f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine();
	ImGui::TextDisabled("Always");
	ImGui::SameLine(0, 16);
	ImGui::ColorButton("##cOrange", { 0.90f, 0.63f, 0.16f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine();
	ImGui::TextDisabled("Conditional");
	ImGui::SameLine(0, 16);
	ImGui::ColorButton("##cRed", { 0.86f, 0.31f, 0.31f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine();
	ImGui::TextDisabled("Mutating");
}

// ── Link metadata ─────────────────────────────────────────────────────────────
void RouterEditor::rebuildLinkMeta() {
	linkMetaMap.clear();
	for (const auto& [srcBlock, actionMap] : device.getDeviceConfig().router.getData()) {
		auto it = actionMap.find(viewedAction);
		if (it == actionMap.end()) continue;
		for (int idx = 0; idx < (int)it->second.size(); idx++) {
			int lid = linkId(srcBlock, viewedAction, idx);
			linkMetaMap[lid] = { srcBlock, viewedAction, idx };
		}
	}
}

// ── Node graph ────────────────────────────────────────────────────────────────
void RouterEditor::renderGraph() {
	nodeEditor::SetCurrentEditor(ctx);
	nodeEditor::Begin("RouterGraph");

	for (int i = 0; i < 6; i++) {
		BlockType bt = static_cast<BlockType>(i);
		if (!positionsSet) nodeEditor::SetNodePosition(nodeId(bt), nodeLayout(bt));
		drawNode(bt);
	}
	positionsSet = true;

	for (const auto& [lid, meta] : linkMetaMap) {
		const auto& vec = device.getDeviceConfig().router.getData().at(meta.srcBlock).at(meta.srcAction);
		const RouterOption& opt = vec[meta.optionIndex];
		nodeEditor::Link(lid, outputPinId(meta.srcBlock, 0), inputPinId(opt.output.first, opt.output.second), ImGui::ColorConvertU32ToFloat4(linkColor(opt)), 2.5f);
	}

	if (selectedLink.has_value()) nodeEditor::SelectLink(*selectedLink, false);

	handleCreation();
	handleDeletion();
	handleNodeSelection();

	nodeEditor::End();

	// Harvest link selection
	{
		std::vector<nodeEditor::LinkId> sel(nodeEditor::GetSelectedObjectCount());
		int n = nodeEditor::GetSelectedLinks(sel.data(), (int)sel.size());
		if (n == 1) selectedLink = sel[0];
		else if (n == 0) selectedLink = std::nullopt;
	}

	nodeEditor::SetCurrentEditor(nullptr);
}

void RouterEditor::drawNode(BlockType blockType) {
	nodeEditor::BeginNode(nodeId(blockType));
	ImGui::TextUnformatted(blockTypeToString(blockType).c_str());
	ImGui::Spacing();

	ImGui::BeginGroup();
	for (int port = 0; port < 2; port++) {
		nodeEditor::BeginPin(inputPinId(blockType, port), nodeEditor::PinKind::Input);
		ImGui::Text("in[%d]", port);
		nodeEditor::EndPin();
	}
	ImGui::EndGroup();

	ImGui::SameLine(0, 60);

	ImGui::BeginGroup();
	nodeEditor::BeginPin(outputPinId(blockType, 0), nodeEditor::PinKind::Output);
	ImGui::TextUnformatted("out");
	nodeEditor::EndPin();
	ImGui::EndGroup();

	nodeEditor::EndNode();
}

// ── Node-click → block selection ─────────────────────────────────────────────
void RouterEditor::handleNodeSelection() {
	std::vector<nodeEditor::NodeId> sel(nodeEditor::GetSelectedObjectCount());
	int n = nodeEditor::GetSelectedNodes(sel.data(), (int)sel.size());

	if (n == 1) {
		const int nid = (int)sel[0].Get();
		// nodeId(bt) == bt+1  →  bt == nid-1
		if (nid >= 1 && nid <= 6) {
			BlockType bt = static_cast<BlockType>(nid - 1);
			if (!selectedNode.has_value() || *selectedNode != sel[0]) {
				selectedNode = sel[0];
				if (onBlockSelected) onBlockSelected(bt);
			}
		}
	} else if (n == 0) {
		selectedNode = std::nullopt;
	}
}

// ── Link creation / deletion ──────────────────────────────────────────────────
void RouterEditor::handleCreation() {
	if (!nodeEditor::BeginCreate(ImColor(255, 255, 255), 2.0f)) {
		nodeEditor::EndCreate();
		return;
	}

	nodeEditor::PinId startPin, endPin;
	if (nodeEditor::QueryNewLink(&startPin, &endPin) && startPin && endPin) {
		const int startId = (int)startPin.Get();
		const int endId = (int)endPin.Get();
		const bool startIsOutput = startId >= 2000;
		const bool endIsInput = endId >= 1000 && endId < 2000;

		if (!startIsOutput || !endIsInput) {
			nodeEditor::RejectNewItem(ImColor(255, 60, 60), 2.0f);
		} else {
			const BlockType srcBlock = static_cast<BlockType>((startId - 2000) / 10);
			const BlockType dstBlock = static_cast<BlockType>((endId - 1000) / 10);
			const int dstPort = (endId - 1000) % 10;
			if (nodeEditor::AcceptNewItem(ImColor(80, 200, 120), 2.0f)) {
				RouterOption opt;
				opt.output = { dstBlock, dstPort };
				device.getDeviceConfig().router.routerData[srcBlock][viewedAction].push_back(opt);
				rebuildLinkMeta();
				int newIdx = (int)device.getDeviceConfig().router.routerData[srcBlock][viewedAction].size() - 1;
				selectedLink = linkId(srcBlock, viewedAction, newIdx);
			}
		}
	}
	nodeEditor::EndCreate();
}

void RouterEditor::handleDeletion() {
	if (!nodeEditor::BeginDelete()) {
		nodeEditor::EndDelete();
		return;
	}

	nodeEditor::LinkId deletedId;
	while (nodeEditor::QueryDeletedLink(&deletedId)) {
		if (!nodeEditor::AcceptDeletedItem()) continue;
		const int lid = (int)deletedId.Get();
		auto it = linkMetaMap.find(lid);
		if (it == linkMetaMap.end()) continue;
		const auto& [src, action, idx] = it->second;
		auto& vec = device.getDeviceConfig().router.routerData[src][action];
		if (idx >= 0 && idx < (int)vec.size()) vec.erase(vec.begin() + idx);
		if (selectedLink == deletedId) selectedLink = std::nullopt;
		rebuildLinkMeta();
	}
	nodeEditor::EndDelete();
}

// ── Link inspector ────────────────────────────────────────────────────────────
void RouterEditor::renderInspector() {
	if (!selectedLink.has_value()) {
		ImGui::TextDisabled("Click a link to inspect it.");
		ImGui::Spacing();
		ImGui::TextDisabled("Click a node to inspect the block.");
		return;
	}

	const int lid = (int)selectedLink->Get();
	auto it = linkMetaMap.find(lid);
	if (it == linkMetaMap.end()) {
		ImGui::TextDisabled("(stale selection)");
		return;
	}

	auto& [srcBlock, action, idx] = it->second;
	auto& vec = device.getDeviceConfig().router.routerData[srcBlock][action];
	if (idx < 0 || idx >= (int)vec.size()) return;

	ImGui::Separator();

	if (ImGui::Button("Delete Connection")) {
		vec.erase(vec.begin() + idx);
		selectedLink = std::nullopt;
		rebuildLinkMeta();
		return;
	}

	RouterOption& opt = vec[idx];
	ImGui::Text("%s -> %s [port %d]", blockTypeToString(srcBlock).c_str(), blockTypeToString(opt.output.first).c_str(), opt.output.second);
	ImGui::TextDisabled("Action: %s", actionTypeToString(action).c_str());
	ImGui::Separator();

	ImGui::TextUnformatted("Destination");
	static const char* kBlockNames[] = { "COMPRESSED_STORAGE", "COMPRESSOR", "DECOMPRESSOR", "DRAM_DATA_CACHE", "METADATA_TABLE", "SRAM_CACHE" };
	static_assert(sizeof(kBlockNames) / sizeof(kBlockNames[0]) == 6, "kBlockNames must match BlockType count");

	int dstIdx = (int)opt.output.first;
	ImGui::SetNextItemWidth(180);
	if (ImGui::Combo("Block##dst", &dstIdx, kBlockNames, 6)) opt.output.first = static_cast<BlockType>(dstIdx);

	ImGui::SameLine();
	int port = opt.output.second;
	ImGui::SetNextItemWidth(60);
	if (ImGui::InputInt("Port##dst", &port)) opt.output.second = std::clamp(port, 0, 1);

	ImGui::Separator();
	renderConditionEditor(opt.condition);
	ImGui::Separator();
	renderMutatorEditor(opt.mutator);
}

// ── Condition editor ──────────────────────────────────────────────────────────
void RouterEditor::renderConditionEditor(Condition& cond) {
	ImGui::TextUnformatted("Conditions");
	ImGui::SameLine();
	if (ImGui::SmallButton("+ Add")) cond.conditions.emplace_back("arg", EQUAL, 0);

	if (cond.conditions.empty()) {
		ImGui::TextDisabled("  (none — always fires)");
		return;
	}

	static const char* kCompareOps[] = { "<", ">", "<=", ">=", "==", "!=" };
	const float rowHeight = ImGui::GetFrameHeightWithSpacing();
	const float childHeight = std::min<float>(rowHeight * (float)cond.conditions.size() + 12.f, 160.f);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##conds", { 0, childHeight }, ImGuiChildFlags_Borders);

	int eraseAt = -1;
	for (int i = 0; i < (int)cond.conditions.size(); i++) {
		auto& [key, compare, value] = cond.conditions[i];
		ImGui::PushID(i);

		char buf[64];
		strncpy(buf, key.c_str(), sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		ImGui::SetNextItemWidth(90);
		if (ImGui::InputText("##k", buf, sizeof(buf))) key = buf;

		ImGui::SameLine(0, 4);
		int cmpI = (int)compare;
		ImGui::SetNextItemWidth(48);
		if (ImGui::Combo("##c", &cmpI, kCompareOps, 6)) compare = static_cast<Compare>(cmpI);

		ImGui::SameLine(0, 4);
		ImGui::SetNextItemWidth(64);
		ImGui::InputInt("##v", &value, 0);

		ImGui::SameLine(0, 4);
		if (ImGui::SmallButton("✕")) eraseAt = i;

		ImGui::PopID();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();

	if (eraseAt >= 0) cond.conditions.erase(cond.conditions.begin() + eraseAt);
}

// ── Mutator editor ────────────────────────────────────────────────────────────
void RouterEditor::renderMutatorEditor(RequestMutator& mut) {
	static const char* kActionNames[] = { "READ", "WRITE", "CACHE_EVICT", "COMPRESS" };

	ImGui::TextUnformatted("Mutator");
	bool hasOverride = mut.setActionType.has_value();
	if (ImGui::Checkbox("Override ActionType##mut", &hasOverride)) mut.setActionType = hasOverride ? std::optional<ActionType>(READ) : std::nullopt;

	if (hasOverride) {
		int idx = (int)mut.setActionType.value();
		ImGui::SetNextItemWidth(140);
		if (ImGui::Combo("##mutAction", &idx, kActionNames, 4)) mut.setActionType = static_cast<ActionType>(idx);
	}
}

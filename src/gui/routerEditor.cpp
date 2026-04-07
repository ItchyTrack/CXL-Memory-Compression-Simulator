#include "routerEditor.h"
#include "../external/imgui.h"
#include "../external/imgui_internal.h"
#include "../external/nodes/imgui_node_editor.h"
#include "../simulator/device.h"
#include "../serialization/deviceConfigSerialization.h"
#include <SDL3/SDL_dialog.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace nodeEditor = ax::NodeEditor;

// ── ID helpers ────────────────────────────────────────────────────────────────
int RouterEditor::linkId(BlockType src, ActionType action, int idx) {
	return ((int)src * 100 + (int)action) * 1000 + idx;
}

// ── Default node positions ────────────────────────────────────────────────────
ImVec2 RouterEditor::nodeDefaultPos(BlockType bt) {
	static const ImVec2 pos[] = {
		{ 700, 250 }, // COMPRESSED_STORAGE
		{ 460, 100 }, // COMPRESSOR
		{ 460, 350 }, // DECOMPRESSOR
		{ 230, 230 }, // DRAM_DATA_CACHE
		{ 230, 430 }, // METADATA_TABLE
		{   0, 100 }, // SRAM_CACHE
	};
	static_assert(sizeof(pos) / sizeof(pos[0]) == 6, "nodeDefaultPos: entry count must match visible BlockType count");
	return pos[(int)bt];
}

// ── Link colour ───────────────────────────────────────────────────────────────
ImU32 RouterEditor::linkColor(const RouterOption& opt) {
	if (opt.mutator.setActionType.has_value()) return IM_COL32(220,  80,  80, 255); // red    - mutating
	if (!opt.condition.conditions.empty())     return IM_COL32(230, 160,  40, 255); // orange - conditional
	return                                            IM_COL32( 80, 200, 120, 255); // green  - always
}

// ── Live state helpers ────────────────────────────────────────────────────────
unsigned int RouterEditor::getQueueDepth(BlockType bt, int port) const {
	auto depth = [port](const auto& blk) -> unsigned int {
		const auto& bi = blk.getIputInterface();
		if ((unsigned)port < (unsigned)bi.inputBuffers.size())
			return (unsigned int)bi.inputBuffers[port].size();
		return 0u;
	};
	switch (bt) {
	case COMPRESSED_STORAGE: return depth(device.compressedStorage);
	case COMPRESSOR:         return depth(device.compressor);
	case DECOMPRESSOR:       return depth(device.decompressor);
	case DRAM_DATA_CACHE:    return depth(device.dramCache);
	case METADATA_TABLE:     return depth(device.metadataTable);
	case SRAM_CACHE:         return depth(device.sramCache);
	case LOGGER_BLOCK:       return depth(device.loggerBlock);
	}
	return 0u;
}

std::string RouterEditor::getBlockStatus(BlockType bt) const {
	switch (bt) {
	case COMPRESSED_STORAGE: {
		const auto& c = device.compressedStorage.compute;
		if (!c.currentRequest.has_value()) return "IDLE";
		return std::string(c.doingRead ? "READ " : "WRITE ") + std::to_string(c.timeLeft) + " cy";
	}
	case COMPRESSOR: {
		const auto& c = device.compressor.compute;
		unsigned occ = 0;
		for (unsigned i = 0; i < c.PIPELINE_DEPTH; i++) if (c.compressing[i].has_value()) occ++;
		return std::to_string(occ) + "/" + std::to_string(c.PIPELINE_DEPTH) + " pipeline";
	}
	case DECOMPRESSOR: {
		const auto& c = device.decompressor.compute;
		unsigned occ = 0;
		for (unsigned i = 0; i < c.PIPELINE_DEPTH; i++) if (c.decompressing[i].has_value()) occ++;
		return std::to_string(occ) + "/" + std::to_string(c.PIPELINE_DEPTH) + " pipeline";
	}
	case DRAM_DATA_CACHE: {
		const auto& c = device.dramCache.compute;
		if (!c.currentRequest.has_value()) return "IDLE";
		return std::string(c.doingRead ? "READ " : "WRITE ") + std::to_string(c.timeLeft) + " cy";
	}
	case METADATA_TABLE: {
		const auto& c = device.metadataTable.compute;
		if (!c.currentRequest.has_value()) return "IDLE";
		return std::string(c.doingRead ? "READ " : "WRITE ") + std::to_string(c.timeLeft) + " cy";
	}
	case SRAM_CACHE:  return "combinational";
	case LOGGER_BLOCK: return "";
	}
	return "";
}

// ── Layout persistence ────────────────────────────────────────────────────────
static const char* kLayoutFile = "RouterEditorLayout.ini";

void RouterEditor::saveLayoutFile() {
	FILE* f = fopen(kLayoutFile, "w");
	if (!f) return;
	fprintf(f, "simPanelWidth=%f\n", simPanelWidth);
	fprintf(f, "inspectorWidth=%f\n", inspectorWidth);
	for (int a = 0; a < 4; a++) {
		auto it = savedPositions.find(static_cast<ActionType>(a));
		if (it == savedPositions.end()) continue;
		for (const auto& [nid, pos] : it->second)
			fprintf(f, "POS_%d_%d=%f,%f\n", a, nid, pos.x, pos.y);
	}
	fclose(f);
}

void RouterEditor::loadLayoutFile() {
	FILE* f = fopen(kLayoutFile, "r");
	if (!f) return;
	char line[256];
	while (fgets(line, sizeof(line), f)) {
		float v;
		if (sscanf(line, "simPanelWidth=%f",  &v) == 1) { simPanelWidth  = v; continue; }
		if (sscanf(line, "inspectorWidth=%f", &v) == 1) { inspectorWidth = v; continue; }
		int a, nid;
		float x, y;
		if (sscanf(line, "POS_%d_%d=%f,%f", &a, &nid, &x, &y) == 4)
			savedPositions[static_cast<ActionType>(a)][nid] = { x, y };
	}
	fclose(f);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────
RouterEditor::RouterEditor(
	SDL_Window* window,
	Device& device,
	std::function<void(BlockType)> onBlockSelected
) : sdlWindow(window), device(device), onBlockSelected(std::move(onBlockSelected)) {
	nodeEditor::Config cfg;
	// Positions are managed manually per-action-type; disable node-editor's auto-save.
	cfg.SettingsFile    = nullptr;
	cfg.EnableSmoothZoom = true;
	ctx = nodeEditor::CreateEditor(&cfg);
	loadLayoutFile();
}

RouterEditor::~RouterEditor() {
	saveLayoutFile();
	nodeEditor::DestroyEditor(ctx);
}

// ── File-dialog callbacks (may be called on a worker thread by SDL3) ──────────
void SDLCALL RouterEditor::fileDialogLoadCB(void* userdata, const char* const* filelist, int) {
	auto* p = static_cast<PendingPath*>(userdata);
	if (!filelist || !filelist[0]) return;
	std::lock_guard<std::mutex> lk(p->mtx);
	p->path  = filelist[0];
	p->ready = true;
}

void SDLCALL RouterEditor::fileDialogSaveCB(void* userdata, const char* const* filelist, int) {
	auto* p = static_cast<PendingPath*>(userdata);
	if (!filelist || !filelist[0]) return;
	std::lock_guard<std::mutex> lk(p->mtx);
	p->path  = filelist[0];
	p->ready = true;
}

void RouterEditor::flushPendingPaths() {
	{
		std::lock_guard<std::mutex> lk(pendingLoad.mtx);
		if (pendingLoad.ready) {
			if (auto cfg = loadDeviceConfig(pendingLoad.path))
				if (cfg.has_value()) device.setDeviceConfig(std::move(cfg.value()));
			pendingLoad.path.clear();
			pendingLoad.ready = false;
		}
	}
	{
		std::lock_guard<std::mutex> lk(pendingSave.mtx);
		if (pendingSave.ready) {
			saveDeviceConfig(pendingSave.path, device.getDeviceConfig());
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

	const ImVec2 avail    = ImGui::GetContentRegionAvail();
	const float graphWidth = avail.x - inspectorWidth - 8.0f;

	// Graph pane
	ImGui::BeginChild("##graph", { graphWidth, avail.y });
	renderGraph();
	ImGui::EndChild();

	// Resizable splitter between graph and inspector
	ImGui::SameLine(0, 0);
	ImGui::InvisibleButton("##isplit", ImVec2(8.0f, avail.y));
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	if (ImGui::IsItemActive()) {
		// Dragging right shrinks the inspector; dragging left grows it.
		inspectorWidth -= ImGui::GetIO().MouseDelta.x;
		inspectorWidth  = std::clamp(inspectorWidth, 180.0f, avail.x - 200.0f);
		saveLayoutFile();
	}
	{
		const ImVec2 p0  = ImGui::GetItemRectMin();
		const ImVec2 p1  = ImGui::GetItemRectMax();
		const ImU32  col = (ImGui::IsItemHovered() || ImGui::IsItemActive())
			? IM_COL32(180, 180, 180, 130)
			: IM_COL32(110, 110, 110,  70);
		ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, col);
	}

	// Inspector pane
	ImGui::SameLine(0, 0);
	ImGui::BeginChild("##inspector", { inspectorWidth, avail.y }, ImGuiChildFlags_Borders);
	renderInspector();
	ImGui::EndChild();
}

// ── Toolbar ───────────────────────────────────────────────────────────────────
void RouterEditor::renderToolbar() {
	static const SDL_DialogFileFilter kJsonFilter[] = {
		{ "Device config (*.json)", "json" },
		{ "All files", "*" },
	};

	if (ImGui::Button("Load Config"))
		SDL_ShowOpenFileDialog(fileDialogLoadCB, &pendingLoad, sdlWindow, kJsonFilter, 2, nullptr, false);
	ImGui::SameLine();
	if (ImGui::Button("Save Config"))
		SDL_ShowSaveFileDialog(fileDialogSaveCB, &pendingSave, sdlWindow, kJsonFilter, 2, nullptr);

	ImGui::SameLine(0, 16);
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine(0, 16);
	ImGui::TextUnformatted("Action:");
	ImGui::SameLine();

	if (ImGui::BeginTabBar("##actionTabs")) {
		for (ActionType at : { READ, WRITE, CACHE_EVICT, COMPRESS }) {
			const std::string label = actionTypeToString(at) + "##tab" + std::to_string((int)at);
			if (ImGui::BeginTabItem(label.c_str())) {
				if (viewedAction != at) {
					viewedAction         = at;
					selectedLink         = std::nullopt;
					positionsNeedRestore = true;
				}
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	// Legend
	ImGui::SameLine(0, 20);
	ImGui::ColorButton("##cGreen",  { 0.31f, 0.78f, 0.47f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine(); ImGui::TextDisabled("Always");
	ImGui::SameLine(0, 12);
	ImGui::ColorButton("##cOrange", { 0.90f, 0.63f, 0.16f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine(); ImGui::TextDisabled("Conditional");
	ImGui::SameLine(0, 12);
	ImGui::ColorButton("##cRed",    { 0.86f, 0.31f, 0.31f, 1.f }, ImGuiColorEditFlags_NoTooltip, { 12, 12 });
	ImGui::SameLine(); ImGui::TextDisabled("Mutating");
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

	// Restore node positions when switching action tabs (or on the first frame).
	if (positionsNeedRestore) {
		for (int i = 0; i < 6; i++) {
			const BlockType bt  = static_cast<BlockType>(i);
			const int       nid = nodeId(bt);
			auto aIt = savedPositions.find(viewedAction);
			if (aIt != savedPositions.end()) {
				auto pIt = aIt->second.find(nid);
				if (pIt != aIt->second.end()) {
					nodeEditor::SetNodePosition(nid, pIt->second);
					continue;
				}
			}
			nodeEditor::SetNodePosition(nid, nodeDefaultPos(bt));
		}
		positionsNeedRestore = false;
	}

	for (int i = 0; i < 6; i++) drawNode(static_cast<BlockType>(i));

	// Draw links for the current action layer.
	for (const auto& [lid, meta] : linkMetaMap) {
		const auto& vec = device.getDeviceConfig().router.getData().at(meta.srcBlock).at(meta.srcAction);
		const RouterOption& opt = vec[meta.optionIndex];
		// Clamp port to the valid range of the destination block.
		const int nPorts   = getInputPortCount(opt.output.first);
		const int safePort = (nPorts > 0) ? std::min(opt.output.second, nPorts - 1) : 0;
		nodeEditor::Link(
			lid,
			outputPinId(meta.srcBlock, 0),
			inputPinId(opt.output.first, safePort),
			ImGui::ColorConvertU32ToFloat4(linkColor(opt)),
			2.5f
		);
	}

	if (selectedLink.has_value()) nodeEditor::SelectLink(*selectedLink, false);

	handleCreation();
	handleDeletion();
	handleNodeSelection();

	// Persist current node positions for the active action type.
	for (int i = 0; i < 6; i++) {
		const BlockType bt  = static_cast<BlockType>(i);
		const int       nid = nodeId(bt);
		savedPositions[viewedAction][nid] = nodeEditor::GetNodePosition(nid);
	}

	nodeEditor::End();

	// Harvest link selection (must happen after End()).
	{
		std::vector<nodeEditor::LinkId> sel(nodeEditor::GetSelectedObjectCount());
		int n = nodeEditor::GetSelectedLinks(sel.data(), (int)sel.size());
		if (n == 1)      selectedLink = sel[0];
		else if (n == 0) selectedLink = std::nullopt;
	}

	nodeEditor::SetCurrentEditor(nullptr);
}

void RouterEditor::drawNode(BlockType bt) {
	const auto portNames = getInputPortNames(bt);
	const int  nPorts    = (int)portNames.size();

	nodeEditor::BeginNode(nodeId(bt));

	// Title
	ImGui::TextUnformatted(blockTypeToString(bt).c_str());
	// ImGui::Spacing();

	// Left group: one input pin per port, labelled with name and live queue depth.
	ImGui::BeginGroup();
	for (int port = 0; port < nPorts; port++) {
		nodeEditor::BeginPin(inputPinId(bt, port), nodeEditor::PinKind::Input);
		const unsigned int depth = getQueueDepth(bt, port);
		ImGui::Text("> %-7s [%u]", portNames[port].c_str(), depth);
		nodeEditor::EndPin();
	}
	ImGui::EndGroup();

	ImGui::SameLine(0, 36);

	// Right group: single output pin.
	ImGui::BeginGroup();
	nodeEditor::BeginPin(outputPinId(bt, 0), nodeEditor::PinKind::Output);
	ImGui::TextUnformatted("out >");
	nodeEditor::EndPin();
	ImGui::EndGroup();

	// Status line below the pin layout.
	ImGui::Spacing();
	const std::string status = getBlockStatus(bt);
	if (!status.empty()) {
		if (status == "IDLE" || status == "combinational")
			ImGui::TextDisabled("%s", status.c_str());
		else
			ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, "%s", status.c_str());
	}

	nodeEditor::EndNode();
}

// ── Node selection -> block inspector ────────────────────────────────────────
void RouterEditor::handleNodeSelection() {
	std::vector<nodeEditor::NodeId> sel(nodeEditor::GetSelectedObjectCount());
	int n = nodeEditor::GetSelectedNodes(sel.data(), (int)sel.size());

	if (n == 1) {
		const int nid = (int)sel[0].Get();
		if (nid >= 1 && nid <= 6) {
			const BlockType bt = static_cast<BlockType>(nid - 1);
			if (!selectedNode.has_value() || *selectedNode != sel[0]) {
				selectedNode = sel[0];
				if (onBlockSelected) onBlockSelected(bt);
			}
		}
	} else if (n == 0) {
		selectedNode = std::nullopt;
	}
}

// ── Link creation ─────────────────────────────────────────────────────────────
void RouterEditor::handleCreation() {
	if (!nodeEditor::BeginCreate(ImColor(255, 255, 255), 2.0f)) {
		nodeEditor::EndCreate();
		return;
	}

	nodeEditor::PinId startPin, endPin;
	if (nodeEditor::QueryNewLink(&startPin, &endPin) && startPin && endPin) {
		const int startId      = (int)startPin.Get();
		const int endId        = (int)endPin.Get();
		const bool startIsOut  = startId >= 2000;
		const bool endIsIn     = endId >= 1000 && endId < 2000;

		if (!startIsOut || !endIsIn) {
			nodeEditor::RejectNewItem(ImColor(255, 60, 60), 2.0f);
		} else {
			const BlockType srcBlock = static_cast<BlockType>((startId - 2000) / 10);
			const BlockType dstBlock = static_cast<BlockType>((endId   - 1000) / 10);
			const int       dstPort  = (endId - 1000) % 10;

			if (dstPort >= getInputPortCount(dstBlock)) {
				nodeEditor::RejectNewItem(ImColor(255, 60, 60), 2.0f);
			} else if (nodeEditor::AcceptNewItem(ImColor(80, 200, 120), 2.0f)) {
				RouterOption opt;
				opt.output = { dstBlock, dstPort };
				device.getDeviceConfig().router.routerData[srcBlock][viewedAction].push_back(opt);
				rebuildLinkMeta();
				int newIdx   = (int)device.getDeviceConfig().router.routerData[srcBlock][viewedAction].size() - 1;
				selectedLink = linkId(srcBlock, viewedAction, newIdx);
			}
		}
	}
	nodeEditor::EndCreate();
}

// ── Link deletion ─────────────────────────────────────────────────────────────
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
		ImGui::TextDisabled("Click a node to select a block.");
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

	RouterOption& opt           = vec[idx];
	const auto    dstPortNames  = getInputPortNames(opt.output.first);
	const int     safePort      = std::min(opt.output.second, (int)dstPortNames.size() - 1);
	const char*   portLabel     = (safePort >= 0 && safePort < (int)dstPortNames.size())
		? dstPortNames[safePort].c_str() : "?";

	ImGui::Text("%s -> %s [%s]",
		blockTypeToString(srcBlock).c_str(),
		blockTypeToString(opt.output.first).c_str(),
		portLabel);
	ImGui::TextDisabled("Action: %s", actionTypeToString(action).c_str());
	ImGui::Separator();

	// -- Destination block combo --
	ImGui::TextUnformatted("Destination");
	static const char* kBlockNames[] = {
		"COMPRESSED_STORAGE", "COMPRESSOR", "DECOMPRESSOR",
		"DRAM_DATA_CACHE",    "METADATA_TABLE", "SRAM_CACHE"
	};
	static_assert(sizeof(kBlockNames) / sizeof(kBlockNames[0]) == 6,
		"kBlockNames must match visible BlockType count");

	int dstIdx = (int)opt.output.first;
	ImGui::SetNextItemWidth(180);
	if (ImGui::Combo("Block##dst", &dstIdx, kBlockNames, 6)) {
		opt.output.first  = static_cast<BlockType>(dstIdx);
		opt.output.second = std::min(opt.output.second,
			getInputPortCount(opt.output.first) - 1);
	}

	// -- Destination port combo (uses named ports of the destination block) --
	ImGui::SameLine(0, 8);
	{
		const auto updatedDstNames = getInputPortNames(opt.output.first);
		std::vector<const char*> cstrs;
		for (const auto& s : updatedDstNames) cstrs.push_back(s.c_str());
		int port = std::min(opt.output.second, (int)cstrs.size() - 1);
		ImGui::SetNextItemWidth(90);
		if (ImGui::Combo("Port##dst", &port, cstrs.data(), (int)cstrs.size()))
			opt.output.second = port;
	}

	ImGui::Separator();
	renderConditionEditor(opt.condition, srcBlock);
	ImGui::Separator();
	renderMutatorEditor(opt.mutator);
}

// ── Condition editor ──────────────────────────────────────────────────────────
void RouterEditor::renderConditionEditor(Condition& cond, BlockType srcBlock) {
	ImGui::TextUnformatted("Conditions");
	ImGui::SameLine();

	const auto argNames  = getRouteArgNames(srcBlock);
	const std::string defaultKey = argNames.empty() ? "arg" : argNames[0];
	if (ImGui::SmallButton("+ Add")) cond.conditions.emplace_back(defaultKey, EQUAL, 0);

	if (cond.conditions.empty()) {
		ImGui::TextDisabled("  (none - always fires)");
		return;
	}

	static const char* kCompareOps[] = { "<", ">", "<=", ">=", "==", "!=" };
	const float rowHeight   = ImGui::GetFrameHeightWithSpacing();
	const float childHeight = std::min<float>(rowHeight * (float)cond.conditions.size() + 12.f, 160.f);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##conds", { 0, childHeight }, ImGuiChildFlags_Borders);

	int eraseAt = -1;
	for (int i = 0; i < (int)cond.conditions.size(); i++) {
		auto& [key, compare, value] = cond.conditions[i];
		ImGui::PushID(i);

		// Key: combo when arg names are known for this source block, otherwise free text.
		if (!argNames.empty()) {
			std::vector<const char*> argCStrs;
			for (const auto& s : argNames) argCStrs.push_back(s.c_str());
			int argIdx = 0;
			for (int j = 0; j < (int)argNames.size(); j++)
				if (argNames[j] == key) { argIdx = j; break; }
			ImGui::SetNextItemWidth(90);
			if (ImGui::Combo("##k", &argIdx, argCStrs.data(), (int)argCStrs.size()))
				key = argNames[argIdx];
		} else {
			char buf[64];
			strncpy(buf, key.c_str(), sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = '\0';
			ImGui::SetNextItemWidth(90);
			if (ImGui::InputText("##k", buf, sizeof(buf))) key = buf;
		}

		ImGui::SameLine(0, 4);
		int cmpI = (int)compare;
		ImGui::SetNextItemWidth(48);
		if (ImGui::Combo("##c", &cmpI, kCompareOps, 6)) compare = static_cast<Compare>(cmpI);

		ImGui::SameLine(0, 4);
		ImGui::SetNextItemWidth(64);
		ImGui::InputInt("##v", &value, 0);

		ImGui::SameLine(0, 4);
		if (ImGui::SmallButton("X")) eraseAt = i;

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
	if (ImGui::Checkbox("Override ActionType##mut", &hasOverride))
		mut.setActionType = hasOverride ? std::optional<ActionType>(READ) : std::nullopt;

	if (hasOverride) {
		int idx = (int)mut.setActionType.value();
		ImGui::SetNextItemWidth(140);
		if (ImGui::Combo("##mutAction", &idx, kActionNames, 4))
			mut.setActionType = static_cast<ActionType>(idx);
	}
}

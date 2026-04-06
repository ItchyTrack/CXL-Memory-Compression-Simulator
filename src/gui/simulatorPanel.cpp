#include "simulatorPanel.h"
#include "../external/imgui.h"
#include "../external/imgui_internal.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>

// ── static data ───────────────────────────────────────────────────────────────
const char* SimulatorPanel::actionNames[] = { "READ", "WRITE", "CACHE_EVICT", "COMPRESS" };
int SimulatorPanel::actionCount = 4;

// ── ctor ──────────────────────────────────────────────────────────────────────
SimulatorPanel::SimulatorPanel(Device& device) : device(device) { lastTime = (double)SDL_GetTicks(); }

// ── public API ────────────────────────────────────────────────────────────────
void SimulatorPanel::selectBlock(BlockType bt) { selectedBlock = bt; }

bool SimulatorPanel::shouldTick() {
	if (!playing) return false;

	double now = (double)SDL_GetTicks();
	double delta = (now - lastTime) / 1000.0; // seconds
	lastTime = now;
	accumulator += delta * (double)ticksPerSecond;

	if (accumulator >= 1.0) {
		accumulator -= 1.0;
		return true;
	}
	return false;
}

// ── top-level render ──────────────────────────────────────────────────────────
void SimulatorPanel::render() {
	renderControls();
	ImGui::Separator();
	renderBlockInspector();
}

// ── simulation controls bar ───────────────────────────────────────────────────
void SimulatorPanel::renderControls() {
	// Play / Pause button
	if (playing) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.f));
		if (ImGui::Button("  ⏸  Pause  ")) playing = false;
		ImGui::PopStyleColor(3);
	} else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.55f, 0.2f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.75f, 0.3f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.4f, 0.1f, 1.f));
		if (ImGui::Button("  ▶  Play   ")) {
			playing = true;
			lastTime = (double)SDL_GetTicks();
		}
		ImGui::PopStyleColor(3);
	}

	ImGui::SameLine(0, 8);

	// Step button (single tick, always available)
	ImGui::BeginDisabled(playing);
	if (ImGui::Button("Step")) {
		device.update();
		totalTicks++;
	}
	ImGui::EndDisabled();

	ImGui::SameLine(0, 8);

	// Reset button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.15f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.20f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.05f, 1.f));
	if (ImGui::Button("Reset")) {
		playing = false;
		totalTicks = 0;
		accumulator = 0.0;
		// Rebuild the device in-place by calling default ctor would need
		// engine support; for now we just stop – caller can restart.
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine(0, 24);
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine(0, 16);

	// Speed slider
	ImGui::SetNextItemWidth(140);
	ImGui::SliderInt("Ticks / sec", &ticksPerSecond, 1, 120);

	ImGui::SameLine(0, 24);
	ImGui::TextDisabled("Tick #%llu", (unsigned long long)totalTicks);

	// advance one tick if playing
	if (shouldTick()) {
		device.update();
		totalTicks++;
	}
}

// ── block inspector panel ─────────────────────────────────────────────────────
void SimulatorPanel::renderBlockInspector() {
	if (!selectedBlock.has_value()) {
		ImGui::TextDisabled("Click a block node in the graph to inspect it.");
		return;
	}

	const BlockType bt = *selectedBlock;
	ImGui::Text("Block: %s", blockTypeToString(bt).c_str());
	ImGui::Spacing();

	switch (bt) {
	case COMPRESSED_STORAGE: renderCompressedStorageInfo(); break;
	case COMPRESSOR: renderCompressorInfo(); break;
	case DECOMPRESSOR: renderDecompressorInfo(); break;
	case DRAM_DATA_CACHE: renderDramDataCacheInfo(); break;
	case METADATA_TABLE: renderMetadataTableInfo(); break;
	case SRAM_CACHE: renderSramCacheInfo(); break;
	case LOGGER_BLOCK: printf("error?"); break;
	}

	ImGui::Spacing();
	ImGui::Separator();
	renderInjectForm();
}

// ── per-block info renderers ──────────────────────────────────────────────────

// Helper: renders a 2-column table showing queue sizes for each input port.
// Works on any block whose blockInput is publicly accessible via getInputInterface().
static void renderInputQueues(const char* label0, unsigned int size0, const char* label1 = nullptr, unsigned int size1 = 0, bool twoInputs = true) {
	if (ImGui::BeginTable("##queues", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("Queue");
		ImGui::TableSetupColumn("Depth");
		ImGui::TableHeadersRow();

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(label0 ? label0 : "in[0]");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%u", size0);

		if (twoInputs) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(label1 ? label1 : "in[1]");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", size1);
		}
		ImGui::EndTable();
	}
}

void SimulatorPanel::renderCompressedStorageInfo() {
	auto& cs = device.compressedStorage;
	auto& bi = cs.getIputInterface();

	ImGui::TextUnformatted("CompressedStorage  (READ: 10 cy, WRITE: 20 cy)");
	ImGui::Spacing();
	renderInputQueues("Reads  in[0]", (unsigned)bi.inputBuffers[0].size(), "Writes in[1]", (unsigned)bi.inputBuffers[1].size());

	// Compute state – access via the friend-accessible compute member
	auto& c = cs.compute;
	ImGui::Spacing();
	if (c.currentRequest.has_value()) {
		ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, c.doingRead ? "READING: %u cycle(s) left" : "WRITING: %u cycle(s) left", c.timeLeft);
	} else {
		ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, "IDLE");
	}
}

void SimulatorPanel::renderCompressorInfo() {
	auto& comp = device.compressor;
	auto& bi = comp.getIputInterface();
	auto& c = comp.compute;

	ImGui::TextUnformatted("Compressor  (pipeline depth: 10)");
	ImGui::Spacing();
	renderInputQueues("in[0]", (unsigned)bi.inputBuffers[0].size(), nullptr, 0, false);

	ImGui::Spacing();
	ImGui::TextUnformatted("Pipeline slots:");
	// Show pipeline as a row of coloured boxes
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 2 });
	for (unsigned int i = 0; i < c.PIPELINE_DEPTH; i++) {
		const auto& slot = c.compressing[(c.end + i + 1) % c.PIPELINE_DEPTH];
		ImVec4 col = slot.has_value() ? ImVec4(0.2f, 0.8f, 0.4f, 1.f) : ImVec4(0.2f, 0.2f, 0.2f, 0.6f);
		ImGui::ColorButton(("##ps" + std::to_string(i)).c_str(), col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, { 18, 18 });
		if (i + 1 < c.PIPELINE_DEPTH) ImGui::SameLine();
	}
	ImGui::PopStyleVar();
	unsigned int occupied = 0;
	for (unsigned int i = 0; i < c.PIPELINE_DEPTH; i++)
		if (c.compressing[i].has_value()) occupied++;
	ImGui::SameLine();
	ImGui::TextDisabled(" %u / %u", occupied, c.PIPELINE_DEPTH);
}

void SimulatorPanel::renderDecompressorInfo() {
	auto& decomp = device.decompressor;
	auto& bi = decomp.getIputInterface();
	auto& c = decomp.compute;

	ImGui::TextUnformatted("Decompressor  (pipeline depth: 10)");
	ImGui::Spacing();
	renderInputQueues("in[0]", (unsigned)bi.inputBuffers[0].size(), nullptr, 0, false);

	ImGui::Spacing();
	ImGui::TextUnformatted("Pipeline slots:");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 2 });
	for (unsigned int i = 0; i < c.PIPELINE_DEPTH; i++) {
		const auto& slot = c.decompressing[(c.end + i + 1) % c.PIPELINE_DEPTH];
		ImVec4 col = slot.has_value() ? ImVec4(0.3f, 0.6f, 1.0f, 1.f) : ImVec4(0.2f, 0.2f, 0.2f, 0.6f);
		ImGui::ColorButton(("##dp" + std::to_string(i)).c_str(), col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, { 18, 18 });
		if (i + 1 < c.PIPELINE_DEPTH) ImGui::SameLine();
	}
	ImGui::PopStyleVar();
	unsigned int occupied = 0;
	for (unsigned int i = 0; i < c.PIPELINE_DEPTH; i++)
		if (c.decompressing[i].has_value()) occupied++;
	ImGui::SameLine();
	ImGui::TextDisabled(" %u / %u", occupied, c.PIPELINE_DEPTH);
}

void SimulatorPanel::renderDramDataCacheInfo() {
	auto& dram = device.dramCache;
	auto& bi = dram.getIputInterface();
	auto& c = dram.compute;

	ImGui::TextUnformatted("DramDataCache  (READ: 10 cy, WRITE: 20 cy)");
	ImGui::Spacing();
	renderInputQueues("Reads  in[0]", (unsigned)bi.inputBuffers[0].size(), "Writes in[1]", (unsigned)bi.inputBuffers[1].size());

	ImGui::Spacing();
	if (c.currentRequest.has_value()) {
		ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, c.doingRead ? "READING: %u cycle(s) left" : "WRITING: %u cycle(s) left", c.timeLeft);
	} else {
		ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, "IDLE");
	}
}

void SimulatorPanel::renderMetadataTableInfo() {
	auto& mt = device.metadataTable;
	auto& bi = mt.getIputInterface();
	auto& c = mt.compute;

	ImGui::TextUnformatted("MetadataTable  (READ: 10 cy, WRITE: 20 cy)");
	ImGui::TextDisabled("Routes with random DRC_valid / CSA_valid metadata.");
	ImGui::Spacing();
	renderInputQueues("Reads  in[0]", (unsigned)bi.inputBuffers[0].size(), "Writes in[1]", (unsigned)bi.inputBuffers[1].size());

	ImGui::Spacing();
	if (c.currentRequest.has_value()) {
		ImGui::TextColored({ 1.f, 0.8f, 0.2f, 1.f }, c.doingRead ? "READING  — %u cycle(s) left" : "WRITING  — %u cycle(s) left", c.timeLeft);
	} else {
		ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, "IDLE");
	}
}

void SimulatorPanel::renderSramCacheInfo() {
	auto& sram = device.sramCache;
	auto& bi = sram.getIputInterface();

	ImGui::TextUnformatted("SramCache  (single-cycle, combinational)");
	ImGui::TextDisabled("Routes immediately with random Found flag.");
	ImGui::Spacing();
	renderInputQueues("Reads  in[0]", (unsigned)bi.inputBuffers[0].size(), "Writes in[1]", (unsigned)bi.inputBuffers[1].size());
}

// ── inject request form ───────────────────────────────────────────────────────
void SimulatorPanel::renderInjectForm() {
	if (!selectedBlock.has_value()) return;

	ImGui::TextUnformatted("Inject Request");
	ImGui::Spacing();

	// Port selector — all blocks except Compressor have 2 ports
	const BlockType bt = *selectedBlock;
	const int maxPorts = (bt == COMPRESSOR) ? 1 : 2;
	if (injectPort >= maxPorts) injectPort = 0;

	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("Input Port", &injectPort);
	injectPort = std::clamp(injectPort, 0, maxPorts - 1);

	ImGui::SetNextItemWidth(140);
	ImGui::Combo("Action Type", &injectActionIdx, actionNames, actionCount);

	if (ImGui::Button("  ➕  Inject  ")) {
		ActionType at = static_cast<ActionType>(injectActionIdx);
		Request req(at);

		auto push = [&](auto& block) { block.getIputInterface().pushRequest((unsigned)injectPort, req); };

		switch (bt) {
		case COMPRESSED_STORAGE: push(device.compressedStorage); break;
		case COMPRESSOR: push(device.compressor); break;
		case DECOMPRESSOR: push(device.decompressor); break;
		case DRAM_DATA_CACHE: push(device.dramCache); break;
		case METADATA_TABLE: push(device.metadataTable); break;
		case SRAM_CACHE: push(device.sramCache); break;
		case LOGGER_BLOCK: printf("error?"); break;
		}
	}
}

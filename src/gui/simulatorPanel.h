#ifndef simulatorPanel_h
#define simulatorPanel_h

#include "../simulator/device.h"
#include "../simulator/router.h"
#include <optional>

// Wraps all simulation-control UI:
//   • Play / Pause / Step / Tick-rate controls
//   • Block inspector (click a block node → show live state)
//   • "Inject Request" panel (add requests to any block's input queue)
class SimulatorPanel {
public:
	explicit SimulatorPanel(Device& device);

	// Call once per ImGui frame. Draws the toolbar strip (play/pause/step)
	// and, below it, the inspector panel for whichever block is selected.
	void render();

	// The router-editor graph calls this when the user clicks a node so the
	// inspector can update without the panel needing to know about the editor.
	void selectBlock(BlockType bt);

	// Returns true when the simulation should advance one tick this frame.
	// The caller (main loop) owns the actual device.update() call.
	bool shouldTick();

private:
	Device& device;

	// ── simulation state ──────────────────────────────────────────────────
	bool playing = false;
	int ticksPerSecond = 10;  // 1-120
	double accumulator = 0.0; // fractional ticks accumulated
	double lastTime = 0.0;	  // SDL_GetTicks64 snapshot (ms)
	uint64_t totalTicks = 0;

	// ── selection ─────────────────────────────────────────────────────────
	std::optional<BlockType> selectedBlock;

	// ── inject-request form state ─────────────────────────────────────────
	int injectPort = 0;
	int injectActionIdx = 0; // index into ActionType enum

	// ── helpers ───────────────────────────────────────────────────────────
	void renderControls();
	void renderBlockInspector();
	void renderInjectForm();

	// Per-block info renderers (mirror the debugPrint() logic as ImGui)
	void renderCompressedStorageInfo();
	void renderCompressorInfo();
	void renderDecompressorInfo();
	void renderDramDataCacheInfo();
	void renderMetadataTableInfo();
	void renderSramCacheInfo();

	// Generic queue-size table (works for any block with a public blockInput)
	template <unsigned int N, class Compute, string_litteral NAME>
	void renderQueueTable(const Block<N, Compute, NAME>& block);

	static const char* actionNames[];
	static int actionCount;
};

#endif /* simulatorPanel_h */

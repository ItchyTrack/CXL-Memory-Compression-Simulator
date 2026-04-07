#ifndef routerEditor_h
#define routerEditor_h

#include "../external/nodes/imgui_node_editor.h"
#include "../simulator/router.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

struct SDL_Window;
namespace nodeEditor = ax::NodeEditor;
class Device;

struct LinkMeta {
	BlockType srcBlock;
	ActionType srcAction;
	int optionIndex;
};

// Pure node-graph editor for the RouterData routing table.
// Simulation controls live in SimulatorPanel.
class RouterEditor {
public:
	// Public so main.cpp can read/write it for the resizable main splitter.
	float simPanelWidth = 380.0f;

	// onBlockSelected(bt) -- called when the user clicks a block node;
	//                        use this to drive SimulatorPanel::selectBlock().
	explicit RouterEditor(
		SDL_Window* window,
		Device& device,
		std::function<void(BlockType)> onBlockSelected = {}
	);
	~RouterEditor();

	// Renders the full router editor (toolbar + graph + link inspector).
	void render();

	// Layout persistence: per-action node positions + panel widths.
	void saveLayoutFile();
	void loadLayoutFile();

private:
	nodeEditor::EditorContext* ctx = nullptr;
	SDL_Window* sdlWindow = nullptr;
	ActionType viewedAction = READ;
	std::optional<nodeEditor::LinkId> selectedLink = std::nullopt;
	std::optional<nodeEditor::NodeId> selectedNode = std::nullopt;
	Device& device;

	float inspectorWidth = 340.0f;

	std::function<void(BlockType)> onBlockSelected;

	// Pending file-dialog paths (may arrive on a worker thread).
	struct PendingPath {
		std::mutex mtx;
		std::string path;
		bool ready = false;
	};
	PendingPath pendingLoad;
	PendingPath pendingSave;

	// linkId (int) -> metadata
	std::unordered_map<int, LinkMeta> linkMetaMap;

	// Per-action node positions: action -> (nodeId -> ImVec2).
	std::map<ActionType, std::map<int, ImVec2>> savedPositions;
	// True on first frame and whenever the viewed action tab changes.
	bool positionsNeedRestore = true;

	void renderToolbar();
	void renderGraph();
	void renderInspector();

	// srcBlock is passed so the condition editor can offer a combo of known arg names.
	void renderConditionEditor(Condition& cond, BlockType srcBlock);
	void renderMutatorEditor(RequestMutator& mut);

	void drawNode(BlockType bt);
	void handleCreation();
	void handleDeletion();
	void handleNodeSelection();

	void rebuildLinkMeta();
	void flushPendingPaths();

	// Live state helpers -- require friend access granted via Block<> in block.h.
	unsigned int getQueueDepth(BlockType bt, int port) const;
	std::string  getBlockStatus(BlockType bt) const;

	static void fileDialogLoadCB(void* userdata, const char* const* filelist, int filter);
	static void fileDialogSaveCB(void* userdata, const char* const* filelist, int filter);

	// ID scheme (non-overlapping, non-zero):
	//   Node  IDs : 1 - 6
	//   Input pins: 1000 + bt*10 + port
	//   Output pin: 2000 + bt*10
	//   Link  IDs : encoded by linkId()
	static int nodeId(BlockType bt)                   { return (int)bt + 1; }
	static int inputPinId(BlockType bt, int port)     { return 1000 + (int)bt * 10 + port; }
	static int outputPinId(BlockType bt, int /*port*/){ return 2000 + (int)bt * 10; }
	static int linkId(BlockType src, ActionType action, int idx);

	static ImVec2 nodeDefaultPos(BlockType bt);
	static ImU32  linkColor(const RouterOption& opt);
};

#endif /* routerEditor_h */

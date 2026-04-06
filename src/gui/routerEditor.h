#ifndef routerEditor_h
#define routerEditor_h

#include "../simulator/router.h"
#include "../external/nodes/imgui_node_editor.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

struct SDL_Window;

namespace nodeEditor = ax::NodeEditor;

struct LinkMeta {
    BlockType  srcBlock;
    ActionType srcAction;
    int        optionIndex;
};

class RouterEditor {
public:
    // Pass the SDL window so file dialogs can be parented to it.
    // onLoad(path) / onSave(path) are called on the render thread once the
    // user confirms a path; the caller is responsible for the actual I/O.
    explicit RouterEditor(SDL_Window* window,
                          std::function<void(const std::string&)> onLoad = {},
                          std::function<void(const std::string&)> onSave = {});
    ~RouterEditor();

    void render(RouterData& routerData);

private:
    nodeEditor::EditorContext* ctx          = nullptr;
    SDL_Window*        sdlWindow    = nullptr;
    ActionType         viewedAction = READ;
    std::optional<nodeEditor::LinkId> selectedLink = std::nullopt;
    bool               positionsSet = false;

    std::function<void(const std::string&)> onLoad;
    std::function<void(const std::string&)> onSave;

    // Pending paths written by the SDL file-dialog callbacks (may arrive on a
    // worker thread) and consumed on the next render() call.
    struct PendingPath {
        std::mutex  mtx;
        std::string path;
        bool        ready = false;
    };
    PendingPath pendingLoad;
    PendingPath pendingSave;

    // linkId (int) → metadata
    std::unordered_map<int, LinkMeta> linkMetaMap;

    void renderToolbar (RouterData& routerData);
    void renderGraph   (RouterData& routerData);
    void renderInspector(RouterData& routerData);

    void renderConditionEditor(Condition& cond);
    void renderMutatorEditor  (RequestMutator& mut);

    void drawNode      (BlockType bt);
    void handleCreation(RouterData& routerData);
    void handleDeletion(RouterData& routerData);

    void rebuildLinkMeta(const RouterData& routerData);

    // Drain any path that arrived from a file-dialog callback and invoke the
    // corresponding handler.
    void flushPendingPaths();

    // SDL file-dialog callback shims — userdata is PendingPath*.
    static void fileDialogLoadCB(void* userdata, const char* const* filelist, int filter);
    static void fileDialogSaveCB(void* userdata, const char* const* filelist, int filter);

    // ID scheme (non-overlapping, non-zero):
    //   Node  IDs :  1 – 7
    //   Input pins : 1000 + bt*10 + port
    //   Output pins: 2000 + bt*10 + port
    //   Link  IDs  : encoded by linkId()
    static int    nodeId     (BlockType bt)           { return (int)bt + 1; }
    static int    inputPinId (BlockType bt, int port) { return 1000 + (int)bt * 10 + port; }
    static int    outputPinId(BlockType bt, int port) { return 2000 + (int)bt * 10 + port; }
    static int    linkId     (BlockType src, ActionType action, int idx);

    static ImVec2 nodeLayout(BlockType bt);
    static ImU32  linkColor (const RouterOption& opt);
};

#endif /* routerEditor_h */
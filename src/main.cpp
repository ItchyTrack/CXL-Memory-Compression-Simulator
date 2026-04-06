#include "external/imgui.h"
#include "external/imgui_impl_sdl3.h"
#include "external/imgui_impl_sdlrenderer3.h"
#include "gui/routerEditor.h"
#include "gui/simulatorPanel.h"
#include "serialization/deviceConfigSerialization.h"
#include "simulator/device.h"
#include <SDL3/SDL.h>
#include <stdio.h>

void run(SDL_Window* window, SDL_Renderer* renderer);

int main(int, char**) {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return 1;
	}

	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	SDL_Window* window = SDL_CreateWindow("CXL Memory Compression Simulator", (int)(1440 * main_scale), (int)(900 * main_scale), window_flags);
	if (!window) {
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderVSync(renderer, 1);
	if (!renderer) {
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	run(window, renderer);

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

void run(SDL_Window* window, SDL_Renderer* renderer) {
	bool done = false;
	ImVec4 clear_color = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);

	// ── Device ────────────────────────────────────────────────────────────
	Device device;
	if (auto cfg = loadDeviceConfig(
			"/Users/ben/Documents/CXL_research/"
			"CXL-Memory-Compression-Simulator/deviceConfigurations/default.json"
		)) {
		if (cfg.has_value()) device.setDeviceConfig(std::move(cfg.value()));
		else printf("Failed to load device config\n");
	}

	// ── GUI panels ────────────────────────────────────────────────────────
	SimulatorPanel simPanel(device);

	RouterEditor routerEditor(
		window,
		device,
		/*onBlockSelected=*/[&simPanel](BlockType bt) { simPanel.selectBlock(bt); }
	);

	// ── Main loop ─────────────────────────────────────────────────────────
	while (!done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) done = true;
		}

		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
			SDL_Delay(10);
			continue;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io2 = ImGui::GetIO();
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize(io2.DisplaySize);
		ImGui::Begin(
			"##ROOT",
			nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus
		);

		// ── Split layout: Simulator panel (left) | Router editor (right) ──
		const float kSimWidth = 380.0f;
		const ImVec2 avail = ImGui::GetContentRegionAvail();

		// ── Left pane: simulation controls + block inspector ──────────────
		ImGui::BeginChild("##simPane", { kSimWidth, avail.y }, ImGuiChildFlags_Borders);
		ImGui::TextUnformatted("Simulator");
		ImGui::Separator();
		simPanel.render();
		ImGui::EndChild();

		ImGui::SameLine(0, 4);

		// ── Right pane: router editor ─────────────────────────────────────
		ImGui::BeginChild("##routerPane", { avail.x - kSimWidth - 4.0f, avail.y }, ImGuiChildFlags_Borders);
		ImGui::TextUnformatted("Router Editor");
		ImGui::Separator();
		routerEditor.render();
		ImGui::EndChild();

		ImGui::End();

		ImGui::Render();
		SDL_SetRenderScale(renderer, io2.DisplayFramebufferScale.x, io2.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}
}

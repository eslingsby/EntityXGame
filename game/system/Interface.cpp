#include "system\Interface.hpp"

#include "component\Transform.hpp"
#include "component\Model.hpp"
#include "component\Collider.hpp"

#include <imgui.h>
#include <examples\imgui_impl_opengl3.h>
#include <examples\imgui_impl_sdl.h>

#include <SDL_video.h>

Interface::Interface() { 
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsLight();

	ImGui_ImplSDL2_InitForOpenGL(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL3_Init("#version 460 core");
}

Interface::~Interface() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void Interface::configure(entityx::EventManager & events) {	
	events.subscribe<WindowOpenEvent>(*this);
	events.subscribe<FramebufferSizeEvent>(*this);
}

void Interface::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt) {
	if (!_running)
		return;
	
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(SDL_GL_GetCurrentWindow());
	ImGui::NewFrame();
	
	if (!_input) {
		for (uint8_t i = 0; i < 5; i++)
			ImGui::GetIO().MouseClicked[i] = false;
		
		ImGui::GetIO().MousePos = { -1, -1 };
	}
	
	bool showFramerate = true;
	
	if (showFramerate) {
		ImGui::Begin("Framerate", &showFramerate, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
	
		ImGui::SetWindowPos({ 0, 0 });
		ImGui::SetWindowSize({ 90, 10 });
	
		ImGui::Text(std::to_string(1.0 / dt).c_str());
	
		ImGui::End();
	}
	
	bool showWindow = _focusedEntity.valid();
	
	if (showWindow) {
		ImGui::Begin("Entity", &showWindow);
	
		if (_focusedEntity.has_component<Transform>()) {
			auto transform = _focusedEntity.component<Transform>();
			
			ImGui::Text("Transform");
	
			ImGui::InputFloat3("Position", &transform->position[0]);
	
			glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform->rotation));
			ImGui::InputFloat3("Rotation", &eulerAngles[0]);
			transform->rotation = glm::quat(glm::radians(eulerAngles));
	
			ImGui::InputFloat3("Scale", &transform->scale[0]);
		}
	
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Interface::receive(const WindowOpenEvent& windowOpenEvent){
	_running = windowOpenEvent.opened;
}

void Interface::receive(const FramebufferSizeEvent& frameBufferSizeEvent){
	_windowSize = frameBufferSizeEvent.size;
}

bool Interface::isHovering() const{
	if (!_running || !_input)
		return false;
	
	return ImGui::IsAnyItemHovered() || ImGui::IsAnyWindowHovered();
}

void Interface::setInputEnabled(bool enabled){
	_input = enabled;
}

void Interface::setFocusedEntity(entityx::Entity entity){
	_focusedEntity = entity;
}

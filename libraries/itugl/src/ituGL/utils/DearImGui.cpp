#include <ituGL/utils/DearImGui.h>

#include <ituGL/application/Window.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <vector>

int R, G, B;



DearImGui::DearImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
}

DearImGui::~DearImGui()
{
    ImGui::DestroyContext();
}

void DearImGui::Initialize(::Window& window)
{
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window.GetInternalWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 410 core");
}

void DearImGui::Cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

void DearImGui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

std::string DearImGui::Portal(std::vector<glm::vec3> ColPalette, glm::vec3& col) {
    ImGui::Text("Choose a portal:");

    if (ImGui::Button("Forest"))
    {
        portal_type = "forest";
        std::cout << "Forest chosen" << std::endl;

        col = ColPalette[1];
       return portal_type;
    }
     
    if (ImGui::Button("Bright Neon"))
    {
        portal_type = "brightneon";
        std::cout << "Bright Neon chosen" << std::endl;
        col = ColPalette[2];
       return portal_type;
    }

    if (ImGui::Button("Italy"))
    {
        portal_type = "italy";
        std::cout << "Italy chosen" << std::endl;
        col = ColPalette[3];
        return portal_type;
    }

    if (ImGui::Button("Baths"))
    {
        portal_type = "baths";
        std::cout << "Baths chosen" << std::endl;
        col = ColPalette[4];
        return portal_type;
    }
  
   return portal_type;
}

std::string DearImGui::MainBG() {
    ImGui::Text("Choose a main Background:");

    if (ImGui::Button("Room"))
    {
        bg_type = "room";
        std::cout << "Room chosen" << std::endl;
        return bg_type;
    }

    if (ImGui::Button("Neon"))
    {
        bg_type = "neonhall";
        std::cout << "Neon hall chosen" << std::endl;
        return bg_type;
    }

    if (ImGui::Button("Scary"))
    {
        bg_type = "scary";
        std::cout << "Scary chosen" << std::endl;
        return bg_type;
    }

    return bg_type;
}

void DearImGui::PortalCol(glm::vec3& col) {
    ImGui::Text("Choose a portal colour:");

    R = col.r * 255;
    G = col.g * 255;
    B = col.b * 255;
   
    //ImGui::Text("Current RGB values: R=%.0f , G=%.0f , B=%.0f", R, G, B);

    ImGui::SliderInt("Red", &R, 0, 255);
    ImGui::SliderInt("Green", &G, 0, 255);
    ImGui::SliderInt("Blue", &B, 0, 255);

    col.r = R / 255.0f;
    col.g = G / 255.0f;
    col.b = B / 255.0f;

}

void DearImGui::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

DearImGui::Window DearImGui::UseWindow(const char* name)
{
    return name;
}

DearImGui::Window::Window(const char* name) : m_open(false)
{
    m_open = ImGui::Begin(name);
}

DearImGui::Window::~Window()
{
    ImGui::End();
}

DearImGui::Window::operator bool() const
{
    return m_open;
}

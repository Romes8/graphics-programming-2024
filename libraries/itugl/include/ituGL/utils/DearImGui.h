#pragma once

#include <string>
#include <glm/glm.hpp>
#include <vector>

class Window;

class DearImGui
{
public:
    class Window
    {
    public:
        ~Window();
        operator bool() const;
    private:
        friend DearImGui;
        Window(const char* name);
    private:
        bool m_open;
    };

public:
    DearImGui();
    ~DearImGui();

    void Initialize(::Window& window);
    void Cleanup();
    std::string Portal(std::vector<glm::vec3> ColPalette, glm::vec3& col,bool& zoom);
    std::string MainBG();
    void PortalCol(glm::vec3& col);


    void BeginFrame();
    void EndFrame();

    Window UseWindow(const char* name);

private:
    std::string portal_type = "forest"; // Default value forest
    std::string bg_type = "room"; // Default value forest


};

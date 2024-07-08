#include "ParticlesApplication.h"

#include <ituGL/shader/Shader.h>
#include <ituGL/geometry/VertexAttribute.h>
#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/utils/DearImGui.h>
#include <stb_image.h>
#include <cassert>
#include <array>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/trigonometric.hpp>
#include <vector>
#include <cmath>

const float CIRCLE_RADIUS = 0.4f;
std::string portal_type = "";
std::string bg_type = "";

int width, height;
glm::vec3 averageColor;


// Color values for scenarios
// Forest 
glm::vec3 ForestCol;

// Scary 
glm::vec3 ScaryCol;

// default room 
glm::vec3 RoomCol;

glm::vec3 CurrentCol; //Caries the colours of the current portal

GLuint currentPortalBackground;
GLuint currentBackground;
std::vector<glm::vec3> ColorPalette;



struct Particle
{
    glm::vec2 position;
    float size;
    float birth;
    float duration;
    Color color;
    glm::vec2 velocity;
};

const std::array<VertexAttribute, 6> s_vertexAttributes =
{
    VertexAttribute(Data::Type::Float, 2), // position
    VertexAttribute(Data::Type::Float, 1), // size
    VertexAttribute(Data::Type::Float, 1), // birth
    VertexAttribute(Data::Type::Float, 1), // duration
    VertexAttribute(Data::Type::Float, 4), // color
    VertexAttribute(Data::Type::Float, 2), // velocity
};


ParticlesApplication::ParticlesApplication()
    : Application(1024, 1024, "Particles demo")
    , m_currentTimeUniform(0)
    , m_gravityUniform(0)
    , m_mousePosition(0)
    , m_particleCount(0)
    , m_particleCapacity(4000)
{
}

void ParticlesApplication::Initialize()
{

    InitializeGeometry();

    // First background
    m_backgroundTexture = LoadTexture("Images/room-default.jpg", RoomCol);
    ColorPalette.push_back(RoomCol); // 0

    // Portal backgrounds
    m_forestbackgroundTexture = LoadTexture("Images/portal.jpg", ForestCol); 
    //std::cout << "Forest Color: R=" << ForestCol.r << ", G=" << ForestCol.g << ", B=" << ForestCol.b << std::endl;
    ColorPalette.push_back(ForestCol); // 1

    m_scarybackgroundTexture = LoadTexture("Images/scary.jpg", ScaryCol);
    //std::cout << "Scary Color: R=" << ScaryCol.r << ", G=" << ScaryCol.g << ", B=" << ScaryCol.b << std::endl;
    ColorPalette.push_back(ScaryCol);  // 2


    // Set the default BG and Portal BG
    currentPortalBackground = m_forestbackgroundTexture;
    currentBackground = m_backgroundTexture;
    CurrentCol = ForestCol;

    InitializeShaders();

    m_mousePosition = GetMainWindow().GetMousePosition(true);

    GetDevice().EnableFeature(GL_PROGRAM_POINT_SIZE);

    GetDevice().EnableFeature(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    GetDevice().SetVSyncEnabled(true);

    m_currentTimeUniform = m_shaderProgram.GetUniformLocation("CurrentTime");

    m_gravityUniform = m_shaderProgram.GetUniformLocation("Gravity");

    m_imgui.Initialize(GetMainWindow());

}

void ParticlesApplication::Update()
{
    Application::Update();

    const Window& window = GetMainWindow();

    int width, height;
    window.GetDimensions(width, height);

    glm::vec2 center(0.0f, 0.0f);

    int numParticles = 40; // Number of particle points to spawn in the circle
    float radius = CIRCLE_RADIUS;   // Radius of the circle
    float angularSpeed = glm::radians(120.0f); // Angular speed in radians


    // Calculate angular speed
    static float elapsedTime = 0.0f;
    elapsedTime += GetDeltaTime();
    float rotationAngle = angularSpeed * elapsedTime;

    // Spawning the particles in a circle
    for (int i = 0; i < numParticles; ++i)
    {
        float angle = glm::radians((360.0f / numParticles) * i) + rotationAngle;
        glm::vec2 position = center + radius * glm::vec2(cos(angle), sin(angle));
        float size = RandomRange(10.0f, 30.0f);
        float duration = RandomRange(0.5f, 0.9f);
        Color color = RandomColor();
        float velocityMagnitude = RandomRange(0.5f, 1.0f);
        glm::vec2 velocityDirection = glm::vec2(std::cos(angle + glm::radians(90.0f)), std::sin(angle + glm::radians(90.0f)));
        glm::vec2 velocity = velocityDirection * velocityMagnitude;

        EmitParticle(position, size, duration, color, velocity);
    }
}

void ParticlesApplication::Render()
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


    glEnable(GL_STENCIL_TEST); 

    // Draw the stencil
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable color  
    glDepthMask(GL_FALSE); // Disable depth  

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    StencilCircle();

    // Step 2: Render the second background
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color 
    glDepthMask(GL_TRUE); // Enable depth  

    glStencilFunc(GL_EQUAL, 1, 0xFF); 
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    RenderPortalBackground();

    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    RenderBackground();

    // Disable stencil test to draw particles on top of the image
    glDisable(GL_STENCIL_TEST);

    

    m_shaderProgram.Use();
    m_shaderProgram.SetUniform(m_currentTimeUniform, GetCurrentTime());
    m_shaderProgram.SetUniform(m_gravityUniform, 0.0f);
    m_vao.Bind();

    glDrawArrays(GL_POINTS, 0, std::min(m_particleCount, m_particleCapacity));


    // UI section
    m_imgui.BeginFrame();
    portal_type = m_imgui.Portal(ColorPalette, CurrentCol);
    bg_type = m_imgui.MainBG();
    m_imgui.PortalCol(CurrentCol); // Colour change in the UI


    m_imgui.EndFrame();
 
    Application::Render();
}

void ParticlesApplication::InitializeGeometry()
{
    m_vbo.Bind();
    m_vbo.AllocateData(m_particleCapacity * sizeof(Particle), BufferObject::Usage::DynamicDraw);
    m_vao.Bind();

    GLsizei stride = sizeof(Particle);
    GLint offset = 0;
    GLuint location = 0;
    for (const VertexAttribute& attribute : s_vertexAttributes)
    {
        m_vao.SetAttribute(location++, attribute, offset, stride);
        offset += attribute.GetSize();
    }

    VertexArrayObject::Unbind();
    VertexBufferObject::Unbind();
}

void ParticlesApplication::InitializeShaders()
{

    Shader vertexShader(Shader::VertexShader);
    LoadAndCompileShader(vertexShader, "shaders/particles.vert");

    Shader fragmentShader(Shader::FragmentShader);
    LoadAndCompileShader(fragmentShader, "shaders/particles.frag");

    if (!m_shaderProgram.Build(vertexShader, fragmentShader))
    {
        std::cout << "Error linking shaders" << std::endl;
    }

    // Background shaders
    Shader backgroundVertexShader(Shader::VertexShader);
    LoadAndCompileShader(backgroundVertexShader, "shaders/background.vert");

    Shader backgroundFragmentShader(Shader::FragmentShader);
    LoadAndCompileShader(backgroundFragmentShader, "shaders/background.frag");

    if (!m_backgroundShaderProgram.Build(backgroundVertexShader, backgroundFragmentShader))
    {
        std::cout << "Error linking background shaders" << std::endl;
    }

    // Portal background shaders
    Shader PortalbackgroundVertexShader(Shader::VertexShader);
    LoadAndCompileShader(PortalbackgroundVertexShader, "shaders/portal.vert");

    Shader PortalbackgroundFragmentShader(Shader::FragmentShader);
    LoadAndCompileShader(PortalbackgroundFragmentShader, "shaders/portal.frag");

    if (!m_PortalbackgroundShaderProgram.Build(PortalbackgroundVertexShader, PortalbackgroundFragmentShader))
    {
        std::cout << "Error linking background shaders" << std::endl;
    }

    m_currentTimeUniform = m_shaderProgram.GetUniformLocation("CurrentTime");
    m_gravityUniform = m_shaderProgram.GetUniformLocation("Gravity");
}

void ParticlesApplication::EmitParticle(const glm::vec2& position, float size, float duration, const Color& color, const glm::vec2& velocity)
{
    Particle particle;
    particle.position = position;
    particle.size = size;
    particle.birth = GetCurrentTime();
    particle.duration = duration;
    particle.color = color;
    particle.velocity = velocity;

    unsigned int particleIndex = m_particleCount % m_particleCapacity;

    m_vbo.Bind();
    int offset = particleIndex * sizeof(Particle);
    m_vbo.UpdateData(std::span(&particle, 1), offset);

    VertexBufferObject::Unbind();

    m_particleCount++;
}

void ParticlesApplication::LoadAndCompileShader(Shader& shader, const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Can't find file: " << path << std::endl;
        std::cout << "Is your working directory properly set?" << std::endl;
        return;
    }

    std::stringstream stringStream;
    stringStream << file.rdbuf() << '\0';

    shader.SetSource(stringStream.str().c_str());

    if (!shader.Compile())
    {
        std::array<char, 256> errors;
        shader.GetCompilationErrors(errors);
        std::cout << "Error compiling shader: " << path << std::endl;
        std::cout << errors.data() << std::endl;
    }
}

// Colour section

float ParticlesApplication::RandomR()
{
    return CurrentCol.r;
}

float ParticlesApplication::RandomG()
{
    return CurrentCol.g;
}

float ParticlesApplication::RandomB()
{
    return CurrentCol.b;
}

float ParticlesApplication::RandomNum()
{
    return static_cast<float>(rand()) / RAND_MAX;
}

float ParticlesApplication::RandomRange(float from, float to)
{
    return RandomNum() * (to - from) + from;
}

glm::vec2 ParticlesApplication::RandomDirection()
{
    return glm::normalize(glm::vec2(RandomNum() - 0.5f, RandomNum() - 0.5f));
}

Color ParticlesApplication::RandomColor()
{
    return Color(RandomR(), RandomG(), RandomB());
}


GLuint ParticlesApplication::LoadTexture(const char* filename, glm::vec3& SceneCol)
{
    stbi_set_flip_vertically_on_load(true);

    int nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    // Calculate the colours
    int pixelCount = width * height;
    if (nrChannels == 3 || nrChannels == 4)
    {
        for (int i = 0; i < pixelCount; ++i)
        {
            SceneCol.r += static_cast<float>(data[i * nrChannels + 0]);
            SceneCol.g += static_cast<float>(data[i * nrChannels + 1]);
            SceneCol.b += static_cast<float>(data[i * nrChannels + 2]);
        }

        SceneCol /= static_cast<float>(pixelCount * 255.0f);
    }

    std::cout << "Average Color: R=" << SceneCol.r << ", G=" << SceneCol.g << ", B=" << SceneCol.b << std::endl;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    if (nrChannels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    else if (nrChannels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}


void ParticlesApplication::RenderBackground()
{

    // Default value is Forest
    GLuint last_value = currentBackground;

    if (bg_type == "room") {
        currentBackground = m_backgroundTexture;
    }
    else if (bg_type == "") {
        currentBackground = m_scarybackgroundTexture;
    }
    else {
        currentBackground = m_forestbackgroundTexture; // Safe in case value is missing
    }

    static const float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };

    static const unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    static GLuint quadVAO = 0;
    static GLuint quadVBO, quadEBO;

    if (quadVAO == 0)
    {
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &quadEBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0); 
    }

    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    m_backgroundShaderProgram.Use();

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticlesApplication::RenderPortalBackground()
{

    // Default value is Forest
    GLuint last_value = currentPortalBackground;

    if (portal_type == "forest") {
        currentPortalBackground = m_forestbackgroundTexture;
        //CurrentCol = ForestCol;
    }
    else if (portal_type == "scary") {
        currentPortalBackground = m_scarybackgroundTexture;
        //CurrentCol = ScaryCol;
    }
    else {
        currentPortalBackground = m_forestbackgroundTexture; // Safe in case value is missing
        //CurrentCol = ForestCol;
    }

    // Render smaller image to create a better looking portal image
    static const float quadVertices[] = {
        -0.8f,  0.8f,  0.0f, 1.0f, 
        -0.8f, -0.8f,  0.0f, 0.0f,  
         0.8f, -0.8f,  1.0f, 0.0f,  
         0.8f,  0.8f,  1.0f, 1.0f   
    };

    static const unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    static GLuint quadVAO = 0;
    static GLuint quadVBO, quadEBO;

    if (quadVAO == 0)
    {
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &quadEBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    // Portal change 
    glBindTexture(GL_TEXTURE_2D, currentPortalBackground);
    m_PortalbackgroundShaderProgram.Use();

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticlesApplication::StencilCircle() {
    const int num_segments = 100; // Number of segments for circle
    const float radius = CIRCLE_RADIUS; // Radius of the circle
    const float centerX = 0.0f;   
    const float centerY = 0.0f;

    std::vector<float> vertices;
    vertices.push_back(centerX);
    vertices.push_back(centerY);

    for (int i = 0; i <= num_segments; ++i)
    {
        float angle = 2.0f * 3.14 * float(i) / float(num_segments);
        float x = radius * cosf(angle);
        float y = radius * sinf(angle);
        vertices.push_back(x + centerX);
        vertices.push_back(y + centerY);
    }

    GLuint circleVAO, circleVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 2);
    glBindVertexArray(0);

    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &circleVAO);
}
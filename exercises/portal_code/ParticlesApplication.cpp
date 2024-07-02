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

// Structure defining that Particle data
struct Particle
{
    glm::vec2 position;
    float size;
    float birth;
    float duration;
    Color color;
    glm::vec2 velocity;
};

// List of attributes of the particle. Must match the structure above
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
    , m_particleCapacity(4000)  // You can change the capacity here to have more particles
{
}

void ParticlesApplication::Initialize()
{

    InitializeGeometry();

    // First background
    m_backgroundTexture = LoadTexture("Images/room-default.jpg"); 

    // Portal background
    m_PortalbackgroundTexture = LoadTexture("Images/portal.jpg"); 



    InitializeShaders();

    // Initialize the mouse position with the current position of the mouse
    m_mousePosition = GetMainWindow().GetMousePosition(true);

    // Enable GL_PROGRAM_POINT_SIZE to have variable point size per-particle
    GetDevice().EnableFeature(GL_PROGRAM_POINT_SIZE);

    // Enable GL_BLEND to have blending on the particles, and configure it as additive blending
    GetDevice().EnableFeature(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // We need to enable V-sync, otherwise the framerate would be too high and spawn multiple particles in one click
    GetDevice().SetVSyncEnabled(true);

    // Get "CurrentTime" uniform location in the shader program
    m_currentTimeUniform = m_shaderProgram.GetUniformLocation("CurrentTime");

    // Get "Gravity" uniform location in the shader program
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

    int numParticles = 40; // Number of particles to spawn in the circle
    float radius = CIRCLE_RADIUS;   // Radius of the circle
    float angularSpeed = glm::radians(120.0f); // Angular speed in radians per second


    // Calculate angular speed for the motion effect
    static float elapsedTime = 0.0f;
    elapsedTime += GetDeltaTime(); // Increment elapsed time
    float rotationAngle = angularSpeed * elapsedTime;

    //spawning the particles in a circle
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

        /* std::cout << "Particle : " << i << std::endl;
        std::cout << "Position x: " << position.x << std::endl;
        std::cout << "Position y: " << position.y << std::endl;*/

       // std::cout << "Veloxity x: " << velocity.x << std::endl;
       // std::cout << "Veloxity y: " << velocity.y << std::endl;


        EmitParticle(position, size, duration, color, velocity);
    }

}

void ParticlesApplication::Render()
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Clear background
    GetDevice().Clear(Color(0.0f, 0.0f, 0.0f));

    glEnable(GL_STENCIL_TEST); 

    // Draw the stencil pattern
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

    // Disable stencil test to draw particles on top of the images
    glDisable(GL_STENCIL_TEST);

    

    m_shaderProgram.Use();
    m_shaderProgram.SetUniform(m_currentTimeUniform, GetCurrentTime());
    m_shaderProgram.SetUniform(m_gravityUniform, 0.0f);
    m_vao.Bind();

    glDrawArrays(GL_POINTS, 0, std::min(m_particleCount, m_particleCapacity));


    m_imgui.BeginFrame();

    m_imgui.Button();

    // End ImGui frame
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

// Load, compile and Build shaders
void ParticlesApplication::InitializeShaders()
{

    // Load and compile vertex shader
    Shader vertexShader(Shader::VertexShader);
    LoadAndCompileShader(vertexShader, "shaders/particles.vert");

    // Load and compile fragment shader
    Shader fragmentShader(Shader::FragmentShader);
    LoadAndCompileShader(fragmentShader, "shaders/particles.frag");

    // Attach shaders and link
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

    // Get uniform locations for particle shader
    m_currentTimeUniform = m_shaderProgram.GetUniformLocation("CurrentTime");
    m_gravityUniform = m_shaderProgram.GetUniformLocation("Gravity");
}

void ParticlesApplication::EmitParticle(const glm::vec2& position, float size, float duration, const Color& color, const glm::vec2& velocity)
{
    // Initialize the particle
    Particle particle;
    particle.position = position;
    particle.size = size;
    particle.birth = GetCurrentTime();
    particle.duration = duration;
    particle.color = color;
    particle.velocity = velocity;

    // Get the index in the circular buffer
    unsigned int particleIndex = m_particleCount % m_particleCapacity;

    // Bind the VBO before updating data
    m_vbo.Bind();

    // Update the particle data in the VBO
    int offset = particleIndex * sizeof(Particle);
    m_vbo.UpdateData(std::span(&particle, 1), offset);

    // Unbind the VBO
    VertexBufferObject::Unbind();

    // Increment the particle count
    m_particleCount++;
}

void ParticlesApplication::LoadAndCompileShader(Shader& shader, const char* path)
{
    // Open the file for reading
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Can't find file: " << path << std::endl;
        std::cout << "Is your working directory properly set?" << std::endl;
        return;
    }

    // Dump the contents into a string
    std::stringstream stringStream;
    stringStream << file.rdbuf() << '\0';

    // Set the source code from the string
    shader.SetSource(stringStream.str().c_str());

    // Try to compile
    if (!shader.Compile())
    {
        // Get errors in case of failure
        std::array<char, 256> errors;
        shader.GetCompilationErrors(errors);
        std::cout << "Error compiling shader: " << path << std::endl;
        std::cout << errors.data() << std::endl;
    }
}

float ParticlesApplication::RandomR()
{
    //float num = (rand() % 20) / 20.0f;
    float num = 0.0f;

    //std::cout << "Red:" << num << std::endl;
    return num;
}

float ParticlesApplication::RandomG()
{
    float num = (rand() % 128 + 128) / 255.0f;
   // std::cout << "Green:" << num << std::endl;
    return num;
}

float ParticlesApplication::RandomB()
{
   // float num = (rand() % 20 / 20.0f);
    float num = 0.0f;

   // std::cout << "Blue:" << num << std::endl;
    return num;
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
    //std::cout << "------------" << std::endl;
    return Color(RandomR(), RandomG(), RandomB());
}

GLuint ParticlesApplication::LoadTexture(const char* filename)
{

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

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

        glBindVertexArray(0); // Unbind VAO
    }

    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    m_backgroundShaderProgram.Use();

    // Render the quad
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticlesApplication::RenderPortalBackground()
{

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

    glBindTexture(GL_TEXTURE_2D, m_PortalbackgroundTexture);
    m_PortalbackgroundShaderProgram.Use();

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticlesApplication::StencilCircle() {
    const int num_segments = 100; // Number of segments to approximate the circle
    const float radius = CIRCLE_RADIUS;    // Radius of the circle
    const float centerX = 0.0f;   // X coordinate of the circle center
    const float centerY = 0.0f;   // Y coordinate of the circle center

    std::vector<float> vertices;
    vertices.push_back(centerX); // Center of the circle
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
#include "TerrainApplication.h"

// (todo) 01.1: Include the libraries you need

#include <cmath>
#include <list>
#include <vector>
#include <iostream>

#include <ituGL/geometry/VertexBufferObject.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/geometry/VertexAttribute.h>
#include <ituGL/geometry/ElementBufferObject.h>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>


int vertexCount;

// Helper structures. Declared here only for this exercise
struct Vector2
{
    Vector2() : Vector2(0.f, 0.f) {}
    Vector2(float x, float y) : x(x), y(y) {}
    float x, y;
};

struct Vector3
{
    Vector3() : Vector3(0.f, 0.f, 0.f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    float x, y, z;

    Vector3 Normalize() const
    {
        float length = std::sqrt(1 + x * x + y * y);
        return Vector3(x / length, y / length, z / length);
    }
};

struct Vertex
{
    Vector3 position;
    Vector2 texCoord;
    Vector3 color;
    Vector3 normal;
};

Vector3 GetColorFromHeight(float height);




TerrainApplication::TerrainApplication()
    : Application(1024, 1024, "Terrain demo"), m_gridX(16), m_gridY(16), m_shaderProgram(0)
{
}

void TerrainApplication::Initialize()
{
    //potentially move to initialize constructor
    Application::Initialize();

    // Build shaders and store in m_shaderProgram
    BuildShaders();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float scaleX = 1.0f / m_gridX;
    float scaleY = 1.0f / m_gridY;

    unsigned int columnCount = m_gridX + 1;
    unsigned int rowCount = m_gridY + 1;

    for (unsigned int j = 0; j < rowCount; ++j)
    {
        for (unsigned int i = 0; i < columnCount; ++i)
        {
            // Compute the position of the vertex
            Vertex& vertex = vertices.emplace_back();
            float x = i * scaleX - 0.5f;
            float y = j * scaleY - 0.5f;
            float z = stb_perlin_fbm_noise3(x * 2, y * 2, 0.0f, 1.9f, 0.5f, 8) * 0.5f;

            vertex.position = Vector3(x, y, z);
            vertex.texCoord = Vector2(static_cast<float>(i), static_cast<float>(j));
            vertex.color = GetColorFromHeight(z);
            vertex.normal = Vector3(0.0f, 0.0f, 1.0f);

            // Skip adding indices if it's the first row or column
            if (i == 0 || j == 0) {
                continue;
            }

            // Calculate indices for the quad formed by this and previous vertices
            unsigned int current = j * columnCount + i;
            unsigned int topLeft = (j - 1) * columnCount + i - 1;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = current - 1;
            unsigned int bottomRight = current;

            // Define the two triangles of the quad
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    for (unsigned int j = 0; j < rowCount; ++j)
    {
        for (unsigned int i = 0; i < columnCount; ++i)
        {
            // Get the vertex at (i, j)
            int index = j * columnCount + i;
            Vertex& vertex = vertices[index];

            // Compute the delta in X
            unsigned int prevX = i > 0 ? index - 1 : index;
            unsigned int nextX = i < m_gridX ? index + 1 : index;
            float deltaHeightX = vertices[nextX].position.z - vertices[prevX].position.z;
            float deltaX = vertices[nextX].position.x - vertices[prevX].position.x;
            float x = deltaHeightX / deltaX;

            // Compute the delta in Y
            int prevY = j > 0 ? index - columnCount : index;
            int nextY = j < m_gridY ? index + columnCount : index;
            float deltaHeightY = vertices[nextY].position.z - vertices[prevY].position.z;
            float deltaY = vertices[nextY].position.y - vertices[prevY].position.y;
            float y = deltaHeightY / deltaY;

            // Compute the normal
            vertex.normal = Vector3(x, y, 1.0f).Normalize();
        }
    }
    // Define the attributes
    VertexAttribute positionAttribute(Data::Type::Float, 3);
    VertexAttribute textureAttribute(Data::Type::Float, 2);
    VertexAttribute colorAttribute(Data::Type::Float, 3);
    VertexAttribute normalAttribute(Data::Type::Float, 3);


    size_t positionOffset = 0u;
    size_t texCoordOffset = positionOffset + positionAttribute.GetSize();
    size_t colorOffset = texCoordOffset + textureAttribute.GetSize();
    size_t normalOffset = colorOffset + colorAttribute.GetSize();

    vbo.Bind();
    vbo.AllocateData(std::span(vertices));

    GLsizei stride = sizeof(Vertex);

    // Set the vertex attributes
    vao.Bind();
    vao.SetAttribute(0, positionAttribute, static_cast<GLint>(positionOffset), stride);
    vao.SetAttribute(1, textureAttribute, static_cast<GLint>(texCoordOffset), stride);
    vao.SetAttribute(2, colorAttribute, static_cast<GLint>(colorOffset), stride);
    vao.SetAttribute(3, normalAttribute, static_cast<GLint>(normalOffset), stride);

    ebo.Bind();
    ebo.AllocateData(std::span(indices));

    // Unbind resources
    vao.Unbind();
    vbo.Unbind();
    ebo.Unbind();

    // Enable wireframe mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    glEnable(GL_DEPTH_TEST);
}

void TerrainApplication::Update()
{
    Application::Update();

    UpdateOutputMode();
}

void TerrainApplication::Render()
{
    Application::Render();

    // Clear color and depth
    GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

    // Set shader to be used
    glUseProgram(m_shaderProgram);

    // (todo) 01.1: Draw the grid

    vao.Bind();
    ebo.Bind();
  
    glDrawElements(GL_TRIANGLES, m_gridX * m_gridY * 6, GL_UNSIGNED_INT, nullptr);

}

void TerrainApplication::Cleanup()
{
    Application::Cleanup();
}

Vector3 GetColorFromHeight(float height)
{
    if (height > 0.3f)
    {
        return Vector3(1.0f, 1.0f, 1.0f); // Snow
    }
    else if (height > 0.1f)
    {
        return Vector3(0.3f, 0.3f, 0.35f); // Rock
    }
    else if (height > -0.05f)
    {
        return Vector3(0.1f, 0.4f, 0.15f); // Grass
    }
    else if (height > -0.1f)
    {
        return Vector3(0.6f, 0.5f, 0.4f); // Sand
    }
    else
    {
        return Vector3(0.1f, 0.1f, 0.3f); // Water
    }
}

void TerrainApplication::BuildShaders()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "layout (location = 2) in vec3 aColor;\n"
        "layout (location = 3) in vec3 aNormal;\n"
        "uniform mat4 Matrix = mat4(1);\n"
        "out vec2 texCoord;\n"
        "out vec3 color;\n"
        "out vec3 normal;\n"
        "void main()\n"
        "{\n"
        "   texCoord = aTexCoord;\n"
        "   color = aColor;\n"
        "   normal = aNormal;\n"
        "   gl_Position = Matrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
    const char* fragmentShaderSource = "#version 330 core\n"
        "uniform uint Mode = 0u;\n"
        "in vec2 texCoord;\n"
        "in vec3 color;\n"
        "in vec3 normal;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   switch (Mode)\n"
        "   {\n"
        "   default:\n"
        "   case 0u:\n"
        "       FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
        "       break;\n"
        "   case 1u:\n"
        "       FragColor = vec4(fract(texCoord), 0.0f, 1.0f);\n"
        "       break;\n"
        "   case 2u:\n"
        "       FragColor = vec4(color, 1.0f);\n"
        "       break;\n"
        "   case 3u:\n"
        "       FragColor = vec4(normalize(normal), 1.0f);\n"
        "       break;\n"
        "   case 4u:\n"
        "       FragColor = vec4(color * max(dot(normalize(normal), normalize(vec3(1,0,1))), 0.2f), 1.0f);\n"
        "       break;\n"
        "   }\n"
        "}\n\0";

    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    m_shaderProgram = shaderProgram;
}

void TerrainApplication::UpdateOutputMode()
{
    for (int i = 0; i <= 4; ++i)
    {
        if (GetMainWindow().IsKeyPressed(GLFW_KEY_0 + i))
        {
            int modeLocation = glGetUniformLocation(m_shaderProgram, "Mode");
            glUseProgram(m_shaderProgram);
            glUniform1ui(modeLocation, i);
            break;
        }
    }
    if (GetMainWindow().IsKeyPressed(GLFW_KEY_TAB))
    {
        const float projMatrix[16] = { 0, -1.294f, -0.721f, -0.707f, 1.83f, 0, 0, 0, 0, 1.294f, -0.721f, -0.707f, 0, 0, 1.24f, 1.414f };
        int matrixLocation = glGetUniformLocation(m_shaderProgram, "Matrix");
        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(matrixLocation, 1, false, projMatrix);
    }
}

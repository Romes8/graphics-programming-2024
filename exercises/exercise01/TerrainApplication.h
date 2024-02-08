#pragma once

#include <ituGL/application/Application.h>
#include <ituGL/geometry/VertexBufferObject.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/geometry/VertexAttribute.h>
#include <ituGL/geometry/ElementBufferObject.h>


// (todo) 01.1: Include the libraries you need


class TerrainApplication : public Application
{
public:
    TerrainApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    unsigned int m_gridX, m_gridY;

    // (todo) 01.1: Declare an VBO, VAO


    VertexBufferObject vbo;
    VertexArrayObject vao;
    ElementBufferObject ebo; //not needed

    // (todo) 01.5: Declare an EBO

};

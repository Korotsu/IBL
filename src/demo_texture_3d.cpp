
#include <cstddef>
#include <vector>

#include <glad/glad.h>
#include <imgui.h>
#include <stb_perlin.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"

#include "demo_texture_3d.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float4 color;
    float2 uv;
};

DemoTexture3D::DemoTexture3D(const DemoInputs& inputs)
{
    mainCamera.position = { 0.f, 0.f, 2.f };

    // Upload vertex buffer
    {
        // In memory
        Vertex vertices[] =
        {
            // Quad (6 vertices)
            { { 0.5f,-0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f } },
            { {-0.5f,-0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } },
            { { 0.5f, 0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f } },

            { { 0.5f, 0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f } },
            { {-0.5f, 0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f } },
            { {-0.5f,-0.5f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } },
        };

        glGenBuffers(1, &vertexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, uv));
    }

    // Create program
    program = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec4 aColor;
        layout(location = 2) in vec2 aUV;

        out vec4 vColor;
        out vec2 vUV;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
            vColor = aColor;
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec4 vColor;
        in vec2 vUV;

        out vec4 fragColor;

        uniform sampler3D noise;
        uniform float time;

        void main()
        {
            vec3 color = vec3(texture(noise, vec3(vUV, time / 8)).r);

            if (color.r < 0.2)
                discard;

            fragColor = vec4(color, 1.0);
        }
        )GLSL"
    );

    // Create texture
    {
        // Creation d'une texture 3D avec perlin noise
        int size = 32;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_3D, texture);
        std::vector<float> pixels(size * size * size);
        int perlinSize = 4;
        for (int x = 0; x < size; ++x)
        {
            for (int y = 0; y < size; ++y)
            {
                for (int z = 0; z < size; ++z)
                {
                    float result = stb_perlin_noise3((float)x / size * perlinSize, (float)y / size * perlinSize, (float)z / size * perlinSize, perlinSize, perlinSize, perlinSize);
                    pixels[x + y * size + z * size * size] = result;
                }
            }
        }

        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, size, size, size, 0, GL_RED, GL_FLOAT, pixels.data());
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

DemoTexture3D::~DemoTexture3D()
{
    // Delete OpenGL objects
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void DemoTexture3D::UpdateAndRender(const DemoInputs& inputs)
{
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    static float time = 0.f;
    time += 1.f / 60.f;

    {
        static bool smooth = false;
        ImGui::Checkbox("smooth", &smooth);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
    }

    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_3D, texture);

    glUseProgram(program);
    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
    mat4 view       = mainCamera.GetViewMatrix();
    mat4 model      = mat4Identity();

    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view.e);
    glUniform1f(glGetUniformLocation(program, "time"), time);

    glBindVertexArray(vertexArrayObject);

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw X quads
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Draw quad
}

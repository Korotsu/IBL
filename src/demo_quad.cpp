
#include <cstddef>
#include <cstdio>

#include <glad/glad.h>
#include <imgui.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"
#include "demo_quad.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float4 color;
    float2 uv;
};

DemoQuad::DemoQuad(const DemoInputs& inputs)
{
    mainCamera.position = { 0.f, 0.f, 2.f };

    // Upload vertex buffer
    {
        // In memory
        Vertex vertices[] =
        {
            // Triangle (3 vertices)
            { { 0.5f,-0.5f, 0.f }, { 1.f, 0.f, 0.f, 1.f }, { 0.f, 0.f } },
            { {-0.5f,-0.5f, 0.f }, { 0.f, 1.f, 0.f, 1.f }, { 0.f, 0.f } },
            { { 0.0f, 0.5f, 0.f }, { 0.f, 0.f, 1.f, 1.f }, { 0.f, 0.f } },

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

        uniform sampler2D noise;

        void main()
        {
            vec3 color = vec3(texture(noise, vUV).r);
            fragColor = vec4(color, 1.0);
        }
        )GLSL"
    );

    // Create texture
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        gl::UploadPerlinNoise(512, 512, 0.f);
        gl::SetTextureDefaultParams(false);
    }
}

DemoQuad::~DemoQuad()
{
    // Delete OpenGL objects
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void DemoQuad::UpdateAndRender(const DemoInputs& inputs)
{
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    static float time = 0.f;
    time += 1.f / 60.f;

    {
        bool update = false;
        update |= ImGui::DragFloat("lacunarity", &noiseProps.lacunarity);
        update |= ImGui::DragFloat("gain", &noiseProps.gain);
        update |= ImGui::DragFloat("offset", &noiseProps.offset);
        update |= ImGui::DragInt("octaves", &noiseProps.octaves);
        ImGui::Checkbox("animate", &noiseProps.animate);
        update |= noiseProps.animate;

        if (noiseProps.animate)
            noiseProps.zValue = time;

        if (update)
        {
            gl::UploadPerlinNoise(128, 128, noiseProps.zValue, noiseProps.lacunarity, noiseProps.gain, noiseProps.offset, noiseProps.octaves);
            gl::SetTextureDefaultParams(false);
        }
    }

    ImGui::Image((ImTextureID)(size_t)texture, { 256, 256 });

    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUseProgram(program);
    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
    mat4 view       = mainCamera.GetViewMatrix();
    mat4 model      = mat4Translate({ calc::Sin(time * 0.1f * calc::TAU) * 0.1f, 0.f, 0.f }) * mat4RotateY(time) * mat4Scale(2.f);

    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, view.e);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, model.e);

    glBindVertexArray(vertexArrayObject);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDrawArrays(GL_TRIANGLES, 0, 3); // Draw triangle
    glDrawArrays(GL_TRIANGLES, 3, 6); // Draw quad
}

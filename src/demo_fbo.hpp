#pragma once

#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

class DemoFBO : public Demo
{
public:
    DemoFBO(const DemoInputs& inputs);
    ~DemoFBO() override;
    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "FBO"; }

    void RenderTavern(const mat4& projection, const mat4& view, const mat4& model);
    void RenderTavernWithPostprocess(const mat4& projection, const mat4& view, const mat4& model);

    GLuint GetDiffuseTexture() const { return diffuseTexture; }

protected:
    struct Framebuffer
    {
        void Generate(int width, int height);
        void Resize(int width, int height);
        void Delete();

        int width = 0;
        int height = 0;

        GLuint id = 0;
        GLuint finalTexture = 0;
        GLuint emissiveTexture = 0;
        GLuint depthRenderbuffer = 0;
    };

    Camera mainCamera = {};

    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;

    // First pass data (render offscreen)
    Framebuffer framebuffer = {};
    GLuint mainProgram = 0;
    GLuint diffuseTexture = 0;
    GLuint emissiveTexture = 0;
    MeshSlice fullscreenQuad = {};

    // Second pass data (postprocess)
    GLuint postProcessProgram = 0;
    MeshSlice obj = {};

    float time = 0.f;
};
#pragma once

#include "glad/glad.h"

#include "camera.hpp"
#include "demo.hpp"

class DemoTexture3D : public Demo
{
public:
    DemoTexture3D(const DemoInputs& inputs);
    ~DemoTexture3D() override;

    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "Texture 3D"; }

private:
    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;
    GLuint program = 0;
    GLuint texture = 0;

    Camera mainCamera = {};
};
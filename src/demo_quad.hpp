#pragma once

#include "glad/glad.h"

#include "camera.hpp"
#include "demo.hpp"

struct NoiseProperty
{
    float lacunarity = 2.f;
    float gain = 0.5f;
    float offset = 1.f;
    int octaves = 6;
    float zValue = 0.f;
    bool animate = false;
};

class DemoQuad : public Demo
{
public:
    DemoQuad(const DemoInputs& inputs);
    ~DemoQuad() override;

    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "Noise"; }

private:
    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;
    GLuint program = 0;
    GLuint texture = 0;

    NoiseProperty noiseProps = {};

    Camera mainCamera = {};
};
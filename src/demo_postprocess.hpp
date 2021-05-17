#pragma once

#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

class DemoPostProcess : public Demo
{
public:
	DemoPostProcess(const DemoInputs& inputs);
	~DemoPostProcess() override;

	void UpdateAndRender(const DemoInputs& inputs) override;
	const char* Name() const override { return "PostProcess"; }

private:
    struct Framebuffer
    {
        void Generate(int width, int height);
        void Resize(int width, int height);
        void Delete();

        int width = 0;
        int height = 0;

        GLuint id = 0;
        GLuint baseTexture = 0;
        GLuint finalTexture = 0;
        GLuint depthRenderbuffer = 0;
    };

    Camera mainCamera = {};

    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;

    // First pass data (render offscreen)
    Framebuffer framebuffer = {};
    GLuint mainProgram = 0;
    GLuint diffuseTexture = 0;
    GLuint specularTexture = 0;
    MeshSlice fullscreenQuad = {};

    // Second pass data (postprocess)
    GLuint postProcessProgram = 0;
    MeshSlice obj = {};

    //PostProcess Parameters
    float   lightIntensity  = 0;
    bool    inversed        = false;
    float   inverseCoef     = 1;
    bool    grayScaled      = false;
    float   grayScaleCoef   = 1;
    bool    kernelEffect    = false;

    float kernelCoef        = 1;
    float kernelIntensity   = 1;
    float kernelLine1[3]    = {0,0,0};
    float kernelLine2[3]    = {0,1,0};
    float kernelLine3[3]    = {0,0,0};
};
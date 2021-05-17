
#include <vector>
#include <imgui.h>

#include "calc.hpp"

#include "demo_mipmap.hpp"

DemoMipmap::DemoMipmap(const DemoInputs& inputs)
    : demoFBO(inputs)
{
    glBindTexture(GL_TEXTURE_2D, demoFBO.GetDiffuseTexture());
    
    // TODO: Remplacer le niveau 1 de mipmap par une texture unie
    {
        int width, height;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_HEIGHT, &height);

        std::vector<float4> mipmapLevel1(width * height, { 1.f, 0.f, 0.f, 1.f });
        std::vector<float4> mipmapLevel2(width * height, { 0.f, 1.f, 0.f, 1.f });
        std::vector<float4> mipmapLevel3(width * height, { 0.f, 0.f, 1.f, 1.f });
        //std::vector<float4> mipmapLevel1(width * height, color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapLevel1.data());
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapLevel2.data());
        //glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapLevel3.data());
        // Utiliser glTexImage2D
    }
}

DemoMipmap::~DemoMipmap()
{

}

static const char* getTextureFilterName(GLint value)
{
    switch (value)
    {
    case GL_NEAREST:                return "GL_NEAREST";
    case GL_LINEAR:                 return "GL_LINEAR";
    case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
    case GL_LINEAR_MIPMAP_NEAREST:  return "GL_LINEAR_MIPMAP_NEAREST";
    case GL_NEAREST_MIPMAP_LINEAR:  return "GL_NEAREST_MIPMAP_LINEAR";
    case GL_LINEAR_MIPMAP_LINEAR:   return "GL_LINEAR_MIPMAP_LINEAR";
    default:                        return "Unknown";
    }
}

void DemoMipmap::UpdateAndRender(const DemoInputs& inputs)
{
    // Update inputs
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    // Debug UI
    // Show texture filter combo box
    {
        glBindTexture(GL_TEXTURE_2D, demoFBO.GetDiffuseTexture());

        int minFilter;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
        bool selected = false;

        GLint filters[] = {
            GL_NEAREST,
            GL_LINEAR,
            GL_NEAREST_MIPMAP_NEAREST,
            GL_LINEAR_MIPMAP_NEAREST,
            GL_NEAREST_MIPMAP_LINEAR,
            GL_LINEAR_MIPMAP_LINEAR
        };

        if (ImGui::BeginCombo("texture filter", getTextureFilterName(minFilter)))
        {
            for (GLint filter : filters)
                if (ImGui::Selectable(getTextureFilterName(filter), &selected))
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);

            ImGui::EndCombo();
        }
    }

    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.1f, 400.f);
    mat4 view       = mainCamera.GetViewMatrix();

    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_FRAMEBUFFER_SRGB);
    demoFBO.RenderTavern(projection, view, mat4Identity());
    glDisable(GL_FRAMEBUFFER_SRGB);
}

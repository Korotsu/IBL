#include <cstddef>
#include <vector>
#include <imgui.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"
#include "demo_postprocess.hpp"


// Vertex format
struct Vertex
{
    float3 position;
    float2 uv;
};

DemoPostProcess::DemoPostProcess(const DemoInputs& inputs)
{
    // Upload vertex buffer
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;

        {
            VertexDescriptor descriptor = {};
            descriptor.size = sizeof(Vertex);
            descriptor.positionOffset = offsetof(Vertex, position);
            descriptor.hasUV = true;
            descriptor.uvOffset = offsetof(Vertex, uv);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            fullscreenQuad = meshBuilder.GenQuad(nullptr, 1.0f, 1.0f);
            obj = meshBuilder.LoadObj(nullptr, "media/fantasy_game_inn.obj", "media", 1.f);
        }

        // In VRAM
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        free(vertices);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, uv));
    }

    // Main program
    mainProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;

        out vec4 vColor;
        out vec2 vUV;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec2 vUV;
        layout(location = 0) out vec4 baseColor;
        layout(location = 1) out vec4 finalColor;

        uniform sampler2D diffuseTexture;  // Texture channel 0
        uniform sampler2D specularTexture; // Texture channel 1
        uniform float lightIntensity;

        void main()
        {
            vec3 diffuse  = texture(diffuseTexture, vUV).rgb;
            vec3 specular = texture(specularTexture, vUV).rgb;
            baseColor     = vec4(diffuse + specular * lightIntensity, 1.0);
            finalColor    = vec4(specular * lightIntensity, 1.0);
        }
        )GLSL"
    );

    // Post process program
    postProcessProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        out vec2 vUV;

        void main()
        {
            gl_Position = vec4(aPosition, 1.0);
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec2 vUV;
        layout(location = 0) out vec4 fragColor;

        uniform sampler2D   colorTexture;
        uniform mat4        colorTransform;
        uniform int         inversed;
        uniform float       inverseCoef;
        uniform int         grayScaled;
        uniform float       grayScaleCoef;
        uniform int         kernelEffect;
        uniform float       kernelIntensity;
        uniform mat3        kernel3;

        const float offset = 1.0 / 300.0;  

        void main()
        {
            fragColor = texture(colorTexture, vUV);

            if(inversed == 1)
            {
                fragColor *= vec4(vec3(inversed - texture(colorTexture, vUV)), 1.0) * inverseCoef;
            }

            if(grayScaled == 1)
            {
                vec4 fragColorTemp = texture(colorTexture, vUV);
                float average = 0.2126 * fragColorTemp.r + 0.7152 * fragColorTemp.g + 0.0722 * fragColorTemp.b;
                fragColor *= vec4(average, average, average, 1.0) * grayScaleCoef;
            }

            if(kernelEffect == 1)
            {
                vec2 offsets[9] = vec2[](
                vec2(-offset,  offset), // top-left
                vec2( 0.0f,    offset), // top-center
                vec2( offset,  offset), // top-right
                vec2(-offset,  0.0f),   // center-left
                vec2( 0.0f,    0.0f),   // center-center
                vec2( offset,  0.0f),   // center-right
                vec2(-offset, -offset), // bottom-left
                vec2( 0.0f,   -offset), // bottom-center
                vec2( offset, -offset)  // bottom-right    
                );

                float kernel[9] = float[](
                                -2, -2, -2,
                                -2,  9, -2,
                                -2, -2, -2
                );

                mat3 kernel2 = mat3(
                                -2, -2, -2,
                                -2,  9, -2,
                                -2, -2, -2
                );
    
                vec3 sampleTex[9];
                for(int i = 0; i < 9; i++)
                {
                    sampleTex[i] = vec3(texture(colorTexture, vUV + offsets[i]));
                }
                vec3 col = vec3(0.0);
                for(int j = 0; j < 3; j++)
                {
                    for(int i = 0; i < 3; i++)
                    {
                        col += (sampleTex[i + j * 3] * kernel3[j][i]);
                    }
                }
                fragColor *= vec4(col, 1.0) * kernelIntensity;
            }
        }
        )GLSL"
    );

    // Load diffuse/specular texture
    {
        glGenTextures(1, &diffuseTexture);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        gl::UploadImage("media/fantasy_game_inn_diffuse.png");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &specularTexture);
        glBindTexture(GL_TEXTURE_2D, specularTexture);
        gl::UploadImage("media/fantasy_game_inn_emissive.png");
        gl::SetTextureDefaultParams();
    }

    // Create framebuffer (for post process pass)
    framebuffer.Generate((int)inputs.windowSize.x, (int)inputs.windowSize.y);
}


void DemoPostProcess::Framebuffer::Generate(int width, int height)
{
    // Create base buffer
    {
        glGenTextures(1, &baseTexture);
        glBindTexture(GL_TEXTURE_2D, baseTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create final buffer
    {
        glGenTextures(1, &finalTexture);
        glBindTexture(GL_TEXTURE_2D, finalTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create depth buffer
    {
        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, baseTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, finalTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DemoPostProcess::Framebuffer::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    glBindTexture(GL_TEXTURE_2D, baseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}

void DemoPostProcess::Framebuffer::Delete()
{
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &baseTexture);
    glDeleteTextures(1, &finalTexture);
    glDeleteRenderbuffers(1, &depthRenderbuffer);
}

DemoPostProcess::~DemoPostProcess()
{
    // Delete OpenGL objects
    framebuffer.Delete();
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
    glDeleteProgram(mainProgram);
    glDeleteProgram(postProcessProgram);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void DemoPostProcess::UpdateAndRender(const DemoInputs& inputs)
{
    // Resize framebuffer if needed
    if ((int)inputs.windowSize.x != framebuffer.width || (int)inputs.windowSize.y != framebuffer.height)
        framebuffer.Resize((int)inputs.windowSize.x, (int)inputs.windowSize.y);

    // Update camera
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    // Show debug info
    static bool finalImage = false;
    ImGui::Checkbox("Final render"              , &finalImage);
    ImGui::SliderFloat("Light Intensity"        , &lightIntensity, -10.0f, 30.0f, "%.3f %");
    ImGui::Checkbox("Inversed ?"                , &inversed);
    ImGui::InputFloat("Inverse Coef : "        , &inverseCoef);
    ImGui::Checkbox("GrayScaled ?"              , &grayScaled);
    ImGui::InputFloat("GrayScale Coef : "      , &grayScaleCoef);
    ImGui::Checkbox("Kernel Effect ?"           , &kernelEffect);
    ImGui::InputFloat("Kernel Coef : "         , &kernelCoef);
    ImGui::InputFloat("Kernel Intensity : "    , &kernelIntensity);
    ImGui::InputFloat3("Kernel line 1 : "       , kernelLine1);
    ImGui::InputFloat3("Kernel line 2 : "       , kernelLine2);
    ImGui::InputFloat3("Kernel line 3 : "       , kernelLine3);

    ImVec2 imageSize = { 256, 256 };
    ImGui::Image((ImTextureID)(size_t)framebuffer.baseTexture, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Image((ImTextureID)(size_t)framebuffer.finalTexture, imageSize, ImVec2(0, 1), ImVec2(1, 0));

    // Setup main program uniforms
    {
        mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
        mat4 view = mainCamera.GetViewMatrix();
        mat4 model = mat4Scale(2.f);

        glUseProgram(mainProgram);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "model"), 1, GL_FALSE, model.e);

        glUniform1i(glGetUniformLocation(mainProgram, "diffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(mainProgram, "specularTexture"), 1);
        glUniform1f(glGetUniformLocation(mainProgram, "lightIntensity"), lightIntensity);
    }

    // Setup post process program uniforms
    {
        mat4 colorTransform = mat4Identity();
        /*{
            0.299f, 0.299f, 0.299f, 0.0f,
            0.587f, 0.587f, 0.587f, 0.0f,
            0.114f, 0.114f, 0.114f, 0.0f,
            0.000f, 0.000f, 0.000f, 1.0f,
        };*/

        glUseProgram(postProcessProgram);
        glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "colorTransform"), 1, GL_FALSE, colorTransform.e);
        glUniform1i(glGetUniformLocation(postProcessProgram, "inversed"), inversed);
        glUniform1f(glGetUniformLocation(postProcessProgram, "inverseCoef"), inverseCoef);
        glUniform1i(glGetUniformLocation(postProcessProgram, "grayScaled"), grayScaled);
        glUniform1f(glGetUniformLocation(postProcessProgram, "grayScaleCoef"), grayScaleCoef);
        glUniform1i(glGetUniformLocation(postProcessProgram, "kernelEffect"), kernelEffect);
        glUniform1f(glGetUniformLocation(postProcessProgram, "kernelIntensity"), kernelIntensity);

        GLfloat kernel[9]{ kernelLine1[0],kernelLine1[1],kernelLine1[2],
                            kernelLine2[0],kernelLine2[1],kernelLine2[2],
                            kernelLine3[0],kernelLine3[1],kernelLine3[2]};

        for (size_t i = 0; i < 9; i++)
        {
            kernel[i] *= kernelCoef;
        }
        

        glUniformMatrix3fv(glGetUniformLocation(postProcessProgram, "kernel3"), 1, GL_FALSE, kernel);
    }

    // Keep track of previous framebuffer to rebind it after offscreen rendering
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // =============================================
    // Start rendering
    // =============================================
    glEnable(GL_DEPTH_TEST);

    // We use the same vao for both passes
    glBindVertexArray(vertexArrayObject);

    // Render to framebuffer
    {
        glViewport(0, 0, framebuffer.width, framebuffer.height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(mainProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTexture);

        glDrawArrays(GL_TRIANGLES, obj.start, obj.count);

        glActiveTexture(GL_TEXTURE0);
    }

    // Render to screen
    {
        glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(postProcessProgram);
        glBindTexture(GL_TEXTURE_2D, finalImage ? framebuffer.finalTexture : framebuffer.baseTexture);
        glDrawArrays(GL_TRIANGLES, fullscreenQuad.start, fullscreenQuad.count);
    }
}
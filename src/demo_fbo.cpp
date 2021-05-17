#include <cstddef>
#include <cstdio>
#include <vector>

#include <imgui.h>
#include <stb_perlin.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"
#include "data.hpp"

#include "demo_fbo.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
};

DemoFBO::DemoFBO(const DemoInputs& inputs)
{
    // Upload vertex buffer
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;

        {
            VertexDescriptor descriptor = {};
            descriptor.size             = sizeof(Vertex);
            descriptor.positionOffset   = offsetof(Vertex, position);
            descriptor.hasUV            = true;
            descriptor.uvOffset         = offsetof(Vertex, uv);
            descriptor.hasNormal        = true;
            descriptor.normalOffset     = offsetof(Vertex, normal);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            fullscreenQuad = meshBuilder.GenQuad(nullptr, 1.0f, 1.0f);
            obj            = meshBuilder.LoadObj(nullptr, "media/fantasy_game_inn.obj", "media", 1.f);
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
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
    }

    // Main program
    {
        const char* vertexShaderSources[] = {
            R"GLSL(
            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec2 aUV;
            layout(location = 2) in vec3 aNormal;

            out vec4 vColor;
            out vec2 vUV;
            out vec3 vWorldPosition;
            out vec3 vWorldNormal;

            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;

            void main()
            {
                vec4 worldPos4 = model * vec4(aPosition, 1.0);
                gl_Position = projection * view * worldPos4;
                vUV = aUV;
                vWorldPosition = worldPos4.xyz / worldPos4.w;
                vWorldNormal = (model * vec4(aNormal, 0.0)).xyz; // Assuming model is scaled linearly
            }
            )GLSL"
        };

        char defines[256];
        sprintf(defines, "#define NB_LIGHTS %d\n", Tavern::CandlesCount);

        const char* fragmentShaderSources[] = {
            defines,
            R"GLSL(
            in vec2 vUV;
            in vec3 vWorldPosition;
            in vec3 vWorldNormal;
            layout(location = 0) out vec4 finalColor;
            layout(location = 1) out vec4 emissiveColor;

            uniform sampler2D diffuseTexture;  // Texture channel 0
            uniform sampler2D emissiveTexture; // Texture channel 1

            uniform vec3 ambientColor       = vec3(0.0063, 0.0014, 0.0008);
            uniform vec3 moonDiffuseColor   = vec3(0.0410, 0.0900, 0.2420);
            uniform vec3 candleDiffuseColor = vec3(1.0000, 1.0000, 0.0711);

            uniform vec3 candleWorldPositions[NB_LIGHTS];
            uniform float candleQuadAttenuation = 1.0;

            void main()
            {
                vec3 worldNormal = normalize(vWorldNormal); 

                vec3 moonVec = normalize(vec3(-5.0, 4.0, 3.0));

                vec3 lightDiffuse = vec3(0.0);
                lightDiffuse += max(dot(moonVec, worldNormal), 0.0) * moonDiffuseColor;
                
                // Compute candle diffuse lighting
                for (int i = 0; i < NB_LIGHTS; ++i)
                {
                    vec3 candleToFragVec = candleWorldPositions[i] - vWorldPosition;
                    float dist = length(candleToFragVec);
                    vec3 dir = normalize(candleToFragVec);
                    float attenuation = 1.0 / (1.0 + candleQuadAttenuation * (dist * dist));
                    lightDiffuse += attenuation * max(dot(dir, worldNormal), 0.0) * candleDiffuseColor;
                }

                vec3 diffuse = texture(diffuseTexture, vUV).rgb * lightDiffuse;
                vec3 emissive = texture(emissiveTexture, vUV).rgb;

                finalColor    = vec4(ambientColor + diffuse + emissive, 1.0);
                emissiveColor = vec4(emissive, 1.0);
            
                // Show normals only
                //finalColor    = vec4(worldNormal, 1.0);

                //finalColor      = vec4(texture(diffuseTexture, vUV).rgb, 1.0);
            }
            )GLSL"
        };

        mainProgram = gl::CreateProgram(
            ARRAYSIZE(vertexShaderSources),
            vertexShaderSources,
            ARRAYSIZE(fragmentShaderSources),
            fragmentShaderSources
        );

        glUseProgram(mainProgram);
        glUniform3fv(glGetUniformLocation(mainProgram, "candleWorldPositions"), Tavern::CandlesCount, Tavern::CandlesPositions[0].e);
    }

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

        uniform sampler2D colorTexture;
        uniform mat4 colorTransform;

        void main()
        {
            fragColor = colorTransform * texture(colorTexture, vUV);
        }
        )GLSL"
    );
    
    // Setup sane default uniforms
    glUseProgram(postProcessProgram);
    glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "colorTransform"), 1, GL_FALSE, mat4Identity().e);

    // Load diffuse/emissive texture
    {
        {
            glGenTextures(1, &diffuseTexture);
            glBindTexture(GL_TEXTURE_2D, diffuseTexture);
            gl::UploadImage("media/fantasy_game_inn_diffuse.png", true); // 2048x2048
            gl::SetTextureDefaultParams();
        }

        glGenTextures(1, &emissiveTexture);
        glBindTexture(GL_TEXTURE_2D, emissiveTexture);
        gl::UploadImage("media/fantasy_game_inn_emissive.png", true);
        gl::SetTextureDefaultParams();
    }

    // Create framebuffer (for post process pass)
    framebuffer.Generate((int)inputs.windowSize.x, (int)inputs.windowSize.y);
}

void DemoFBO::Framebuffer::Generate(int width, int height)
{
    // Create base buffer
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

    // Create final buffer
    {
        glGenTextures(1, &emissiveTexture);
        glBindTexture(GL_TEXTURE_2D, emissiveTexture);

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

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, emissiveTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->width = width;
    this->height = height;
}

void DemoFBO::Framebuffer::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, emissiveTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}

void DemoFBO::Framebuffer::Delete()
{
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &finalTexture);
    glDeleteTextures(1, &emissiveTexture);
    glDeleteRenderbuffers(1, &depthRenderbuffer);
}

DemoFBO::~DemoFBO()
{
    // Delete OpenGL objects
    framebuffer.Delete();
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &emissiveTexture);
    glDeleteProgram(mainProgram);
    glDeleteProgram(postProcessProgram);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void EditFloatUniform(GLuint program, const char* name, float speed = 0.01f)
{
    GLint location = glGetUniformLocation(program, name);
    if (location == -1)
        return;

    float value;
    glGetUniformfv(program, location, &value);
    if (ImGui::DragFloat(name, &value, speed))
        glUniform1f(location, value);
}

void EditColorUniform(GLuint program, const char* name)
{
    GLint location = glGetUniformLocation(program, name);
    if (location == -1)
        return;

    float gamma = 2.2f;

    float3 value;
    glGetUniformfv(program, location, value.e);
    value = calc::Pow(value, 1.f / gamma);
    if (ImGui::ColorEdit3(name, value.e, ImGuiColorEditFlags_Float))
    {
        value = calc::Pow(value, gamma);
        glUniform3fv(location, 1, value.e);
    }
}

void DemoFBO::UpdateAndRender(const DemoInputs& inputs)
{
    time += inputs.deltaTime;

    // Resize framebuffer if needed
    if ((int)inputs.windowSize.x != framebuffer.width || (int)inputs.windowSize.y != framebuffer.height)
        framebuffer.Resize((int)inputs.windowSize.x, (int)inputs.windowSize.y);

    // Update camera
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    // Show debug info
    static bool applyPostprocess = false;
    static bool showEmissive = false;
    ImGui::Checkbox("Perform post-process pass (use fbo)", &applyPostprocess);
    if (applyPostprocess)
    {
        ImGui::Checkbox("Show emissive only", &showEmissive);
        ImVec2 imageSize = { 128, 128 };
        ImGui::Image((ImTextureID)(size_t)framebuffer.finalTexture, imageSize,  ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
        ImGui::Image((ImTextureID)(size_t)framebuffer.emissiveTexture, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    }

    // Setup main program uniforms
    mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.1f, 400.f);
    mat4 view       = mainCamera.GetViewMatrix();
    mat4 model      = mat4Identity();

    glUseProgram(mainProgram);
    EditColorUniform(mainProgram, "candleDiffuseColor");
    EditColorUniform(mainProgram, "moonDiffuseColor");
    EditFloatUniform(mainProgram, "candleQuadAttenuation");

    // Setup post process program uniforms
    {
        mat4 colorTransform = //mat4Identity();
        {
            calc::Sin(time * calc::TAU) * 0.5f + 1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f,
        };

        glUseProgram(postProcessProgram);
        glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "colorTransform"), 1, GL_FALSE, colorTransform.e);
    }

    // =============================================
    // Start rendering
    // =============================================
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Keep track of previous framebuffer to rebind it after offscreen rendering
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // Render 3d model to framebuffer
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (applyPostprocess)
        {
            glViewport(0, 0, framebuffer.width, framebuffer.height);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        }
        else
        {
            glEnable(GL_FRAMEBUFFER_SRGB);
        }

        RenderTavern(projection, view, model);

        if (!applyPostprocess)
            glDisable(GL_FRAMEBUFFER_SRGB);
    }

    if (applyPostprocess)
    {
        // Render framebuffer to screen using postprocess shader and a fullscreen quad
        {
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
            glEnable(GL_FRAMEBUFFER_SRGB);
            glUseProgram(postProcessProgram);

            glBindTexture(GL_TEXTURE_2D, showEmissive ? framebuffer.emissiveTexture : framebuffer.finalTexture);
            glBindVertexArray(vertexArrayObject);

            glDrawArrays(GL_TRIANGLES, fullscreenQuad.start, fullscreenQuad.count);
            glDisable(GL_FRAMEBUFFER_SRGB);
        }

        // Copy framebuffer depth into backbuffer
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.id);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, framebuffer.width, framebuffer.height,
                viewport[0], viewport[1], viewport[2], viewport[3], 
                GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
    }
}

void DemoFBO::RenderTavern(const mat4& projection, const mat4& view, const mat4& model)
{
    // Setup main program uniforms
    {
        glUseProgram(mainProgram);

        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "model"), 1, GL_FALSE, model.e);

        glUniform1i(glGetUniformLocation(mainProgram, "diffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(mainProgram, "emissiveTexture"), 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, emissiveTexture);

        glBindVertexArray(vertexArrayObject);

        glDrawArrays(GL_TRIANGLES, obj.start, obj.count);

        glActiveTexture(GL_TEXTURE0);
    }
}

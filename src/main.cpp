
#include <cstdio>
#include <vector>

#ifdef _MSC_VER
#define USE_PAUL_DLL
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#define DLLEXPORT _declspec(dllexport)
#endif
#include <windows.h>
#else
#define DLLEXPORT 
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "types.hpp"
#include "calc.hpp"
#include "demo_fbo.hpp"
#include "demo_quad.hpp"
#include "demo_mipmap.hpp"
#include "demo_texture_3d.hpp"
#include "demo_dll_wrapper.hpp"
#include "demo_postprocess.hpp"

// TODO: Add demo include here

extern "C"
{
    DLLEXPORT int NvOptimusEnablement = 1;
    DLLEXPORT int AmdPowerXpressRequestHighPerformance = 1;
}

void debugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    printf("GL error = %s\n", message);
}

void debugGLFWCallback(int error_code, const char* description)
{
    printf("GLFW error (%x) = %s\n", error_code, description);
}

// ImGui/GLFW implementation of camera inputs
CameraInputs getCameraInputs(bool mouseCaptured, float mouseDX, float mouseDY)
{
    CameraInputs cameraInputs = {};
    if (!mouseCaptured)
        return cameraInputs;

    cameraInputs.mouseDX = mouseDX;
    cameraInputs.mouseDY = mouseDY;
    cameraInputs.deltaTime = ImGui::GetIO().DeltaTime;

    const struct CameraKeyMapping { int key; int flag; } cameraKeyMapping[]
    {
        { GLFW_KEY_W,            CAM_MOVE_FORWARD  },
        { GLFW_KEY_S,            CAM_MOVE_BACKWARD },
        { GLFW_KEY_A,            CAM_STRAFE_LEFT   },
        { GLFW_KEY_D,            CAM_STRAFE_RIGHT  },
        { GLFW_KEY_LEFT_SHIFT,   CAM_MOVE_FAST     },
        { GLFW_KEY_SPACE,        CAM_MOVE_UP       },
        { GLFW_KEY_LEFT_CONTROL, CAM_MOVE_DOWN     },
    };

    cameraInputs.keyInputsFlags = 0;
    for (int i = 0; i < ARRAYSIZE(cameraKeyMapping); ++i)
    {
        CameraKeyMapping map = cameraKeyMapping[i];
        cameraInputs.keyInputsFlags |= ImGui::IsKeyDown(map.key) ? map.flag : 0;
    }

    return cameraInputs;
}

int main(int argc, char* argv[])
{
    int initWidth  = 1280;
    int initHeight = 720;

    // Init glfw
    if (!glfwInit())
    {
        fprintf(stderr, "glfwInit failed");
        return 1;
    }

    glfwSetErrorCallback(debugGLFWCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(initWidth, initHeight, "IBL", nullptr, nullptr);
    if (window == nullptr)
    {
        fprintf(stderr, "glfwCreateWindow failed");
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // v-sync

    // Init glad
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        fprintf(stderr, "gladLoadGLLoader failed");
        return 1;
    }

    printf("GL_VENDOR = %s\n",   glGetString(GL_VENDOR));
    printf("GL_RENDERER = %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION = %s\n",  glGetString(GL_VERSION));

    // Setup KHR_debug callback
    if (GLAD_GL_KHR_debug)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugGLCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    }

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Init demo
    DemoInputs demoInputs = {};
    demoInputs.windowSize.x = (float)initWidth;
    demoInputs.windowSize.y = (float)initHeight;

    int demoId = 2;
    std::vector<Demo*> demos;
    demos.push_back(new DemoQuad(demoInputs));
    demos.push_back(new DemoFBO(demoInputs));
    demos.push_back(new DemoPostProcess(demoInputs));
    demos.push_back(new DemoMipmap(demoInputs));
    demos.push_back(new DemoTexture3D(demoInputs));
    // TODO: Here, add other demos
    //demos.push_back(new DemoBloom(demoInputs));

#ifdef USE_PAUL_DLL
    // Load some demo from dll
    HMODULE paulDemoLib = loadDemosInDll(demos, "ibl-paul.dll", demoInputs);
#endif

    // Various main loop variables
    bool showDemoWindow = false;
    bool mouseCaptured = false;
    double prevMouseX = 0.0;
    double prevMouseY = 0.0;

    while (glfwWindowShouldClose(window) == false)
    {
        glfwPollEvents();

        // ImGui NewFrame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        if (mouseCaptured)
            ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX); // Disable ImGui mouse handling
        ImGui::NewFrame();

        // Navigation UI
        {
            if (ImGui::Button("<"))
                demoId = calc::Modulo(demoId - 1, (int)demos.size());
            ImGui::SameLine();
            ImGui::Text("%d/%d", demoId + 1, demos.size());
            ImGui::SameLine();
            if (ImGui::Button(">"))
                demoId = calc::Modulo(demoId + 1, (int)demos.size());
            ImGui::SameLine();
            ImGui::Text("[%s]", demos[demoId]->Name());
        }

        // ImGui demo window
        ImGui::Checkbox("ImGui demo window", &showDemoWindow);
        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        // Mouse capture (Mouse right click to enable, escape key to disable)
        {
            if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE) && mouseCaptured)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                mouseCaptured = false;
            }

            if (!ImGui::GetIO().WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouseCaptured = true;
            }
        }

        // Compute mouse deltas
        float mouseDX;
        float mouseDY;
        {
            double mouseX;
            double mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            mouseDX = (float)(mouseX - prevMouseX);
            mouseDY = (float)(mouseY - prevMouseY);
            prevMouseX = mouseX;
            prevMouseY = mouseY;
        }

        // Update demo inputs
        demoInputs.deltaTime    = ImGui::GetIO().DeltaTime;
        demoInputs.windowSize   = { ImGui::GetIO().DisplaySize.x,ImGui::GetIO().DisplaySize.y };
        demoInputs.cameraInputs = getCameraInputs(mouseCaptured, mouseDX, mouseDY);

        // Render current demo
        demos[demoId]->UpdateAndRender(demoInputs);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present frame
        glfwSwapBuffers(window);
    }

    // Cleanup
    for (Demo* demo : demos)
        delete demo;

#ifdef USE_PAUL_DLL
    FreeLibrary(paulDemoLib);
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
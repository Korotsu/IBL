
#include <cassert>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "demo_dll_wrapper.hpp"

DemoDllWrapper::DemoDllWrapper(DemoPrototype demoPrototype, const DemoInputs& inputs)
    : demoPrototype(demoPrototype)
{
    demo = demoPrototype.demoBuild(inputs);
    assert(demo);
}

DemoDllWrapper::~DemoDllWrapper()
{
    demoPrototype.demoDestroy(demo);
}

void DemoDllWrapper::UpdateAndRender(const DemoInputs& inputs)
{
    demoPrototype.demoUpdateAndRender(demo, inputs);
}

const char* DemoDllWrapper::Name() const
{
    return demoPrototype.demoName(demo);
}
HMODULE loadDemosInDll(std::vector<Demo*>& demos, const char* file, const DemoInputs& inputs)
{
    HMODULE library = LoadLibrary(file);
    if (library == 0)
    {
        fprintf(stderr, "Cannot load library '%s'\n", file);
        return library;
    }
    GetDemoApiFunc getDemoApi = (GetDemoApiFunc)GetProcAddress(library, "getDemoApi");
    assert(getDemoApi);
    DemoApi demoApi = getDemoApi();

    // Init dll context
    demoApi.initContext(ImGui::GetCurrentContext(),
        [](size_t size, void* user_data) { IM_UNUSED(user_data); return malloc(size); },
        [](void* ptr, void* user_data) { IM_UNUSED(user_data); free(ptr); },
        (GLADloadproc)glfwGetProcAddress);

    // Get demo prototypes
    int count;
    DemoPrototype demoPrototypes[128];
    demoApi.getDemoPrototypes(ARRAYSIZE(demoPrototypes), &count, demoPrototypes);
    for (int i = 0; i < count; ++i)
        demos.push_back(new DemoDllWrapper(demoPrototypes[i], inputs));

    return library;
}
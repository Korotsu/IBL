#pragma once

#include <vector>

#include "demo.hpp"

struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;

HMODULE loadDemosInDll(std::vector<Demo*>& demos, const char* file, const DemoInputs& inputs);

// DLL stuff
struct DemoPrototype
{
    Demo* (*demoBuild)(const DemoInputs& inputs);
    void (*demoDestroy)(Demo*);
    void (*demoUpdateAndRender)(Demo*, const DemoInputs& inputs);
    const char* (*demoName)(Demo*);
};

struct DemoApi
{
    void (*initContext)(ImGuiContext* context, void* (*allocFunc)(size_t, void*), void (*freeFunc)(void*, void*), GLADloadproc loadProc);
    void (*getDemoPrototypes)(int prototypesCapacityInput, int* prototypesCountOutput, DemoPrototype* prototypes);
};

typedef DemoApi(*GetDemoApiFunc)();

// Wrapper
class DemoDllWrapper : public Demo
{
public:
    DemoDllWrapper(DemoPrototype demoPrototype, const DemoInputs& inputs);
    virtual ~DemoDllWrapper() override;
    virtual void UpdateAndRender(const DemoInputs& input) override;
    virtual const char* Name() const override;

private:
    DemoPrototype demoPrototype;
    Demo* demo;
};
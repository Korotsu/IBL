#pragma once

#include "demo_fbo.hpp"

class DemoMipmap : public Demo
{
public:
    DemoMipmap(const DemoInputs& inputs);
    ~DemoMipmap() override;

    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "Mipmap"; }

private:
    DemoFBO demoFBO;

    Camera mainCamera = {};
};
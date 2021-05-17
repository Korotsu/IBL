#pragma once

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

union float2
{
    float e[2];
    struct { float x; float y; };
    struct { float u; float v; };
};

union float3
{
    float3() = default;
    float3(float x, float y, float z)
        : x(x), y(y), z(z)
    {}

    float3 (float2 xy, float z)
        : x(xy.x), y(xy.y), z(z)
    {}

    float e[3];
    struct { float x; float y; float z; };
    struct { float r; float g; float b; };
};

union float4
{
    float4() = default;
    float4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {}

    float4 (float3 xyz, float w)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {}

    struct { float x; float y; float z; float w; };
    struct { float r; float g; float b; float a; };
    float3 xyz;
    float e[4];
};

union mat4
{
    float e[16];
    float4 c[4];
};

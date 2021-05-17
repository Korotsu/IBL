#pragma once

#include "types.hpp"

struct MeshSlice
{
    int start;
    int count;
};

struct VertexDescriptor
{
    int size;
    int positionOffset;
    bool hasUV;
    int uvOffset;
    bool hasNormal;
    int normalOffset;
    bool hasColor;
    int colorOffset;
};

class MeshBuilder
{
public:
    MeshBuilder(const VertexDescriptor& descriptor, void** verticesPtr, int* vertexCount);

    MeshSlice GenTriangle(int* startIndex);
    MeshSlice GenQuad(int* startIndex, float halfWidth, float halfHeight);
    MeshSlice GenIcosphere(int* startIndex, int depth = 2);
    MeshSlice LoadObj(int* startIndex, const char* objFile, const char* mtlDir, float scale = 1.f);

private:
    VertexDescriptor descriptor;
    void** verticesPtr;
    int* vertexCount;

    void* GetDst(int* startIndex, int count);
    void* Grow(int count);

    MeshSlice GenIcosphereFace(int* index, float3 a, float3 b, float3 c, int depth);
};
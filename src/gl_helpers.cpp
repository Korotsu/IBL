
#include <cstdio>
#include <fstream>
#include <vector>

#include <stb_perlin.h>
#include <stb_image.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"

GLuint gl::CreateShader(GLenum type, int sourceCount, const char** sources)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, sourceCount, sources, nullptr);
    glCompileShader(shader);
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, ARRAYSIZE(infoLog), nullptr, infoLog);
        printf("Shader compilation error: %s", infoLog);
    }

    return shader;
}

GLuint gl::CreateProgram(int vsStrsCount, const char** vsStrs, int fsStrsCount, const char** fsStrs)
{
    GLuint program = glCreateProgram();

    const char* shaderHeader = "#version 330\n";

    std::vector<const char*> vertexShaderSources;
    vertexShaderSources.push_back(shaderHeader);
    for (int i = 0; i < vsStrsCount; ++i)
        vertexShaderSources.push_back(vsStrs[i]);

    std::vector<const char*> pixelShaderSources;
    pixelShaderSources.push_back(shaderHeader);
    for (int i = 0; i < fsStrsCount; ++i)
        pixelShaderSources.push_back(fsStrs[i]);

    GLuint vertexShader = gl::CreateShader(GL_VERTEX_SHADER,   (int)vertexShaderSources.size(), vertexShaderSources.data());
    GLuint pixelShader  = gl::CreateShader(GL_FRAGMENT_SHADER, (int)pixelShaderSources.size(),  pixelShaderSources.data());

    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, ARRAYSIZE(infoLog), nullptr, infoLog);
        printf("Program link error: %s", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(pixelShader);

    return program;
}

GLuint gl::CreateBasicProgram(const char* vsStr, const char* fsStr)
{
    return CreateProgram(1, &vsStr, 1, &fsStr);
}

void gl::UploadPerlinNoise(int width, int height, float z, float lacunarity, float gain, float offset, int octaves)
{
    std::vector<float> pixels(width * height);

#if 0
    for (float& pixel : pixels)
        pixel = std::rand() / (float)RAND_MAX;
#else
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            pixels[x + y * width] = stb_perlin_ridge_noise3(x / (float)width, y / (float)height, z, lacunarity, gain, offset, octaves);
        }
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, pixels.data());
}

void gl::UploadImage(const char* file, bool linear)
{
    int width    = 0;
    int height   = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(1);
    void* colors;

    if (linear)
        colors = stbi_loadf(file, &width, &height, &channels, 0);
    else
        colors = stbi_load(file, &width, &height, &channels, 0);

    if (colors == nullptr)
        fprintf(stderr, "Failed to load image '%s'\n", file);
    else
        printf("Load image '%s' (%dx%d %d channels)\n", file, width, height, channels);

    GLenum format;
    switch (channels)
    {
    case 1:          format = GL_RED;  break;
    case 2:          format = GL_RG;   break;
    case 3:          format = GL_RGB;  break;
    case 4: default: format = GL_RGBA; break;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, linear ? GL_FLOAT : GL_UNSIGNED_BYTE, colors);

    stbi_image_free(colors);
}

void gl::SetTextureDefaultParams(bool genMipmap)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (genMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.f);
}

// TODO: Move dds specific stuff somewhere else
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

struct DDSPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
};

struct DDSHeader
{
    uint32_t       size;
    uint32_t       flags;
    uint32_t       height;
    uint32_t       width;
    uint32_t       pitchOrLinearSize;
    uint32_t       depth;
    uint32_t       mipMapCount;
    uint32_t       reserved1[11];
    DDSPixelFormat pixelFormat;
    uint32_t       caps;
    uint32_t       caps2;
    uint32_t       caps3;
    uint32_t       caps4;
    uint32_t       reserved2;
};

void gl::UploadCubemap(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);

    // Parse magic number
    char magic[4] = {};
    file.read((char*)&magic, sizeof(uint32_t));
    if (strncmp(magic, "DDS ", 4) != 0)
    {
        fprintf(stderr, "Not a dds file\n");
        return;
    }

    // Parse header
    DDSHeader header = {};
    file.read((char*)&header, sizeof(DDSHeader));
    file.seekg(header.size - sizeof(DDSHeader), std::ios_base::cur); // Jump to data

    // Abort loading if the texture is not a cubemap...
    if ((header.caps  & DDS_SURFACE_FLAGS_CUBEMAP) == 0 || 
        (header.caps2 & DDS_CUBEMAP_ALLFACES)      == 0)
    {
        fprintf(stderr, "Not a cubemap or not complete\n");
        return;
    }

    // Abort if the format is not supported
    if (header.pixelFormat.rgbBitCount != 128)
    {
        fprintf(stderr, "Only 128 bits (4 floats) textures supported\n");
        return;
    }

    // Parse each texture level and upload them to GPU
    std::vector<float4> buffer(header.width * header.height); // Buffer size for level 0
    for (int i = 0; i < 6; ++i)
    {
        for (uint32_t level = 0; level <= header.mipMapCount; ++level)
        {
            int width  = header.width  >> level;
            int height = header.height >> level;

            file.read((char*)buffer.data(), width * height * sizeof(float4));

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, buffer.data());
        }
    }
}
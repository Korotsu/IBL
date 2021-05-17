#pragma once

#include <cmath>
#include "types.hpp"

namespace calc
{
    const float TAU = 6.28318530717f;

    inline float Cos(float v) { return std::cos(v); }
    inline float Sin(float v) { return std::sin(v); }
    inline float Tan(float v) { return std::tan(v); }

    inline int Modulo(int a, int b)
    {
        int r = a % b;
        return (r >= 0) ? r : r + b;
    }

    inline float Modulo(float a, float b)
    {
        return std::fmod(a, b);
    }

    inline float Sqrt(float a)
    {
        return std::sqrt(a);
    }

    inline float Pow(float a, float exp)
    {
        return std::pow(a, exp);
    }

    inline float3 Pow(float3 a, float exp)
    {
        return { Pow(a.x, exp), Pow(a.y, exp), Pow(a.z, exp) };
    }

    template<typename T>
    inline T Min(T x, T minValue)
    {
        return x < minValue ? x : minValue;
    }

    template<typename T>
    inline T Max(T x, T maxValue)
    {
        return x > maxValue ? x : maxValue;
    }

    template<typename T>
    inline T Clamp(T x, T minValue, T maxValue)
    {
        return Min(Max(x, minValue), maxValue);
    }

    inline float Floor(float x)
    {
        return std::floor(x);
    }

    template<typename T>
    inline T Lerp(T a, T b, float t)
    {
        return (1.f - t) * a + t * b;
    }

    inline float ToRadians(float degrees) { return degrees * TAU / 360.f; }
}

inline float2 operator-(float2 a) { return { -a.x, -a.y }; }
inline float2 operator+(float2 a, float2 b) { return { a.x + b.x, a.y + b.y }; }
inline float2 operator-(float2 a, float2 b) { return { a.x - b.x, a.y - b.y }; }
inline float2 operator*(float2 a, float2 b) { return { a.x * b.x, a.y * b.y }; }
inline float2 operator/(float2 a, float2 b) { return { a.x / b.x, a.y / b.y }; }

inline float2 operator+(float2 a, float b) { return { a.x + b, a.y + b }; }
inline float2 operator-(float2 a, float b) { return { a.x - b, a.y - b }; }
inline float2 operator*(float2 a, float b) { return { a.x * b, a.y * b }; }
inline float2 operator/(float2 a, float b) { return { a.x / b, a.y / b }; }

inline float2& operator+=(float2& a, float2 b) { a = a + b; return a; }
inline float2& operator-=(float2& a, float2 b) { a = a - b; return a; }
inline float2& operator*=(float2& a, float2 b) { a = a * b; return a; }
inline float2& operator/=(float2& a, float2 b) { a = a / b; return a; }

inline float2& operator+=(float2& a, float b) { a = a + b; return a; }
inline float2& operator-=(float2& a, float b) { a = a - b; return a; }
inline float2& operator*=(float2& a, float b) { a = a * b; return a; }
inline float2& operator/=(float2& a, float b) { a = a / b; return a; }

inline float3 operator-(float3 a) { return { -a.x, -a.y, -a.z }; }
inline float3 operator+(float3 a, float3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline float3 operator-(float3 a, float3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline float3 operator*(float3 a, float3 b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline float3 operator/(float3 a, float3 b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }

inline float3 operator+(float3 a, float b) { return { a.x + b, a.y + b, a.z + b }; }
inline float3 operator-(float3 a, float b) { return { a.x - b, a.y - b, a.z - b }; }
inline float3 operator*(float3 a, float b) { return { a.x * b, a.y * b, a.z * b }; }
inline float3 operator/(float3 a, float b) { return { a.x / b, a.y / b, a.z / b }; }

inline float3& operator+=(float3& a, float3 b) { a = a + b; return a; }
inline float3& operator-=(float3& a, float3 b) { a = a - b; return a; }
inline float3& operator*=(float3& a, float3 b) { a = a * b; return a; }
inline float3& operator/=(float3& a, float3 b) { a = a / b; return a; }

inline float3& operator+=(float3& a, float b) { a = a + b; return a; }
inline float3& operator-=(float3& a, float b) { a = a - b; return a; }
inline float3& operator*=(float3& a, float b) { a = a * b; return a; }
inline float3& operator/=(float3& a, float b) { a = a / b; return a; }

inline float v3Length(float3 v)
{
    return calc::Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float3 v3Normalize(float3 v)
{
    return v / v3Length(v);
}

inline mat4 mat4Identity()
{
    return
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

inline mat4 mat4Scale(float s)
{
    return
    {
        s, 0.f, 0.f, 0.f,
        0.f,   s, 0.f, 0.f,
        0.f, 0.f,   s, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

inline mat4 mat4Scale(float3 s)
{
    return
    {
        s.x, 0.f, 0.f, 0.f,
        0.f, s.y, 0.f, 0.f,
        0.f, 0.f, s.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

inline mat4 mat4Translate(float3 t)
{
    return
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        t.x, t.y, t.z, 1.f
    };
}

inline mat4 mat4RotateX(float radians)
{
    float c = calc::Cos(radians);
    float s = calc::Sin(radians);
    return
    {
        1.f, 0.f, 0.f, 0.f,
        0.f,   c,   s, 0.f,
        0.f,  -s,   c, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
}

inline mat4 mat4RotateY(float radians)
{
    float c = calc::Cos(radians);
    float s = calc::Sin(radians);
    return
    {
          c, 0.f,  -s, 0.f,
        0.f, 1.f, 0.f, 0.f,
          s, 0.f,   c, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
}

inline mat4 mat4Transpose(const mat4& m)
{
    return {
        m.c[0].e[0], m.c[1].e[0], m.c[2].e[0], m.c[3].e[0],
        m.c[0].e[1], m.c[1].e[1], m.c[2].e[1], m.c[3].e[1],
        m.c[0].e[2], m.c[1].e[2], m.c[2].e[2], m.c[3].e[2],
        m.c[0].e[3], m.c[1].e[3], m.c[2].e[3], m.c[3].e[3],
    };
}

inline mat4 operator*(const mat4& a, const mat4& b)
{
    mat4 res = {};
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            for (int k = 0; k < 4; ++k)
                res.c[c].e[r] += a.c[k].e[r] * b.c[c].e[k];
    return res;
}

inline mat4& operator*=(mat4& a, const mat4& b) { a = a * b; return a; }

inline float4 operator*(const mat4& m, float4 v)
{
    float4 r;
    r.x = v.x * m.c[0].e[0] + v.y * m.c[1].e[0] + v.z * m.c[2].e[0] + v.w * m.c[3].e[0];
    r.y = v.x * m.c[0].e[1] + v.y * m.c[1].e[1] + v.z * m.c[2].e[1] + v.w * m.c[3].e[1];
    r.z = v.x * m.c[0].e[2] + v.y * m.c[1].e[2] + v.z * m.c[2].e[2] + v.w * m.c[3].e[2];
    r.w = v.x * m.c[0].e[3] + v.y * m.c[1].e[3] + v.z * m.c[2].e[3] + v.w * m.c[3].e[3];
    return r;
}

inline mat4 mat4Perspective(float yFov, float aspect, float n, float f)
{
    mat4 m;
    float const a = 1.f / calc::Tan(yFov / 2.f);

    m.c[0].e[0] = a / aspect;
    m.c[0].e[1] = 0.f;
    m.c[0].e[2] = 0.f;
    m.c[0].e[3] = 0.f;

    m.c[1].e[0] = 0.f;
    m.c[1].e[1] = a;
    m.c[1].e[2] = 0.f;
    m.c[1].e[3] = 0.f;

    m.c[2].e[0] = 0.f;
    m.c[2].e[1] = 0.f;
    m.c[2].e[2] = -((f + n) / (f - n));
    m.c[2].e[3] = -1.f;

    m.c[3].e[0] = 0.f;
    m.c[3].e[1] = 0.f;
    m.c[3].e[2] = -((2.f * f * n) / (f - n));
    m.c[3].e[3] = 0.f;

    return m;
}

#ifdef USE_CALC_EXT
#include "calc_ext.hpp"
#endif

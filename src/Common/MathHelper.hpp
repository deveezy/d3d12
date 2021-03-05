#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <Common/defines.hpp>

class MathHelper
{
public:
    // Returns random float int [0, 1)
    static f32 RandF()
    {
        return (f32)(rand() / (float)RAND_MAX);
    }

    // Returns random float in [a, b)
    static f32 RandF(f32 a, f32 b)
    {
        return a + RandF() * (b - a);
    }

    static i32 Rand(i32 a, i32 b)
    {
        return a + rand() % ((b - a) + 1);
    }

    template <typename T>
    static T Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    template <typename T>
    static T Max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }

    template <typename T>
    static T Lerp(const T& a, const T& b, f32)
    {
        return a + (b - a) * t;
    }
    
    template <typename T>
    static T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    // Returns the polar angle of the point (x, y) in [0, 2*PI);
    static f32 AngleFromXY(f32 x, f32 y);

    static DirectX::XMVECTOR SphericalToCartesian(f32 radius, f32 theta, f32 phi)
    {
        f32 r1 = radius * sinf(phi) * cosf(theta);
        f32 r2 = radius * cosf(phi);
        f32 r3 = radius * sinf(phi) * sinf(theta);

        return DirectX::XMVectorSet(r1, r2, r3, 1.f);
    }

    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M)
    {
        // Inverse-transpose is just applied to normals. So zero out
        // translation row so that it doesn't get into out inverse-transpose
        // calculation-- we don't want the inverse-transpose of the translation.
        DirectX::XMMATRIX A = M;
        A.r[3] = DirectX::XMVectorSet(.0f, .0f, .0f, 1.f);

        DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(A);
        DirectX::XMMATRIX inv = DirectX::XMMatrixInverse(&det, A);
        return DirectX::XMMatrixTranspose(inv);
    }

    static DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 I(
            1.f, .0f, .0f, .0f,
            .0f, 1.f, .0f, .0f,
            .0f, .0f, 1.f, .0f,
            .0f, .0f, .0f, 1.f
        );
        return I;
    }

    static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);

    inline static const f32 Infitinity = FLT_MAX;
    inline static const f32 Pi         = 3.1415926535f;
};
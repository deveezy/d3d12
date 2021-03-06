#pragma once

#include <vector>
#include <DirectXMath.h>
#include <Common/defines.hpp>

namespace DX = DirectX;

class Waves
{
public:
    Waves(i32 m, i32 n, f32 dx, f32 dt, f32 speed, f32 damping);
    Waves(const Waves&) = delete;
    Waves& operator=(const Waves&) = delete;
    ~Waves();

    i32 RowCount() const;
    i32 ColumnCount() const;
    i32 VertexCount() const;
    i32 TriangleCount() const;
    f32 Width() const;
    f32 Depth() const;

    // Returns the solution at the ith grid point.
    const DX::XMFLOAT3& Position(i32 i) const { return mCurrSolution[i]; }

    // Returns the solution normal at the ith grid point.
    const DX::XMFLOAT3& Normal(i32 i) const { return mNormals[i]; }

    // Returns the unit tangent vector at the ith grid point in the local x-axis direction.
    const DX::XMFLOAT3& TangentX(i32 i) const { return mTangentX[i]; }

    void Update(f32 dt);
    void Disturb(i32 i, i32 j, f32 magnitude);

private:
    i32 mNumRows = 0;
    i32 mNumCols = 0;

    i32 mVertexCount = 0;
    i32 mTriangleCount = 0;

    // Simulation constants we can precompute.
    f32 mK1 = .0f;
    f32 mK2 = .0f;
    f32 mK3 = .0f;

    f32 mTimeStep = .0f;
    f32 mSpatialStep = .0f;

    std::vector<DX::XMFLOAT3> mPrevSolution;
    std::vector<DX::XMFLOAT3> mCurrSolution;
    std::vector<DX::XMFLOAT3> mNormals;
    std::vector<DX::XMFLOAT3> mTangentX;
};


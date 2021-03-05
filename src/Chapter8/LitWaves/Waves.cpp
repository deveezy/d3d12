#include "Chapter8/LitWaves/Waves.hpp"
#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace DirectX;

Waves::Waves(i32 m, i32 n, f32 dx, f32 dt, f32 speed, f32 damping)
{
    mNumRows = m;
    mNumCols = n;

    mVertexCount = m*n;
    mTriangleCount = (m - 1)*(n - 1) * 2;

    mTimeStep = dt;
    mSpatialStep = dx;

    f32 d = damping*dt + 2.0f;
    f32 e = (speed*speed)*(dt*dt) / (dx*dx);
    mK1 = (damping*dt - 2.0f) / d;
    mK2 = (4.0f - 8.0f*e) / d;
    mK3 = (2.0f*e) / d;

    mPrevSolution.resize(m*n);
    mCurrSolution.resize(m*n);
    mNormals.resize(m*n);
    mTangentX.resize(m*n);

    // Generate grid vertices in system memory.

    f32 halfWidth = (n - 1)*dx*0.5f;
    f32 halfDepth = (m - 1)*dx*0.5f;
    for(i32 i = 0; i < m; ++i)
    {
        f32 z = halfDepth - i*dx;
        for(i32 j = 0; j < n; ++j)
        {
            f32 x = -halfWidth + j*dx;

            mPrevSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
            mCurrSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
            mNormals[i*n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            mTangentX[i*n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
        }
    }
}

Waves::~Waves()
{
}

i32 Waves::RowCount()const
{
	return mNumRows;
}

i32 Waves::ColumnCount()const
{
	return mNumCols;
}

i32 Waves::VertexCount()const
{
	return mVertexCount;
}

i32 Waves::TriangleCount()const
{
	return mTriangleCount;
}

f32 Waves::Width()const
{
	return mNumCols*mSpatialStep;
}

f32 Waves::Depth()const
{
	return mNumRows*mSpatialStep;
}

void Waves::Update(f32 dt)
{
	static f32 t = 0;

	// Accumulate time.
	t += dt;

	// Only update the simulation at the specified time step.
	if( t >= mTimeStep )
	{
		// Only update i32erior poi32s; we use zero boundary conditions.
		concurrency::parallel_for(1, mNumRows - 1, [this](i32 i)
		//for(i32 i = 1; i < mNumRows-1; ++i)
		{
			for(i32 j = 1; j < mNumCols-1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.

				mPrevSolution[i*mNumCols+j].y = 
					mK1*mPrevSolution[i*mNumCols+j].y +
					mK2*mCurrSolution[i*mNumCols+j].y +
					mK3*(mCurrSolution[(i+1)*mNumCols+j].y + 
					     mCurrSolution[(i-1)*mNumCols+j].y + 
					     mCurrSolution[i*mNumCols+j+1].y + 
						 mCurrSolution[i*mNumCols+j-1].y);
			}
		});

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(mPrevSolution, mCurrSolution);

		t = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		concurrency::parallel_for(1, mNumRows - 1, [this](i32 i)
		//for(i32 i = 1; i < mNumRows - 1; ++i)
		{
			for(i32 j = 1; j < mNumCols-1; ++j)
			{
				f32 l = mCurrSolution[i*mNumCols+j-1].y;
				f32 r = mCurrSolution[i*mNumCols+j+1].y;
				f32 t = mCurrSolution[(i-1)*mNumCols+j].y;
				f32 b = mCurrSolution[(i+1)*mNumCols+j].y;
				mNormals[i*mNumCols+j].x = -r+l;
				mNormals[i*mNumCols+j].y = 2.0f*mSpatialStep;
				mNormals[i*mNumCols+j].z = b-t;

				XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[i*mNumCols+j]));
				XMStoreFloat3(&mNormals[i*mNumCols+j], n);

				mTangentX[i*mNumCols+j] = XMFLOAT3(2.0f*mSpatialStep, r-l, 0.0f);
				XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[i*mNumCols+j]));
				XMStoreFloat3(&mTangentX[i*mNumCols+j], T);
			}
		});
	}
}

void Waves::Disturb(i32 i, i32 j, f32 magnitude)
{
	// Don't disturb boundaries.
	assert(i > 1 && i < mNumRows-2);
	assert(j > 1 && j < mNumCols-2);

	f32 halfMag = 0.5f*magnitude;

	// Disturb the ijth vertex height and its neighbors.
	mCurrSolution[i*mNumCols+j].y     += magnitude;
	mCurrSolution[i*mNumCols+j+1].y   += halfMag;
	mCurrSolution[i*mNumCols+j-1].y   += halfMag;
	mCurrSolution[(i+1)*mNumCols+j].y += halfMag;
	mCurrSolution[(i-1)*mNumCols+j].y += halfMag;
}
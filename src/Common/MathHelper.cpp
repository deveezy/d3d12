#include <Common/MathHelper.hpp>

using namespace DirectX;

f32 MathHelper::AngleFromXY(f32 x, f32 y)
{
    f32 theta = .0f;
    // Quadrant I or IV
    if (x >= .0f)
    {
        // If x = 0, then atanf(y/x) = +pi/2 if y > 0
        //                atanf(y/x) = -pi/2 if y < 0
        theta = atanf(y/x); // in [-pi/2, +pi/2]

        if (theta <.0f)
        {
            theta += 2.f * Pi; // in [0, 2*pi]
        }
    }
    // Quadrant II or III
    else
    {
        theta = atanf(y/x) + Pi; // in [0, 2*pi].
    }
    return theta;
}

XMVECTOR MathHelper::RandUnitVec3()
{
    XMVECTOR One  = XMVectorSet(1.f, 1.f, 1.f, 1.f);
    XMVECTOR Zero = XMVectorZero();

    // Keep trying until we get a point on/in the hemisphere.
    while (true)
    {
        // Generate random point in the cube [-1, 1] ^ 3.
        XMVECTOR v = XMVectorSet(
            MathHelper::RandF(-1.f, 1.f), 
            MathHelper::RandF(-1.f, 1.f), 
            MathHelper::RandF(-1.f, 1.f), .0f);

        // Ignore points outside the unit sphere in order to get an even distribution
        // over the unit sphere. Otherwise points will clump more on the sphere near
        // the corners of the cube.
        
        if (XMVector3Greater(XMVector3LengthSq(v), One))
        {
            continue;
        }
        return XMVector3Normalize(v);
    }
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(DirectX::XMVECTOR n) 
{
    XMVECTOR One  = XMVectorSet(1.f, 1.f, 1.f, 1.f);
    XMVECTOR Zero = XMVectorZero();

    // Keep trying until we get a point on/in the hemisphere.
    while (true)
    {
        // Generate random point in the cube [-1, 1] ^ 3.
        XMVECTOR v = XMVectorSet(
            MathHelper::RandF(-1.f, 1.f), 
            MathHelper::RandF(-1.f, 1.f), 
            MathHelper::RandF(-1.f, 1.f), .0f);

        // Ignore points outside the unit sphere in order to get an even distribution
        // over the unit sphere. Otherwise points will clump more on the sphere near
        // the corners of the cube.
        if (XMVector3Greater(XMVector3LengthSq(v), One))
        {
            continue;
        }
        // Ignore points in the bottm hemishpere.
        if (XMVector3Less(XMVector3Dot(n, v), Zero))
        {
            continue;
        }
        return XMVector3Normalize(v);
    }
}


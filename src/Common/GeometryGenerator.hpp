//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//**

#pragma once

#include <Common/defines.hpp>
#include <DirectXMath.h>
#include <vector>

namespace DX = DirectX;

class GeometryGenerator
{
public:
    struct Vertex
    {
        Vertex() {}
        Vertex(const DX::XMFLOAT3& p,
               const DX::XMFLOAT3& n,
               const DX::XMFLOAT3& t,
               const DX::XMFLOAT2& uv) 
        : Position(p), Normal(n), TangentU(t), TexC(uv) {}

        Vertex(f32 px, f32 py, f32 pz,
               f32 nx, f32 ny, f32 nz,
               f32 tx, f32 ty, f32 tz,
               f32 u,  f32 v) 
        : Position(px, py, pz), Normal(nx, ny, nz), TangentU(tx, ty, tz), TexC(u, v) {}
    
        DX::XMFLOAT3 Position;
        DX::XMFLOAT3 Normal;
        DX::XMFLOAT3 TangentU;
        DX::XMFLOAT2 TexC;
    };

    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<u32>    Indices32;
        std::vector<u16>& GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); ++i)
                {
                    mIndices16[i] = static_cast<u16>(Indices32[i]);
                }
            }
            return mIndices16;
        }

    private:
        std::vector<u16>    mIndices16;
    };

    // Creates a box centered at the origin with the given dimensions, where each 
    // face has m rows and n columns of vertices. 
    MeshData CreateBox(f32 width, f32 height, f32 depth, u32 numSubdivisions);

    // Creates a sphere centered at the origin with the given radius. The
    // slices and stacks parameters control the degree of tesselation.
    MeshData CreateSphere(f32 radius, u32 sliceCount, u32 stackCount);

    // Creates a geosphere centered at the origin with the given radius. The
    // depth controls the level of tesselation.
    MeshData CreateGeosphere(f32 radius, u32 numSubdivisions);

    // Creates a cylinder parallel to the y-axis, and centered about the origin.
    // The bottom and top radius can vary to form various cone shapes rather than true
    // cylinders. The slices and stacks parameters control the degree of tesselation.
    MeshData CreateCylinder(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount);

    // Creates an mxn grid in the xz-plane with rows and n columns, centered
    // at the origin with the specified width and depth.
    MeshData CreateGrid(f32 width, f32 depth, u32 m, u32 n);

    // Creates a quad aligned with the screen. This is useful for postprocessing and screen effects.
    MeshData CreateQuad(f32 x, f32 y, f32 w, f32 h, f32 depth);

private:
    void Subdivide(MeshData& meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
    void BuildCylinderTopCap(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount, MeshData& meshData);
    void BuildCylinderBottomCap(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount, MeshData& meshData);
};
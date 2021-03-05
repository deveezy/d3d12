#include <Common/GeometryGenerator.hpp>
#include <algorithm>

using namespace DirectX;

GeometryGenerator::MeshData GeometryGenerator::CreateBox(f32 width, f32 height, f32 depth, u32 numSubdivisions) 
{
    MeshData meshData;

    // Create the vertices.
    Vertex v[24];

    f32 w2 = .5f * width; 
    f32 h2 = .5f * height;
    f32 d2 = .5f * depth;

    // Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    meshData.Vertices.assign(&v[0], &v[24]);

    // Create the indices.
    u32 indices[36];
    
    // Fill in the front face index data
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 0; indices[4] = 2; indices[5] = 3;

	// Findicesll indicesn the back face indicesndex data
	indices[6] = 4; indices[7]  = 5; indices[8]  = 6;
	indices[9] = 4; indices[10] = 6; indices[11] = 7;

	// Findicesll indicesn the top face indicesndex data
	indices[12] = 8; indices[13] =  9; indices[14] = 10;
	indices[15] = 8; indices[16] = 10; indices[17] = 11;

	// Findicesll indicesn the bottom face indicesndex data
	indices[18] = 12; indices[19] = 13; indices[20] = 14;
	indices[21] = 12; indices[22] = 14; indices[23] = 15;

	// Findicesll indicesn the left face indicesndex data
	indices[24] = 16; indices[25] = 17; indices[26] = 18;
	indices[27] = 16; indices[28] = 18; indices[29] = 19;

	// Findicesll indicesn the rindicesght face indicesndex data
	indices[30] = 20; indices[31] = 21; indices[32] = 22;
	indices[33] = 20; indices[34] = 22; indices[35] = 23;

	meshData.Indices32.assign(&indices[0], &indices[36]);

    // Put a cap on the number of subdivisions.
    numSubdivisions = std::min<u32>(numSubdivisions, 6u);

    for (u32 i = 0; i < numSubdivisions; ++i)
    {
        Subdivide(meshData);
    }

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(f32 radius, u32 sliceCount, u32 stackCount) 
{
    MeshData meshData; 

    // Compute the vertices stating at the top pole and moving down the stacks.

    // Poles: note that there will be texture coordinate distortion as there is
    // not a unique point on the texture map to assign to the pole when mapping
    // a rectangular texture onto the sphere.
    Vertex topVertex(.0f, +radius, .0f, .0f, +1.f, .0f, 1.f, .0f, .0f, .0f, .0f);
    Vertex bottomVertex(.0f, -radius, .0f, .0f, -1.f, .0f, 1.f, .0f, .0f, .0f, 1.f);

    meshData.Vertices.push_back(topVertex);

    f32 phiStep = DX::XM_PI / stackCount;
    f32 thetaStep = 2.f * DX::XM_PI / sliceCount;

    // Compute vertices for each stack ring (do not count the poles as rings.)
    for (u32 i = 1; i <= stackCount - 1; ++i)
    {
        f32 phi = i * phiStep;

        // Vertices of ring.
        for (u32 j = 0; j <= sliceCount; ++j)
        {
            f32 theta = j * thetaStep;
            Vertex v;

            // spherical to cartesian
            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * cosf(theta);

            // Partial deriviate of P with respect to theta
            v.TangentU.x = -radius * sinf(phi) * sinf(theta);
            v.TangentU.y = 0;
            v.TangentU.z = +radius * sinf(phi) * cosf(theta);

            DX::XMVECTOR T = DX::XMLoadFloat3(&v.TangentU);
            DX::XMStoreFloat3(&v.TangentU, DX::XMVector3Normalize(T));

            DX::XMVECTOR p = DX::XMLoadFloat3(&v.Position);
            DX::XMStoreFloat3(&v.Normal, DX::XMVector3Normalize(p));

            v.TexC.x = theta / DX::XM_PI;
            v.TexC.y = phi   / DX::XM_PI;

            meshData.Vertices.push_back(v);
        }
    }
    meshData.Vertices.push_back(bottomVertex);

    // Compute indices for top stack. The top stack was written first to the vertex buffer
    // and connects the top pole to the first ring.
    for (u32 i = 0; i <= sliceCount; ++i)
    {
        meshData.Indices32.push_back(0);
        meshData.Indices32.push_back(i + 1);
        meshData.Indices32.push_back(i);
    }

    // Compute indices for inner stacks (not connected to poles).

    // Offset the indices to the index of the first vertex in the first ring.
    // This is just skipping the top pole vertex.
    u32 baseIndex = 1;
    u32 ringVertexCount = sliceCount + 1;
    for (u32 i = 0; i < stackCount - 2; ++i)
    {
        for (u32 j = 0; j < sliceCount; ++j)
        {
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // Compute indices for bottom stack. The bottom stack was written last to the vertex buffer
    // and connects the bottom pole to the bottom ring.
    
    // South pole vertex was added last.
    u32 southPoleIndex = (u32)meshData.Vertices.size() - 1;

    // Offset the indices to the index of the first vertex in the last ring.
    baseIndex = southPoleIndex - ringVertexCount;

    for (u32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(southPoleIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(f32 radius, u32 numSubdivisions) 
{
    MeshData meshData;

    // Put a cap on the number of subdivisions.
    numSubdivisions = std::min<u32>(numSubdivisions, 6u);

    // Approximate a sphere by tesselating an icosahedron.
    const f32 X = 0.525731f; 
	const f32 Z = 0.850651f;

    DX::XMFLOAT3 pos[12] =
    {
        DX::XMFLOAT3(-X, 0.0f, Z),  DX::XMFLOAT3(X, 0.0f, Z),  
		DX::XMFLOAT3(-X, 0.0f, -Z), DX::XMFLOAT3(X, 0.0f, -Z),    
		DX::XMFLOAT3(0.0f, Z, X),   DX::XMFLOAT3(0.0f, Z, -X), 
		DX::XMFLOAT3(0.0f, -Z, X),  DX::XMFLOAT3(0.0f, -Z, -X),    
		DX::XMFLOAT3(Z, X, 0.0f),   DX::XMFLOAT3(-Z, X, 0.0f), 
		DX::XMFLOAT3(Z, -X, 0.0f),  DX::XMFLOAT3(-Z, -X, 0.0f)
    };

    u32 k[60] =
    {
        1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,    
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,    
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0, 
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7 
    };
    
    meshData.Vertices.resize(12);
    meshData.Indices32.assign(&k[0], &k[60]);

    for (u32 i = 0; i < 12; ++i)
        meshData.Vertices[i].Position = pos[i];

    for (u32 i = 0; i < numSubdivisions; ++i)
        Subdivide(meshData);
    
    // Project vertices onto sphere and scale.
    for (u32 i = 0; i < meshData.Vertices.size(); ++i)
    {
        // Project onto unit sphere.
		DX::XMVECTOR n = DX::XMVector3Normalize(DX::XMLoadFloat3(&meshData.Vertices[i].Position));

		// Project onto sphere.
		DX::XMVECTOR p = radius * n;

		DX::XMStoreFloat3(&meshData.Vertices[i].Position, p);
		DX::XMStoreFloat3(&meshData.Vertices[i].Normal, n);

        // Derive texture coordinates from spherical coordinates.
        float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);

        // Put in [0, 2pi].
        if(theta < 0.0f)
            theta += DX::XM_2PI;

		float phi = acosf(meshData.Vertices[i].Position.y / radius);

		meshData.Vertices[i].TexC.x = theta / DX::XM_2PI;
		meshData.Vertices[i].TexC.y = phi / DX::XM_PI;

		// Partial derivative of P with respect to theta
		meshData.Vertices[i].TangentU.x = -radius*sinf(phi)*sinf(theta);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = +radius*sinf(phi)*cosf(theta);

		DX::XMVECTOR T = DX::XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, DX::XMVector3Normalize(T));
    }

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount) 
{
    MeshData meshData;

    // Build Stacks
    f32 stackHeight = height / stackCount;

    // Amount to increment radius as we move up each stack level from bottom to top.
    f32 radiusStep = (topRadius - bottomRadius) / stackCount;
    u32 ringCount = stackCount + 1;

    // Compute vertices for each stack ring starting at the bottom and moving up.
    for (u32 i = 0; i < ringCount; ++i)
    {
        f32 y = -.5f * height + i * stackHeight; // 0.5 to center local space of cylinder
        f32 r = bottomRadius + i * radiusStep;

        // vertices of ring
        f32 dTheta = 2.f * DX::XM_PI / sliceCount;
        for (u32 j = 0; j <= sliceCount; ++j)
        {
            Vertex vertex;
            f32 c = cosf(j * dTheta);
            f32 s = sinf(j * dTheta);

            vertex.Position = DX::XMFLOAT3(r*c, y, r*s);
            vertex.TexC.x = (f32)j / sliceCount;
            vertex.TexC.y = 1.f - (f32)i / stackCount;

            // Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

            // This is unit length.
            vertex.TangentU = DX::XMFLOAT3(-s, .0f, c);

            f32 dr = bottomRadius - topRadius;
            DX::XMFLOAT3 bitangent(dr*c, -height, dr*s);

            DX::XMVECTOR T = DX::XMLoadFloat3(&vertex.TangentU);
            DX::XMVECTOR B = DX::XMLoadFloat3(&bitangent);
			DX::XMVECTOR N = DX::XMVector3Normalize(DX::XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);

			meshData.Vertices.push_back(vertex);
        }
    }

    // Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	u32 ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for(u32 i = 0; i < stackCount; ++i)
	{
		for(u32 j = 0; j < sliceCount; ++j)
		{
			meshData.Indices32.push_back(i * ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);

			meshData.Indices32.push_back(i*ringVertexCount + j);
			meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.Indices32.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap   (bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

    return meshData;
}

void GeometryGenerator::Subdivide(MeshData& meshData) 
{
    // Save a copy of the input geometry.
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	u32 numTris = (u32)inputCopy.Indices32.size() / 3;
	for(u32 i = 0; i < numTris; ++i)
	{
		Vertex v0 = inputCopy.Vertices[ inputCopy.Indices32[i * 3 + 0] ];
		Vertex v1 = inputCopy.Vertices[ inputCopy.Indices32[i * 3 + 1 ] ];
		Vertex v2 = inputCopy.Vertices[ inputCopy.Indices32[i * 3 + 2] ];

		// Generate the midpoints.

        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

		// Add new geometry.
		meshData.Vertices.push_back(v0); // 0
		meshData.Vertices.push_back(v1); // 1
		meshData.Vertices.push_back(v2); // 2
		meshData.Vertices.push_back(m0); // 3
		meshData.Vertices.push_back(m1); // 4
		meshData.Vertices.push_back(m2); // 5
 
		meshData.Indices32.push_back(i * 6 + 0);
		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 5);

		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 4);
		meshData.Indices32.push_back(i * 6 + 5);

		meshData.Indices32.push_back(i * 6 + 5);
		meshData.Indices32.push_back(i * 6 + 4);
		meshData.Indices32.push_back(i * 6 + 2);

		meshData.Indices32.push_back(i * 6 + 3);
		meshData.Indices32.push_back(i * 6 + 1);
		meshData.Indices32.push_back(i * 6 + 4);
	} 
}

void GeometryGenerator::BuildCylinderTopCap(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount, MeshData& meshData) 
{
    u32 baseIndex = (u32)meshData.Vertices.size();
    f32 y = .5f * height;
    f32 dTheta = 2.f * DX::XM_PI / sliceCount;

    // Duplicate cap ring vertices because the texture coordinates and normals differ.
    for (u32 i = 0; i <= sliceCount; ++i)
    {
        f32 x = topRadius * cosf(i * dTheta);
        f32 z = topRadius * sinf(i * dTheta);

        // Scale down by the height to try and make top cap texture coord area
        // proportional to base.
        f32 u = x / height + .5f;
        f32 v = z / height + .5f;

        meshData.Vertices.push_back(Vertex(x, y, z, .0f, 1.f, .0f, 1.f, .0f, .0f, u, v));
    }

    // Cap center vertex.
    meshData.Vertices.push_back(Vertex(.0f, y, .0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    // Index of center vertex.
    u32 centerIndex = (u32)meshData.Vertices.size() - 1;

    for (u32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i + 1);
        meshData.Indices32.push_back(baseIndex + i);
    }
}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1)
{
    DX::XMVECTOR p0 = DX::XMLoadFloat3(&v0.Position);
    DX::XMVECTOR p1 = DX::XMLoadFloat3(&v1.Position);

    DX::XMVECTOR n0 = DX::XMLoadFloat3(&v0.Normal);
    DX::XMVECTOR n1 = DX::XMLoadFloat3(&v1.Normal);

    DX::XMVECTOR tan0 = DX::XMLoadFloat3(&v0.TangentU);
    DX::XMVECTOR tan1 = DX::XMLoadFloat3(&v1.TangentU);

    DX::XMVECTOR tex0 = DX::XMLoadFloat2(&v0.TexC);
    DX::XMVECTOR tex1 = DX::XMLoadFloat2(&v1.TexC);

    // Compute the midpoints of all the attributes.  Vectors need to be normalized
    // since linear interpolating can make them not unit length.  
    DX::XMVECTOR pos = 0.5f * (p0 + p1);
    DX::XMVECTOR normal = DX::XMVector3Normalize(0.5f * (n0 + n1));
    DX::XMVECTOR tangent = DX::XMVector3Normalize(0.5f * (tan0 + tan1));
    DX::XMVECTOR tex = 0.5f * (tex0 + tex1);

    Vertex v;
    XMStoreFloat3(&v.Position, pos);
    XMStoreFloat3(&v.Normal, normal);
    XMStoreFloat3(&v.TangentU, tangent);
    XMStoreFloat2(&v.TexC, tex);

    return v;
}

void GeometryGenerator::BuildCylinderBottomCap(f32 bottomRadius, f32 topRadius, f32 height, u32 sliceCount, u32 stackCount, MeshData& meshData) 
{
    // Build bottom cap.
    u32 baseIndex = (u32)meshData.Vertices.size();
    f32 y = -.5f * height;

    // vertices of ring.
    f32 dTheta = 2.f * DX::XM_PI / sliceCount;
    for (u32 i = 0; i <= sliceCount; ++i)
    {
        f32 x = bottomRadius * cosf(i * dTheta);
        f32 z = bottomRadius * sinf(i * dTheta);

        // Scale down by the height to try and make top cap texture coord area
        // proportional to base.
        f32 u = x / height + .5f;
        f32 v = z / height + .5f;

        meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
    }

    // Cap center vertex.
    meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

    // Cache the index of center vertex.
    u32 centerIndex = (u32)meshData.Vertices.size() - 1;

    for (u32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(f32 width, f32 depth, u32 m, u32 n)
{
    MeshData meshData;

	u32 vertexCount = m * n;
	u32 faceCount   = (m - 1) * (n - 1) * 2;

	// Create the vertices.
	f32 halfWidth = 0.5f * width;
	f32 halfDepth = 0.5f * depth;

	f32 dx = width / (n - 1);
	f32 dz = depth / (m - 1);

	f32 du = 1.0f / (n - 1);
	f32 dv = 1.0f / (m - 1);

	meshData.Vertices.resize(vertexCount);
	for(u32 i = 0; i < m; ++i)
	{
		f32 z = halfDepth - i * dz;
		for(u32 j = 0; j < n; ++j)
		{
			f32 x = -halfWidth + j * dx;

			meshData.Vertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i * n + j].Normal   = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i * n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			meshData.Vertices[i * n + j].TexC.x = j * du;
			meshData.Vertices[i * n + j].TexC.y = i * dv;
		}
	}
 
	// Create the indices.

	meshData.Indices32.resize(faceCount * 3); // 3 indices per face

	// Iterate over each quad and compute indices.
	u32 k = 0;
	for(u32 i = 0; i < m-1; ++i)
	{
		for(u32 j = 0; j < n-1; ++j)
		{
			meshData.Indices32[k]   = i * n + j;
			meshData.Indices32[k + 1] = i * n + j + 1;
			meshData.Indices32[k + 2] = (i + 1) * n + j;

			meshData.Indices32[k + 3] = (i + 1) * n + j;
			meshData.Indices32[k + 4] = i * n + j + 1;
			meshData.Indices32[k + 5] = (i + 1) * n + j + 1;
            
			k += 6; // next quad
		}
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
    MeshData meshData;

	meshData.Vertices.resize(4);
	meshData.Indices32.resize(6);

	// Position coordinates specified in NDC space.
	meshData.Vertices[0] = Vertex(
        x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	meshData.Vertices[1] = Vertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	meshData.Vertices[2] = Vertex(
		x+w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	meshData.Vertices[3] = Vertex(
		x+w, y-h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	meshData.Indices32[0] = 0;
	meshData.Indices32[1] = 1;
	meshData.Indices32[2] = 2;

	meshData.Indices32[3] = 0;
	meshData.Indices32[4] = 2;
	meshData.Indices32[5] = 3;

    return meshData;
}

#include <Chapter8/Exercises/3/ShapesApp.hpp>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        ShapesApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

ShapesApp::ShapesApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

ShapesApp::~ShapesApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool ShapesApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void ShapesApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    DX::XMMATRIX P = DX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    DX::XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::Update(const GameTimer& gt) 
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE hEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, hEvent))
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    UpdateMainPassCB(gt);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = DX::XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = DX::XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void ShapesApp::OnKeyboardInput(const GameTimer& gt)
{
    const float dt = gt.DeltaTime();

	if(GetAsyncKeyState(VK_LEFT) & 0x8000)
		mSunTheta -= 1.0f * dt;

	if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
		mSunTheta += 1.0f * dt;

	if(GetAsyncKeyState(VK_UP) & 0x8000)
		mSunPhi -= 1.0f * dt;

	if(GetAsyncKeyState(VK_DOWN) & 0x8000)
		mSunPhi += 1.0f * dt;

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, DX::XM_PIDIV2);
}
 
void ShapesApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	DX::XMVECTOR pos = DX::XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	DX::XMVECTOR target = DX::XMVectorZero();
	DX::XMVECTOR up = DX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DX::XMMATRIX view = DX::XMMatrixLookAtLH(pos, target, up);
	DX::XMStoreFloat4x4(&mView, view);
}

void ShapesApp::Draw(const GameTimer& gt) 
{
    ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    ID3D12Resource* passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;
    
    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence); 
}

void ShapesApp::UpdateObjectCBs(const GameTimer& gt) 
{
    UploadBuffer<ObjectConstants>* currObjectCB = mCurrFrameResource->ObjectCB.get(); 
    for (std::unique_ptr<RenderItem>& element : mRenderItems)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (element->NumFramesDirty > 0)
        {
            DX::XMMATRIX world = DX::XMLoadFloat4x4(&element->World);
            ObjectConstants objConstants;
            DX::XMStoreFloat4x4(&objConstants.World, DX::XMMatrixTranspose(world));
            currObjectCB->CopyData(element->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            element->NumFramesDirty--;
        }
    }
}

void ShapesApp::UpdateMaterialCBs(const GameTimer& gt)
{
	UploadBuffer<MaterialConstants>* currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			DX::XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}
void ShapesApp::UpdateMainPassCB(const GameTimer& gt) 
{
    DX::XMMATRIX view = DX::XMLoadFloat4x4(&mView);
    DX::XMMATRIX proj = DX::XMLoadFloat4x4(&mProj);

    DX::XMMATRIX viewProj    = DX::XMMatrixMultiply(view, proj);
    DX::XMMATRIX invView     = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(view), view);
    DX::XMMATRIX invProj     = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(proj), proj);
    DX::XMMATRIX invViewProj = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(viewProj), viewProj);

    DX::XMStoreFloat4x4(&mMainPassCB.View, DX::XMMatrixTranspose(view));
    DX::XMStoreFloat4x4(&mMainPassCB.InvView, DX::XMMatrixTranspose(invView));
    DX::XMStoreFloat4x4(&mMainPassCB.Proj, DX::XMMatrixTranspose(proj));
    DX::XMStoreFloat4x4(&mMainPassCB.InvProj, DX::XMMatrixTranspose(invProj));
    DX::XMStoreFloat4x4(&mMainPassCB.ViewProj, DX::XMMatrixTranspose(viewProj));
    DX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DX::XMMatrixTranspose(invViewProj));

    mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.RenderTargetSize = DX::XMFLOAT2((f32)mClientWidth, (f32)mClientHeight);
    mMainPassCB.InvRenderTargetSize = DX::XMFLOAT2(1.f / (f32)mClientWidth, 1.f / (f32)mClientHeight);
    mMainPassCB.NearZ = 1.f;
    mMainPassCB.FarZ = 1000.f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.AmbientLight = { .0f, .0f, .0f, 1.f };

    DX::XMVECTOR toLight = MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	DirectX::XMVECTOR lightDir = DX::XMVectorNegate(toLight);

    DX::XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    mMainPassCB.Lights[0].Strength = { 1.f, 1.f, 1.f };

    UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void ShapesApp::BuildRootSignature() 
{
    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    // Create root CBV.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		3, 
		slotRootParameter, 
		0, 
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void ShapesApp::BuildShadersAndInputLayout() 
{
    mShaders["standardVS"] = d3dUtil::CompileShader(L"C:\\dev\\d3d12_book\\src\\Chapter8\\Exercises\\3\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"C:\\dev\\d3d12_book\\src\\Chapter8\\Exercises\\3\\Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void ShapesApp::BuildFrameResources() 
{
    for (i32 i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(
            std::make_unique<FrameResource>(
                md3dDevice.Get(), 
                1, 
                (u32)mRenderItems.size(),
                (u32)mMaterials.size())
            );
    }
}

void ShapesApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box      = geoGen.CreateBox(1.5f, .5f, 1.5f, 3);
    GeometryGenerator::MeshData grid     = geoGen.CreateGrid(20.f, 30.f, 60, 40);
    GeometryGenerator::MeshData sphere   = geoGen.CreateSphere(.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
    // GeometryGenerator::MeshData lamp     = geoGen.CreateSphere(.1, 20, 20);

    // We are concatenating all the geometry into one big vertex/index buffer. So
    // define the regions in the buffer each submesh covers.

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    u32 boxVertexOffset = 0u;
    u32 gridVertexOffset = (u32)box.Vertices.size();
    u32 sphereVertexOffset = gridVertexOffset + (u32)grid.Vertices.size();
    u32 cylinderVertexOffset = sphereVertexOffset + (u32)sphere.Vertices.size();
    // u32 lampVertexOffset = cylinderVertexOffset + (u32)lamp.Vertices.size();

    // Cache the index offsets to each object in the concatenated index buffer.
    u32 boxIndexOffset = 0u;
    u32 gridIndexOffset = (u32)box.Indices32.size();
    u32 sphereIndexOffset = gridIndexOffset + (u32)grid.Indices32.size();
    u32 cylinderIndexOffset = sphereIndexOffset + (u32)sphere.Indices32.size();
    // u32 lampIndexOffset = cylinderIndexOffset + (u32)lamp.Indices32.size();

    // Define the SubmeshGeometry that cover different 
    // regions of the vertex/index buffers.
    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = (u32)box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = (u32)grid.Indices32.size();
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (u32)sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = (u32)cylinder.Indices32.size();
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    // SubmeshGeometry lampSubmesh;
    // lampSubmesh.IndexCount = (u32)lamp.Indices32.size();
    // lampSubmesh.StartIndexLocation = lampIndexOffset;
    // lampSubmesh.BaseVertexLocation = lampVertexOffset;


    // Extract the vertex elements we are interested in and pack the 
    // vertices of all the meshes into one vertex buffer.

    size_t totalVertexCount = 
        box.Vertices.size() + 
        grid.Vertices.size() +
        sphere.Vertices.size() +
        cylinder.Vertices.size(); 
        // lamp.Vertices.size();
    
    std::vector<Vertex> vertices(totalVertexCount);

    u32 k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
    }

    for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
    }

	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
	}
    
    // for (size_t  i = 0; i < lamp.Vertices.size(); ++i, ++k)
    // {
    //     vertices[k].Pos = lamp.Vertices[i].Position;
	// 	vertices[k].Normal = lamp.Vertices[i].Normal;
    // }
    
    std::vector<u16> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	// indices.insert(indices.end(), std::begin(lamp.GetIndices16()), std::end(lamp.GetIndices16()));

    const u32 vbByteSize = (u32)vertices.size() * sizeof(Vertex);
    const u32 ibByteSize = (u32)indices.size() * sizeof(u16);

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), 
        mCommandList.Get(),
        vertices.data(),
        vbByteSize,
        geo->VertexBufferUploader);
    
    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        indices.data(),
        ibByteSize,
        geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;
    // geo->DrawArgs["lamp"] = lampSubmesh;

    mGeometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildMaterials()
{
	std::unique_ptr<Material> plastic = std::make_unique<Material>();
	plastic->Name = "plastic";
	plastic->MatCBIndex = 0;
    plastic->DiffuseAlbedo = DX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    plastic->FresnelR0 = DX::XMFLOAT3(0.05f, 0.05f, 0.05f);
    plastic->Roughness = 0.5f;

	std::unique_ptr<Material> silver = std::make_unique<Material>();
	silver->Name = "silver";
	silver->MatCBIndex = 1;
    silver->DiffuseAlbedo = DX::XMFLOAT4(0.972, 0.960, 0.915, 1.0f);
    silver->FresnelR0 = DX::XMFLOAT3(0.95f, 0.93f, 0.88f);
    silver->Roughness = 0.01f;

    std::unique_ptr<Material> copper = std::make_unique<Material>();
	copper->Name = "copper";
	copper->MatCBIndex = 2;
    copper->DiffuseAlbedo = DX::XMFLOAT4(0.93f, 0.62f, 0.54f, 1.0f);
    copper->FresnelR0 = DX::XMFLOAT3(0.95f, 0.64f, 0.54f);
    copper->Roughness = 0.2f;

    std::unique_ptr<Material> gold = std::make_unique<Material>();
	gold->Name = "gold";
	gold->MatCBIndex = 3;
    gold->DiffuseAlbedo = DX::XMFLOAT4(0.95f, 0.79f, 0.41f, 1.0f);
    gold->FresnelR0 = DX::XMFLOAT3(1.f, .71, .29);
    gold->Roughness = 0.01f;

	mMaterials["plastic"] = std::move(plastic);
	mMaterials["silver"]  = std::move(silver);
	mMaterials["copper"]  = std::move(copper);
	mMaterials["gold"]    = std::move(gold);
}

void ShapesApp::BuildRenderItems() 
{
    std::unique_ptr<RenderItem> boxRenderItem = std::make_unique<RenderItem>();
    DX::XMMATRIX boxWorld = DX::XMMatrixScaling(2.f, 2.f, 2.f) * DX::XMMatrixTranslation(.0f, .5f, .0f);
    DX::XMStoreFloat4x4(&boxRenderItem->World, boxWorld);
    boxRenderItem->ObjCBIndex = 0;
    boxRenderItem->Mat = mMaterials["silver"].get();
    boxRenderItem->Geo = mGeometries["shapeGeo"].get();
    boxRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRenderItem->IndexCount = boxRenderItem->Geo->DrawArgs["box"].IndexCount;
    boxRenderItem->StartIndexLocation = boxRenderItem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRenderItem->BaseVertexLocation = boxRenderItem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRenderItems.push_back(std::move(boxRenderItem));

    std::unique_ptr<RenderItem> gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
    gridRitem->Mat = mMaterials["plastic"].get();
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mRenderItems.push_back(std::move(gridRitem));
    u32 objCBIndex = 2u;
    for (u32 i = 0; i < 5; ++i)
    {
        std::unique_ptr<RenderItem> leftCylRenderItem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightCylRenderItem = std::make_unique<RenderItem>();

		std::unique_ptr<RenderItem> leftSphereRenderItem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightSphereRenderItem = std::make_unique<RenderItem>();

		// std::unique_ptr<PointLightItem> rightLampRenderItem = std::make_unique<PointLightItem>();
        // std::unique_ptr<PointLightItem> leftLampRenderItem = std::make_unique<PointLightItem>();

        DX::XMMATRIX leftCylWorld = DX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		DX::XMMATRIX rightCylWorld = DX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		DX::XMMATRIX leftSphereWorld = DX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		DX::XMMATRIX rightSphereWorld = DX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        DX::XMStoreFloat4x4(&leftCylRenderItem->World, rightCylWorld);
		leftCylRenderItem->ObjCBIndex = objCBIndex++;
        leftCylRenderItem->Mat = mMaterials["copper"].get();
		leftCylRenderItem->Geo = mGeometries["shapeGeo"].get();
		leftCylRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRenderItem->IndexCount = leftCylRenderItem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRenderItem->StartIndexLocation = leftCylRenderItem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRenderItem->BaseVertexLocation = leftCylRenderItem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        DX::XMStoreFloat4x4(&rightCylRenderItem->World, leftCylWorld);
		rightCylRenderItem->ObjCBIndex = objCBIndex++;
        rightCylRenderItem->Mat = mMaterials["copper"].get();
		rightCylRenderItem->Geo = mGeometries["shapeGeo"].get();
		rightCylRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRenderItem->IndexCount = rightCylRenderItem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRenderItem->StartIndexLocation = rightCylRenderItem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRenderItem->BaseVertexLocation = rightCylRenderItem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        DX::XMStoreFloat4x4(&leftSphereRenderItem->World, leftSphereWorld);
		leftSphereRenderItem->ObjCBIndex = objCBIndex++;
        leftSphereRenderItem->Mat = mMaterials["gold"].get();
		leftSphereRenderItem->Geo = mGeometries["shapeGeo"].get();
		leftSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRenderItem->IndexCount = leftSphereRenderItem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRenderItem->StartIndexLocation = leftSphereRenderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRenderItem->BaseVertexLocation = leftSphereRenderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		DX::XMStoreFloat4x4(&rightSphereRenderItem->World, rightSphereWorld);
		rightSphereRenderItem->ObjCBIndex = objCBIndex++;
        rightSphereRenderItem->Mat = mMaterials["gold"].get();
		rightSphereRenderItem->Geo = mGeometries["shapeGeo"].get();
		rightSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRenderItem->IndexCount = rightSphereRenderItem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRenderItem->StartIndexLocation = rightSphereRenderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRenderItem->BaseVertexLocation = rightSphereRenderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        // DX::XMMATRIX leftShperePointLight  = DX::XMMatrixTranslation(-5.0f, 5.5f, -10.0f + i * 5.0f);
        // DX::XMMATRIX rightShperePointLight = DX::XMMatrixTranslation(+5.0f, 5.5f, -10.0f + i * 5.0f);

        // DX::XMStoreFloat4x4(&leftLampRenderItem->World, leftShperePointLight);
        // leftLampRenderItem->Mat = mMaterials["silver"].get();
		// leftLampRenderItem->Geo = mGeometries["shapeGeo"].get();
		// leftLampRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		// leftLampRenderItem->IndexCount = leftLampRenderItem->Geo->DrawArgs["lamp"].IndexCount;
		// leftLampRenderItem->StartIndexLocation = leftLampRenderItem->Geo->DrawArgs["lamp"].StartIndexLocation;
		// leftLampRenderItem->BaseVertexLocation = leftLampRenderItem->Geo->DrawArgs["lamp"].BaseVertexLocation;

		// DX::XMStoreFloat4x4(&rightLampRenderItem->World, rightShperePointLight);
        // rightLampRenderItem->Mat = mMaterials["silver"].get();
		// rightLampRenderItem->Geo = mGeometries["shapeGeo"].get();
		// rightLampRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		// rightLampRenderItem->IndexCount = rightLampRenderItem->Geo->DrawArgs["lamp"].IndexCount;
		// rightLampRenderItem->StartIndexLocation = rightLampRenderItem->Geo->DrawArgs["lamp"].StartIndexLocation;
		// rightLampRenderItem->BaseVertexLocation = rightLampRenderItem->Geo->DrawArgs["lamp"].BaseVertexLocation;

		mRenderItems.push_back(std::move(leftCylRenderItem));
		mRenderItems.push_back(std::move(rightCylRenderItem));
		mRenderItems.push_back(std::move(leftSphereRenderItem));
		mRenderItems.push_back(std::move(rightSphereRenderItem));

        // mPointLightItems.push_back(std::move(leftLampRenderItem));
        // mPointLightItems.push_back(std::move(rightLampRenderItem));
    }

    // All the render items are opaque.
	for(auto& e : mRenderItems)
		mRitemLayer[(i32)RenderLayer::Opaque].push_back(e.get());
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems) 
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = mCurrFrameResource->ObjectCB->Resource();
	ID3D12Resource* matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for(size_t i = 0; i < ritems.size(); ++i)
	{
		RenderItem* ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}


void ShapesApp::BuildPSOs() 
{
    	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// PSO for opaque objects.
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

#include <Chapter9/CrateApp.hpp>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, i32 showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        CrateApp theApp(hInstance);
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

CrateApp::CrateApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool CrateApp::Initialize()
{
    if (!D3DApp::Initialize()) { return false; }
    
    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
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
     
    FlushCommandQueue();
    return true;
}

void CrateApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    DX::XMMATRIX P = DX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    DX::XMStoreFloat4x4(&mProj, P);
}

void CrateApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    AnimateMaterials(gt);
    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    UpdateMainPassCB(gt);
}

void CrateApp::Draw(const GameTimer& gt)
{
    ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrFrameResource->CmdAlloc;
    
    // Resuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get())); 

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    CD3DX12_RESOURCE_BARRIER transitionBarrier = 
        CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(), 
            D3D12_RESOURCE_STATE_PRESENT, 
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &transitionBarrier);

    // Clear the back buffer and depth buffer
    D3D12_CPU_DESCRIPTOR_HANDLE hBackBufferDesc = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE hDepthStencilDesc = DepthStencilView();

    mCommandList->ClearRenderTargetView(hBackBufferDesc, DX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(hDepthStencilDesc, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &hBackBufferDesc, true, &hDepthStencilDesc);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    ID3D12Resource* passCB = mCurrFrameResource->MaterialCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    // Indicate a state transition on the resource usage.
    transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
    );

    mCommandList->ResourceBarrier(1, &transitionBarrier);

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

void CrateApp::OnMouseDown(WPARAM btnState, i32 x, i32 y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, i32 x, i32 y)
{
    ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, i32 x, i32 y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        f32 dx = DX::XMConvertToRadians(0.25f * static_cast<f32>(x - mLastMousePos.x));
        f32 dy = DX::XMConvertToRadians(0.25f * static_cast<f32>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        f32 dx = 0.05f * static_cast<f32>(x - mLastMousePos.x);
        f32 dy = 0.05f * static_cast<f32>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void CrateApp::OnKeyboardInput(const GameTimer& gt)
{
}
 
void CrateApp::UpdateCamera(const GameTimer& gt)
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

void CrateApp::AnimateMaterials(const GameTimer& gt)
{

}

void CrateApp::UpdateObjectCBs(const GameTimer& gt)
{
    UploadBuffer<ObjectConstants>* currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto& item : mAllRitems)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (item->NumFramesDirty > 0)
        {
            DX::XMMATRIX world = DX::XMLoadFloat4x4(&item->World);
            DX::XMMATRIX texTransform = DX::XMLoadFloat4x4(&item->TexTransform);
            ObjectConstants objConstants;
            DX::XMStoreFloat4x4(&objConstants.World, DX::XMMatrixTranspose(world));
            DX::XMStoreFloat4x4(&objConstants.TexTransform, DX::XMMatrixTranspose(texTransform));
            currObjectCB->CopyData(item->ObjCBIndex, objConstants);
            // Next FrameResource need to be updated too.
            item->NumFramesDirty--;
        }
    }
}

void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
{
    UploadBuffer<MaterialConstants>* currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto& material : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed. If the cbuffer 
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = material.second.get();
        if(mat->NumFramesDirty > 0)
		{
			DX::XMMATRIX matTransform = DX::XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
    }
}

void CrateApp::UpdateMainPassCB(const GameTimer& gt)
{
    DX::XMMATRIX view = DX::XMLoadFloat4x4(&mView);
	DX::XMVECTOR detView = DX::XMMatrixDeterminant(view);

	DX::XMMATRIX proj = DX::XMLoadFloat4x4(&mProj);
	DX::XMVECTOR detProj = DX::XMMatrixDeterminant(proj);

	DX::XMMATRIX viewProj = DX::XMMatrixMultiply(view, proj);
	DX::XMVECTOR detViewProj = DX::XMMatrixDeterminant(viewProj);

	DX::XMMATRIX invView = DX::XMMatrixInverse(&detView, view);
	DX::XMMATRIX invProj = DX::XMMatrixInverse(&detProj, proj);
	DX::XMMATRIX invViewProj = DX::XMMatrixInverse(&detViewProj, viewProj);

	DX::XMStoreFloat4x4(&mMainPassCB.View, DX::XMMatrixTranspose(view));
	DX::XMStoreFloat4x4(&mMainPassCB.InvView, DX::XMMatrixTranspose(invView));
	DX::XMStoreFloat4x4(&mMainPassCB.Proj, DX::XMMatrixTranspose(proj));
	DX::XMStoreFloat4x4(&mMainPassCB.InvProj, DX::XMMatrixTranspose(invProj));
	DX::XMStoreFloat4x4(&mMainPassCB.ViewProj, DX::XMMatrixTranspose(viewProj));
	DX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DX::XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosWorld = mEyePos;
	mMainPassCB.RenderTargetSize = DX::XMFLOAT2((f32)mClientWidth, (f32)mClientHeight);
	mMainPassCB.InvRenderTargetSize = DX::XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void CrateApp::LoadTextures()
{
    std::unique_ptr<Texture> woodCrateTex = std::make_unique<Texture>();
    woodCrateTex->Name = "woodCrateTex";
    woodCrateTex->Filename = L"Textures/WoodCrate01.dds";
    ThrowIfFailed(DX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), woodCrateTex->Filename.c_str(),
        woodCrateTex->Resource, woodCrateTex->UploadHeap));

    mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void CrateApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    
    // Perfomance TIP: order from most frequent to least frequent
    slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

    auto staticSamplers = GetStaticSamplers();
    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        _countof(slotRootParameter), 
        slotRootParameter, 
        (u32)staticSamplers.size(), 
        staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    // create a root signature with a single slot which points to a desc range consisting of single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    
    if (errorBlob != nullptr)
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

void CrateApp::BuildDescriptorHeaps()
{
    // Create the SRV heap  
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    // Fill out the heap with actual descriptors.
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    ComPtr<ID3D12Resource> woodCrateTex = mTextures["woodCrateTex"]->Resource;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = woodCrateTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = .0f;

    md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
}

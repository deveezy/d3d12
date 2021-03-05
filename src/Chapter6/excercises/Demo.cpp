#include <Common/d3dApp.hpp>
#include <Common/MathHelper.hpp>
#include <Common/UploadBuffer.hpp>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
    u32      Instance;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
    XMFLOAT4 PulseColor = XMFLOAT4(Colors::DarkMagenta);
    f32 Time = .0f;
};

class BoxApp : public D3DApp
{
public:
    BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp&);
    ~BoxApp();

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, i32 x, i32 y) override;
    virtual void OnMouseUp(WPARAM btnState, i32 x, i32 y) override;
    virtual void OnMouseMove(WPARAM btnState, i32 x, i32 y) override;

    void BuildCBDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPyramidGeometry();
    void BuildPSO();

private:
    ComPtr<ID3D12RootSignature>  mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView  = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj  = MathHelper::Identity4x4();

    f32 mTheta  = 1.5f * XM_PI;
    f32 mPhi    = XM_PIDIV4;
    f32 mRadius = 5.f;

    POINT mLastMousePos;
};

// i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
// 				   PSTR cmdLine, i32 showCmd)
// {
// 	// Enable run-time memory check for debug builds.
// #if defined(DEBUG) | defined(_DEBUG)
// 	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
// #endif

//     try
//     {
//         BoxApp theApp(hInstance);
//         if(!theApp.Initialize())
//             return 0;

//         return theApp.Run();
//     }
//     catch(DxException& e)
//     {
//         MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
//         return 0;
//     }
// }

// BoxApp::BoxApp(HINSTANCE hInstance)
// : D3DApp(hInstance) {}

// BoxApp::~BoxApp() {}

// bool BoxApp::Initialize()
// {
//     if (!D3DApp::Initialize())
//     {
//         return false;
//     }

//     // Reset the command list to prep for initialization commands
//     ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//     BuildCBDescriptorHeaps();
//     BuildConstantBuffers();
//     BuildRootSignature();
//     BuildShadersAndInputLayout();
//     // BuildPyramidGeometry();
//     BuildBoxGeometry();
//     BuildPSO();

//     // Execute the initialization commands.
//     ThrowIfFailed(mCommandList->Close());
//     ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//     mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

//     // Wait until initialization is complete.
//     FlushCommandQueue();

//     return true;
// }

void BoxApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio
    // and recompute the projection matrix.
    XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
    DirectX::XMStoreFloat4x4(&mProj, P);
}

void BoxApp::BuildCBDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 2;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
    mCbvHeap->SetName(L"Constant Buffer View Descriptor Heap");
}

void BoxApp::BuildConstantBuffers()
{
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 2, true);
    u32 objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants)); 
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    // Offset to the ith object constant buffer in the buffer.

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHeapHandle(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
    for (size_t boxCBufIndex = 0; boxCBufIndex < 2; ++boxCBufIndex) 
    {
        cbAddress += boxCBufIndex * objCBByteSize;
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = objCBByteSize;
        md3dDevice->CreateConstantBufferView(&cbvDesc, cbvHeapHandle);

        cbvHeapHandle.Offset(mCbvSrvUavDescriptorSize);
    }
}

void BoxApp::BuildRootSignature()
{
    // Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.  

	// Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameters[1];

    // Create a single descriptor table of CBVs.
    CD3DX12_DESCRIPTOR_RANGE range[2];
    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    slotRootParameters[0].InitAsDescriptorTable(_countof(range), range);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(slotRootParameters), slotRootParameters, 0,
        nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a
    // descriptor range consisting of a single constant buffer.
    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1, 
        serializedRootSignature.GetAddressOf(),
        errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0, 
        serializedRootSignature->GetBufferPointer(), 
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
    mvsByteCode = d3dUtil::CompileShader(L"D:\\dev\\d3d12_book\\src\\Chapter6\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = d3dUtil::CompileShader(L"D:\\dev\\d3d12_book\\src\\Chapter6\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "INSTANCE", 0, DXGI_FORMAT_R32_UINT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}


template <typename T>
void mergeArrays(T* arr1, T* arr2, int n1, int n2, T* arr3) 
{ 
    size_t idx = 0;
    for (idx = 0; idx < n1; ++idx) 
    {
        arr3[idx] = arr1[idx];
    }
    for (size_t i = 0; i < n2; ++i)
    {
        arr3[idx + i] = arr2[i];
    }
} 

void BoxApp::BuildBoxGeometry()
{
    Vertex vertices[8] = 
    {
        Vertex({ XMFLOAT3(-1.5f, -1.0f, -1.0f), XMFLOAT4(Colors::White), 0u}),
		Vertex({ XMFLOAT3(-1.5f, +1.0f, -1.0f), XMFLOAT4(Colors::Black), 0u}),
		Vertex({ XMFLOAT3(+0.5f, +1.0f, -1.0f), XMFLOAT4(Colors::Red), 0u}),
		Vertex({ XMFLOAT3(+0.5f, -1.0f, -1.0f), XMFLOAT4(Colors::Green), 0u }),
		Vertex({ XMFLOAT3(-1.5f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue), 0u }),
		Vertex({ XMFLOAT3(-1.5f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow), 0u }),
		Vertex({ XMFLOAT3(+0.5f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan), 0u }),
		Vertex({ XMFLOAT3(+0.5f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta), 0u })
    };

	u16 indices[] =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

    // Pyramid
    Vertex vertices_pyramid[5] =
    {
		// far left 0
        Vertex { XMFLOAT3(1.f, -1.0f, 2.0f), XMFLOAT4(Colors::Green), 1u},
        // far right 1
        Vertex { XMFLOAT3(3.0f, -1.0f, 2.0f), XMFLOAT4(Colors::Green), 1u },
        // near left 2
        Vertex { XMFLOAT3(1.f, -1.0f, 0.0f), XMFLOAT4(Colors::Green), 1u },
        //  3
        Vertex { XMFLOAT3(3.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Green), 1u },
        // top 4
        Vertex { XMFLOAT3(2.0f, 1.0f, 1.0f), XMFLOAT4(Colors::Red), 1u },
    };

	std::uint16_t indices_pyramid[] =
	{
        0, 2, 1,    // side 1
        1, 2, 3,
        0, 1, 4,    // side 2
        1, 3, 4,
        3, 2, 4,    // side 3
        2, 0, 4,
	};

    const u32 vbByteSize = (u32)_countof(vertices) * sizeof(Vertex) + (u32)_countof(vertices_pyramid) * sizeof(Vertex);
    const u32 ibByteSize = (u32)_countof(indices)  * sizeof(u16) + (u32)_countof(indices_pyramid) * sizeof(u16);
    Vertex vertex[13];
    u16 index[54];
    mergeArrays(vertices, vertices_pyramid, _countof(vertices), _countof(vertices_pyramid), vertex);
    mergeArrays(indices,  indices_pyramid,  _countof(indices),  _countof(indices_pyramid),  index);
    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "boxGeo";
    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertex, vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), index, ibByteSize);

    mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(), 
        vertex, 
        vbByteSize,
        mBoxGeo->VertexBufferUploader);

    mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(), 
        index, 
        ibByteSize,
        mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)36;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;

    //*****************New Added**********************
	submesh.IndexCount = (UINT)18;
	submesh.StartIndexLocation = 36;
	submesh.BaseVertexLocation = 8;
	mBoxGeo->DrawArgs["pyramid"] = submesh;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (u32)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = 
    {
        (BYTE*)(mvsByteCode->GetBufferPointer()),
        mvsByteCode->GetBufferSize()
    };
    psoDesc.PS = 
    {
        (BYTE*)(mpsByteCode->GetBufferPointer()),
        mpsByteCode->GetBufferSize()
    };
    CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);
    psoDesc.RasterizerState = rsDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void BoxApp::Update(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    f32 x = mRadius * sinf(mPhi) * cosf(mTheta);
    f32 z = mRadius * sinf(mPhi) * sinf(mTheta);
    f32 y = mRadius * cosf(mPhi);
    // Build the view matrix.
    XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.f);
    XMVECTOR target = DirectX::XMVectorZero();
    XMVECTOR up = DirectX::XMVectorSet(.0f, 1.f, .0f, .0f);

    XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
    DirectX::XMStoreFloat4x4(&mView, view);

    XMMATRIX world = DirectX::XMLoadFloat4x4(&mWorld);
    XMMATRIX proj  = DirectX::XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;
    // Update the constant buffer with the latest worldViewProj matrix.
    ObjectConstants objConstants;
    DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));
    objConstants.Time = gt.TotalTime();
    mObjectCB->CopyData(0, objConstants);
    
}

void BoxApp::Draw(const GameTimer& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists 
    // have finished execution on the GPU.
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // A command list can be reset after it has been added to the 
    // command queue via ExecuteCommandList.  
    // Reusing the command list reuses memory
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(), 
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
	
    SubmeshGeometry pyramid = mBoxGeo->DrawArgs["pyramid"];
	mCommandList->DrawIndexedInstanced(pyramid.IndexCount, 1, pyramid.StartIndexLocation, pyramid.BaseVertexLocation, 0);
    
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

    // swap the back buffer and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Wait until frame commands are complete. This waiting is inefficient
    // and is done for simplicity. Later we will show how to organize our
    // rendering code so we do not have to wait per frame.
    FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, i32 x, i32 y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, i32 x, i32 y)
{
    ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, i32 x, i32 y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        f32 dx = XMConvertToRadians(0.25f * (f32)(x - mLastMousePos.x));
        f32 dy = XMConvertToRadians(0.25f * (f32)(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi   += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, .1f, MathHelper::Pi - .1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        f32 dx = 0.005f * (f32)(x - mLastMousePos.x);
        f32 dy = 0.005f * (f32)(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;
        
        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 3.f, 15.f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
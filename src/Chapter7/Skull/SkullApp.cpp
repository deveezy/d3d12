#include <Chapter7/Skull/SkullApp.hpp>
#include <io/FileUtil.hpp>
#include <io/StringUtil.hpp>
#include <ppl.h>



i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        SkullApp theApp(hInstance);
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

SkullApp::SkullApp(HINSTANCE h_instance) : D3DApp(h_instance) {}

SkullApp::~SkullApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

bool SkullApp::Initialize()
{
    if (!D3DApp::Initialize())
        return false;

    // Reset the command lists to prepare for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildRenderItems();
    BuildFrameResources();
    BuildDescriptorHeaps();
    BuildConstantBufferViews();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void SkullApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    DX::XMMATRIX P = DX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    DX::XMStoreFloat4x4(&mat_proj, P);
}

void SkullApp::BuildRootSignature() 
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    // Root parameter can be a table, root descriptor or root constants. 
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsConstants(sizeof(DX::XMMATRIX) / 4, 0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        _countof(slotRootParameter),
        slotRootParameter, 
        0,
        nullptr, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
 	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc, 
        D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), 
        errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(root_signature.GetAddressOf())));
}

// Descriptors for root descriptors table
void SkullApp::BuildDescriptorHeaps() 
{
    // Need a CBV desriptor for each object for each frame resource,
    // +1 for the perPass CBV for each frame resource.
    u32 numDescriptors = (1) * gNumFrameResources; // 1 - count of global constant buffers

    // Save an offset to the start of the pass CBVs. These are the last 3 desriptors.
    // mPassCbvOffset = objCount * gNumFrameResources;
    global_cbv_offset = 0u;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbv_heap)));
}

void SkullApp::BuildConstantBufferViews()
{
    u32 global_cb_byte_size = d3dUtil::CalcConstantBufferByteSize(sizeof(GlobalConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for(i32 frame_index = 0; frame_index < gNumFrameResources; ++frame_index)
    {
        ID3D12Resource* global_constant_buffer = frame_resources[frame_index]->global_cb->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = global_constant_buffer->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        i32 heapIndex = global_cbv_offset + frame_index;
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle = 
            CD3DX12_CPU_DESCRIPTOR_HANDLE(cbv_heap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = global_cb_byte_size;
        
        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void SkullApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);
    
    // Cycle through the circular frame resource array.
    curr_frame_resource_index = (curr_frame_resource_index + 1) % gNumFrameResources;
    curr_frame_resource       = frame_resources[curr_frame_resource_index].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (curr_frame_resource->fence != 0 && mFence->GetCompletedValue() < curr_frame_resource->fence)
    {
        HANDLE hEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(curr_frame_resource->fence, hEvent))
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
}

void SkullApp::OnMouseDown(WPARAM btn_state, i32 x, i32 y)
{
    last_mouse_pos.x = x;
    last_mouse_pos.y = y;

    SetCapture(mhMainWnd);
}

void SkullApp::OnMouseUp(WPARAM btn_state, i32 x, i32 y)
{
    ReleaseCapture();
}

void SkullApp::OnMouseMove(WPARAM btn_state, i32 x, i32 y)
{
    if((btn_state & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        f32 dx = DX::XMConvertToRadians(0.25f * static_cast<f32>(x - last_mouse_pos.x));
        f32 dy = DX::XMConvertToRadians(0.25f*static_cast<f32>(y - last_mouse_pos.y));

        // Update angles based on input to orbit camera around box.
        theta += dx;
        phi += dy;

        // Restrict the angle mPhi.
        phi = MathHelper::Clamp(phi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btn_state & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f * static_cast<f32>(x - last_mouse_pos.x);
        float dy = 0.05f * static_cast<f32>(y - last_mouse_pos.y);

        // Update the camera radius based on input.
        radius += dx - dy;

        // Restrict the radius.
        radius = MathHelper::Clamp(radius, 5.0f, 150.0f);
    }

    last_mouse_pos.x = x;
    last_mouse_pos.y = y;
}

void SkullApp::OnKeyboardInput(const GameTimer& gt)
{
    if(GetAsyncKeyState('1') & 0x8000)
        is_wire_frame = true;
    else
        is_wire_frame = false;
}

void SkullApp::UpdateCamera(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    vec3_eye_pos.x = radius * sinf(phi) * cosf(theta);
    vec3_eye_pos.z = radius * sinf(phi) * sinf(theta);
    vec3_eye_pos.y = radius * cosf(phi);

    // Build the view matrix.
    DX::XMVECTOR pos = DX::XMVectorSet(vec3_eye_pos.x, vec3_eye_pos.y, vec3_eye_pos.z, 1.f);
    DX::XMVECTOR target = DX::XMVectorZero();
    DX::XMVECTOR up = DX::XMVectorSet(.0f, 1.f, .0f, .0f);

    DX::XMMATRIX view = DX::XMMatrixLookAtLH(pos, target, up);
    DX::XMStoreFloat4x4(&mat_view, view);
}

void SkullApp::Draw(const GameTimer& gt)
{
    ComPtr<ID3D12CommandAllocator> cmd_alloc = curr_frame_resource->cmd_alloc;
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmd_alloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    if(is_wire_frame)
    {
        ThrowIfFailed(mCommandList->Reset(cmd_alloc.Get(), PSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmd_alloc.Get(), PSOs["opaque"].Get()));
    }

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

    ID3D12DescriptorHeap* descriptorHeaps[] = { cbv_heap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(root_signature.Get());

    i32 global_cbv_index = global_cbv_offset + curr_frame_resource_index; 
    CD3DX12_GPU_DESCRIPTOR_HANDLE global_cbv_handle = 
        CD3DX12_GPU_DESCRIPTOR_HANDLE(cbv_heap->GetGPUDescriptorHandleForHeapStart());
    global_cbv_handle.Offset(global_cbv_index, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(1, global_cbv_handle);
    DrawRenderItems(mCommandList.Get(), opaque_render_items);

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
    curr_frame_resource->fence = ++mCurrentFence;
    
    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence); 
}

void SkullApp::UpdateObjectCBs(const GameTimer& gt) 
{
    UploadBuffer<PerObjectConstants>* curr_object_cb = curr_frame_resource->per_object_cb.get(); 
    for (std::unique_ptr<RenderItem>& element : render_items)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (element->num_frames_dirty > 0)
        {
            DX::XMMATRIX world = DX::XMLoadFloat4x4(&element->mat_world);
            PerObjectConstants per_object_constant;
            DX::XMStoreFloat4x4(&per_object_constant.mat_world, DX::XMMatrixTranspose(world));
            curr_object_cb->CopyData(element->per_obj_cb_index, per_object_constant);

            // Next FrameResource need to be updated too.
            element->num_frames_dirty--;
        }
    }
}

void SkullApp::UpdateMainPassCB(const GameTimer& gt) 
{
    DX::XMMATRIX view = DX::XMLoadFloat4x4(&mat_view);
    DX::XMMATRIX proj = DX::XMLoadFloat4x4(&mat_proj);

    DX::XMMATRIX viewProj    = DX::XMMatrixMultiply(view, proj);
    DX::XMMATRIX invView     = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(view), view);
    DX::XMMATRIX invProj     = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(proj), proj);
    DX::XMMATRIX invViewProj = DX::XMMatrixInverse(&DX::XMMatrixDeterminant(viewProj), viewProj);

    DX::XMStoreFloat4x4(&global_cb.mat_view, DX::XMMatrixTranspose(view));
    DX::XMStoreFloat4x4(&global_cb.mat_inv_view, DX::XMMatrixTranspose(invView));
    DX::XMStoreFloat4x4(&global_cb.mat_proj, DX::XMMatrixTranspose(proj));
    DX::XMStoreFloat4x4(&global_cb.mat_inv_proj, DX::XMMatrixTranspose(invProj));
    DX::XMStoreFloat4x4(&global_cb.mat_view_proj, DX::XMMatrixTranspose(viewProj));
    DX::XMStoreFloat4x4(&global_cb.mat_inv_view_proj, DX::XMMatrixTranspose(invViewProj));

    global_cb.vec3_eye_pos_world = vec3_eye_pos;
    global_cb.vec2_render_target_size = DX::XMFLOAT2( (f32)mClientWidth, (f32)mClientHeight );
    global_cb.vec2_inv_render_target_size = DX::XMFLOAT2( 1.f / (f32)mClientWidth, 1.f / (f32)mClientHeight );
    global_cb.near_z = 1.f;
    global_cb.far_z = 1000.f;
    global_cb.total_time = gt.TotalTime();
    global_cb.delta_time = gt.DeltaTime();

    UploadBuffer<GlobalConstants>* currPassCB = curr_frame_resource->global_cb.get();
    currPassCB->CopyData(0, global_cb);
}

void SkullApp::BuildShadersAndInputLayout()
{
    shaders_blob["standardVS"] = d3dUtil::CompileShader(
        L"C:\\dev\\d3d12_book\\src\\Chapter7\\Skull\\Shaders\\shader.hlsl", nullptr, "VS", "vs_5_1");

	shaders_blob["opaquePS"] = d3dUtil::CompileShader(
        L"C:\\dev\\d3d12_book\\src\\Chapter7\\Skull\\Shaders\\shader.hlsl", nullptr, "PS", "ps_5_1");

    input_layout = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void SkullApp::BuildFrameResources()
{
    for (size_t i = 0; i < gNumFrameResources; ++i)
    {
        frame_resources.push_back(
            std::make_unique<FrameResource>
            (
                md3dDevice.Get(),
                1,
                (u32)render_items.size()
            )
        );
    }
}

#define POSITION_VEC3 
#define NORMAL_VEC3 

void ReadFormatted(const char* str, const char * format, ... )
{
    va_list args;
    va_start (args, format);
    int items = vsscanf (str, format, args);
    va_end (args);
}

i32 ReadVec4Float(const char* str, DirectX::XMFLOAT4* vec4)
{
    i32 offset = -1;
    ReadFormatted(str, "%.6f %.6f %.6f %.6f %n", &vec4->x, &vec4->y, &vec4->z, &vec4->w, &offset);
    return offset;
}

i32 ReadVec3x2Float(const char* str, DirectX::XMFLOAT3* first, DirectX::XMFLOAT3* second)
{
    i32 offset = -1;
    ReadFormatted(str, "%f %f %f %f %f %f%n",
        &first->x, &first->y, &first->z, 
        &second->x, &second->y, &second->z, 
        &offset);

    return offset;
}

i32 ReadVec3Float(const char* str, DirectX::XMFLOAT3* vec3)
{
    i32 offset = -1;
    ReadFormatted(str, "%f %f %f%n", &vec3->x, &vec3->y, &vec3->z, &offset);
    return offset;
}

i32 ReadNumber(const char* str, u16* number)
{
    i32 offset = -1;
    ReadFormatted(str, "%hu%n", number, &offset);
    return offset;
}

void AddIndices(std::vector<u16>& indices, const char* index_string)
{
    i32 offset = 0;
    u16 number = 0;
    for (; *(index_string + offset) != '\0' ;)
    {
        offset += ReadNumber(index_string + offset, &number);
        indices.emplace_back(std::move(number));
    }
}

void AddVertecies(std::vector<Vertex>& vertices, const char* vertex_string)
{
    i32 offset = 0;
    Vertex vertex;
    for (; *(vertex_string + offset) != '\0'; )
    {
    #ifdef POSITION_VEC3
        offset += ReadVec3x2Float(vertex_string + offset, &vertex.vec3_pos, &vertex.vec3_norm);
    #endif
        vertices.emplace_back(std::move(vertex));
    }
}

void SkullApp::BuildShapeGeometry()
{
    File file("D:\\dev\\skull.txt", "r");
    file.ReadText(file.Size());

    char** splitted_strings = StringUtil::Split(file.m_fBuffer, '\n', nullptr);
    char** count_substrings = StringUtil::Substring(splitted_strings, "count", nullptr);

    i64 vertexCount = StringUtil::ExtractNumber(count_substrings[0]);
    i64 triangleCount = StringUtil::ExtractNumber(count_substrings[1]);

    StringUtil::ClearStrings(splitted_strings);
    StringUtil::ClearStrings(count_substrings);
    std::vector<Vertex> vertices;
    std::vector<u16>    indices;
    vertices.reserve(vertexCount);
    indices.reserve(triangleCount * 3);

    size_t stringCount;
    char** strings = StringUtil::SplitFormatted(file.m_fBuffer, '{', '}', &stringCount);

    for (size_t i = 0; i < stringCount; ++i)
    {
        StringUtil::RemoveEndl(strings[i]);
        StringUtil::StripExtraSpace(strings[i]);
        size_t len = strlen(strings[i]);
        strings[i] = (char*)realloc(strings[i], len + 1);
        strings[i][len] = 0;
    }
    
    char* vertex_str = strings[0];
    char* index_str  = strings[1];

    AddVertecies(vertices, vertex_str);
    AddIndices(indices, index_str);
    StringUtil::ClearStrings(strings);

    SubmeshGeometry skull_submesh;
    skull_submesh.IndexCount = (u32)indices.size();
    skull_submesh.StartIndexLocation = 0;
    skull_submesh.BaseVertexLocation = 0;

    const u32 vb_byte_size = (u32)vertices.size() * sizeof(Vertex);
    const u32 ib_byte_size = (u32)indices.size()  * sizeof(u16);

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "skullGeo";

    ThrowIfFailed(D3DCreateBlob(vb_byte_size, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vb_byte_size);

    ThrowIfFailed(D3DCreateBlob(ib_byte_size, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ib_byte_size);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(), 
        mCommandList.Get(),
        vertices.data(),
        vb_byte_size,
        geo->VertexBufferUploader);
    
    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        indices.data(),
        ib_byte_size,
        geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vb_byte_size;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ib_byte_size;

    geo->DrawArgs["skull"] = skull_submesh;

    geometries[geo->Name] = std::move(geo);
}

void SkullApp::BuildRenderItems()
{
    std::unique_ptr<RenderItem> skull_render_item = std::make_unique<RenderItem>();
    DX::XMMATRIX skull_mat_world = DX::XMMatrixScaling(2.f, 2.f, 2.f) * DX::XMMatrixTranslation(.0f, .5f, .0f);
    DX::XMStoreFloat4x4(&skull_render_item->mat_world, skull_mat_world);
    skull_render_item->per_obj_cb_index = 0;
    skull_render_item->geometry = geometries["skullGeo"].get();
    skull_render_item->primitive_type = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skull_render_item->index_count = skull_render_item->geometry->DrawArgs["skull"].IndexCount;
    skull_render_item->start_index_location = skull_render_item->geometry->DrawArgs["skull"].StartIndexLocation;
    skull_render_item->base_vertex_location = skull_render_item->geometry->DrawArgs["skull"].BaseVertexLocation;
    render_items.push_back(std::move(skull_render_item));

    // All the render items are opaque.
    for (auto& e : render_items)
    {
        opaque_render_items.push_back(e.get());
    }
}

void SkullApp::DrawRenderItems(ID3D12GraphicsCommandList* cmd_list, const std::vector<RenderItem*>& ritems) 
{
    // For each render item...
    for (size_t i = 0; i < ritems.size(); ++i)
    {
        RenderItem* item = ritems[i];
        cmd_list->IASetVertexBuffers(0, 1, &item->geometry->VertexBufferView());
        cmd_list->IASetIndexBuffer(&item->geometry->IndexBufferView());
        cmd_list->IASetPrimitiveTopology(item->primitive_type);

        DX::XMMATRIX mat_world = DX::XMLoadFloat4x4(&item->mat_world);
        cmd_list->SetGraphicsRoot32BitConstants(
            0, 
            sizeof(DX::XMMATRIX) / 4, 
            &DX::XMMatrixTranspose(mat_world),
            0);
        cmd_list->DrawIndexedInstanced(
            item->index_count,
            1,
            item->start_index_location,
            item->base_vertex_location,
            0);
    }
}

void SkullApp::BuildPSOs() 
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// PSO for opaque objects.
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { input_layout.data(), (UINT)input_layout.size() };
	opaquePsoDesc.pRootSignature = root_signature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(shaders_blob["standardVS"]->GetBufferPointer()), 
		shaders_blob["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(shaders_blob["opaquePS"]->GetBufferPointer()),
		shaders_blob["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&PSOs["opaque"])));


    // PSO for opaque wireframe objects.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&PSOs["opaque_wireframe"]))); 
}
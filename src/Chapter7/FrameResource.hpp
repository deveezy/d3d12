#pragma once

#include <Common/d3dUtil.hpp>
#include <Common/MathHelper.hpp>
#include <Common/UploadBuffer.hpp>

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { .0f, .0f, .0f };
    f32 cbPerObjectPad1 = .0f;
    DirectX::XMFLOAT2 RenderTargetSize = { .0f, .0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { .0f, .0f };
    f32 NearZ = .0f;
    f32 FarZ  = .0f;
    f32 TotalTime = .0f;
    f32 DeltaTime = .0f;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.
struct FrameResource
{
    FrameResource(ID3D12Device* device, u32 passCount, u32 objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it. So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence value to mark commands up to this fence point. This lets us
    // check if these frame resources are still in use by the GPU.
    u64 Fence = 0;
};

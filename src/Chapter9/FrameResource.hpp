#pragma once

#include <Common/d3dUtil.hpp>
#include <Common/MathHelper.hpp>
#include <Common/UploadBuffer.hpp>

namespace DX = DirectX;

struct ObjectConstants
{
    DX::XMFLOAT4X4 World = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

struct PassConstants
{
    DX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DX::XMFLOAT3   EyePosWorld = { .0f, .0f, .0f };
    f32 padding = .0f;
    DX::XMFLOAT2 RenderTargetSize = { .0f, .0f };
    DX::XMFLOAT2 InvRenderTargetSize = { .0f, .0f };
    f32 NearZ = .0f;
    f32 FarZ  = .0f;
    f32 TotalTime = .0f;
    f32 DeltaTime = .0f;

    DX::XMFLOAT4 AmbientLight = { .0f, .0f, .0f, 1.f };
    Light Lights[MaxLights];
};

struct Vertex
{
    DX::XMFLOAT3 Pos;
    DX::XMFLOAT3 Normal;
    DX::XMFLOAT2 TexC;
};

struct FrameResource
{
    FrameResource(ID3D12Device* device, u32 passCount, u32 objectCount, u32 materialCount);
    FrameResource(const FrameResource&) = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    ~FrameResource();

    // We cannot reset the allocator until the GPU is done processing the comands.
    // So each frame needs their own allocator.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it. So each frame needs thir own cbuffers.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};
#pragma once

#include <Common/d3dUtil.hpp>
#include <Common/MathHelper.hpp>
#include <Common/UploadBuffer.hpp>

namespace DX = DirectX;
using Microsoft::WRL::ComPtr;

struct PerObjectConstants
{
    DX::XMFLOAT4X4 mat_world = MathHelper::Identity4x4();
};

struct GlobalConstants
{
    DX::XMFLOAT4X4 mat_view          = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 mat_inv_view      = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 mat_proj          = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 mat_inv_proj      = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 mat_view_proj     = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 mat_inv_view_proj = MathHelper::Identity4x4();

    DX::XMFLOAT3   vec3_eye_pos_world = { .0f, .0f, .0f };
    f32 padding = .0f;
    DX::XMFLOAT2   vec2_render_target_size = { .0f, .0f };
    DX::XMFLOAT2   vec2_inv_render_target_size = { .0f, .0f };

    f32 near_z = .0f;
    f32 far_z  = .0f;
    f32 total_time = .0f;
    f32 delta_time = .0f;
};

struct Vertex
{
    DX::XMFLOAT3 vec3_pos;
    DX::XMFLOAT3 vec3_norm;
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.
struct FrameResource
{
    FrameResource(ID3D12Device* device, u32 global_cb_count, u32 object_count);
    FrameResource(const FrameResource&) = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    ~FrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    ComPtr<ID3D12CommandAllocator> cmd_alloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it. So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<GlobalConstants>>    global_cb = nullptr;
    std::unique_ptr<UploadBuffer<PerObjectConstants>> per_object_cb = nullptr;

    // Fence value to mark commands up to this fence point. This lets us
    // check if these frame resources are still in use by the GPU.
    u64 fence = 0;
};
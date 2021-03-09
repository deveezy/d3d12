#include <Chapter7/Skull/FrameResource.hpp>

FrameResource::FrameResource(ID3D12Device* device, u32 global_cb_count, u32 object_count) 
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(cmd_alloc.GetAddressOf())));
    
    global_cb = std::make_unique<UploadBuffer<GlobalConstants>>(device, global_cb_count, true);
    
    per_object_cb = std::make_unique<UploadBuffer<PerObjectConstants>>(device, object_count, true);
}


FrameResource::~FrameResource() {}
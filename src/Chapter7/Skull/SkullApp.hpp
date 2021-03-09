#pragma once

#include <Common/d3dApp.hpp>
#include <Common/UploadBuffer.hpp>
#include <Common/GeometryGenerator.hpp>
#include <Chapter7/Skull/FrameResource.hpp>

const i32 gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.
// This will vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object the world.
    DX::XMFLOAT4X4 mat_world = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource. Thus, when modify object data we should set
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    i32 num_frames_dirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the PerObjectConstant for this render item;
    i32 per_obj_cb_index = -1;

    MeshGeometry* geometry = nullptr;

    // Primitive topology
    D3D12_PRIMITIVE_TOPOLOGY primitive_type = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    u32 index_count = 0;
    u32 start_index_location = 0;
    i32 base_vertex_location = 0;
};

class SkullApp : public D3DApp
{
public:
    SkullApp(HINSTANCE hInstance);
    SkullApp(const SkullApp&) = delete;
    SkullApp& operator=(const SkullApp&) = delete;
    ~SkullApp();

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, i32 x, i32 y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

    void BuildDescriptorHeaps();
    void BuildConstantBufferViews();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:
    std::vector<std::unique_ptr<FrameResource>> frame_resources;
    FrameResource* curr_frame_resource = nullptr;
    i32 curr_frame_resource_index = 0;

    ComPtr<ID3D12RootSignature>  root_signature = nullptr;
    ComPtr<ID3D12DescriptorHeap> cbv_heap = nullptr; // for root descriptors
    ComPtr<ID3D12DescriptorHeap> srv_heap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> shaders_blob;

    std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> render_items;

    // Render items divided by PSO.
    std::vector<RenderItem*> opaque_render_items;

    GlobalConstants global_cb;
    u32 global_cbv_offset = 0u; // offset from start heap to general cbv

    bool is_wire_frame = false;

    DX::XMFLOAT3   vec3_eye_pos = { .0f, .0f, .0f };
    DX::XMFLOAT4X4 mat_view = MathHelper::Identity4x4(); 
    DX::XMFLOAT4X4 mat_proj = MathHelper::Identity4x4();

    f32 theta  = 1.5f * DX::XM_PI;
    f32 phi    = .2f  * DX::XM_PI;
    f32 radius = 15.f;

    POINT last_mouse_pos;
};

#pragma once

#include <Common/d3dApp.hpp>
#include <Common/UploadBuffer.hpp>
#include <Common/GeometryGenerator.hpp>
#include <Chapter7/FrameResource.hpp>

namespace dx = DirectX;
using Microsoft::WRL::ComPtr;

const i32 gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape. This will
// vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object the world.
    dx::XMFLOAT4X4 World = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource. Thus, when modify object data we should set
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    i32 NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    u32 ObjCBIndex = -1;

    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    u32 IndexCount = 0;
    u32 StartIndexLocation = 0;
    i32 BaseVertexLocation = 0;
};

class ShapesApp : public D3DApp
{
public:
    ShapesApp(HINSTANCE hInstance);
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp();

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
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    i32 mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature>  mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mRenderItems;

    // Render items divided by PSO.
    std::vector<RenderItem*> mOpaqueRenderItems;

    PassConstants mMainPassCB;
    u32 mPassCbvOffset = 0u;

    bool mIsWireframe = false;

    dx::XMFLOAT3   mEyePos = { .0f, .0f, .0f };
    dx::XMFLOAT4X4 mView = MathHelper::Identity4x4();
    dx::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    f32 mTheta  = 1.5f * dx::XM_PI;
    f32 mPhi    = .2f * dx::XM_PI;
    f32 mRadius = 15.f;

    POINT mLastMousePos;
};
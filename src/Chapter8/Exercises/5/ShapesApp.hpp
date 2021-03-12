#pragma once

#include <Common/d3dApp.hpp>
#include <Common/UploadBuffer.hpp>
#include <Common/GeometryGenerator.hpp>
#include <Chapter8/Exercises/5/FrameResource.hpp>

namespace DX = DirectX;
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
    DX::XMFLOAT4X4 World = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource. Thus, when modify object data we should set
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    i32 NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    i32 ObjCBIndex = -1;

    Material* Mat = nullptr;
    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    u32 IndexCount = 0;
    u32 StartIndexLocation = 0;
    i32 BaseVertexLocation = 0;
};

struct PointLightItem : RenderItem
{
    Light light;
};

enum class RenderLayer : i32 
{
	Opaque = 0,
	Count
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
    void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
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
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mRenderItems;
    std::vector<std::unique_ptr<PointLightItem>> mPointLightItems;

    // Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(i32)RenderLayer::Count];

    PassConstants mMainPassCB;
    DX::XMFLOAT3   mEyePos = { 0.0f, 0.0f, 0.0f };
	DX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    f32 mTheta = 1.5f * DX::XM_PI;
    f32 mPhi = DX::XM_PIDIV2 - 0.1f;
    f32 mRadius = 50.0f;

	f32 mSunTheta = 1.25f * DX::XM_PI;
	f32 mSunPhi   = DX::XM_PIDIV4;

    POINT mLastMousePos;
};
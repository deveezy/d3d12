#include <Common/d3dApp.hpp>
#include <Common/MathHelper.hpp>
#include <Common/UploadBuffer.hpp>
#include <Common/GeometryGenerator.hpp>
#include <Chapter9/FrameResource.hpp>

using Microsoft::WRL::ComPtr;

const i32 gNumFrameResources = 3;

struct RenderItem
{
    RenderItem() = default;

    DX::XMFLOAT4X4 World = MathHelper::Identity4x4();
    DX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    i32 NumFramesDirty = gNumFrameResources;
    u32 ObjCBIndex = -1;
    Material* Mat = nullptr;
    MeshGeometry* Geo = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    u32 IndexCount = 0;
    u32 StartIndexLocation = 0;
    i32 BaseVertexLocation = 0;
};

class CrateApp : public D3DApp
{
public:
    CrateApp(HINSTANCE hInstance);
    CrateApp(const CrateApp& rhs) = delete;
    CrateApp& operator=(const CrateApp& rhs) = delete;
    ~CrateApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, i32 x, i32 y) override;
    virtual void OnMouseUp(WPARAM btnState, i32 x, i32 y) override;
    virtual void OnMouseMove(WPARAM btnState, i32 x, i32 y) override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void LoadTextures();
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    i32 mCurrFrameResourceIndex = 0;

    u32 mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
 
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

    PassConstants mMainPassCB;

	DX::XMFLOAT3   mEyePos = { 0.0f, 0.0f, 0.0f };
	DX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	f32 mTheta = 1.3f * DX::XM_PI;
	f32 mPhi = 0.4f * DX::XM_PI;
	f32 mRadius = 2.5f;

    POINT mLastMousePos;
};

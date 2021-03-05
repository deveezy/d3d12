#pragma once

// #if defined(DEBUG) || defined(_DEBUG) || defined(D_DEBUG)
// #define _CRTDBG_MAP_ALLOC
// #include <crtdbg.h>
// #endif

#include <Common/GameTimer.hpp>
#include <Common/d3dUtil.hpp>
#include <Common/defines.hpp>


// Link necessary d3d12 libraries
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp&) = delete;
    D3DApp& operator=(const D3DApp&) = delete;
    virtual ~D3DApp();

public:
    static D3DApp* GetApp();
    HINSTANCE AppInst() const;
    HWND MainWnd()      const;
    f32 AspectRatio()   const;

    bool Get4xMsaaState() const;
    void Set4xMsaaState(bool value); 

    i32 Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptionHeaps();
    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, i32 x, i32 y) { }
    virtual void OnMouseUp(WPARAM btnState, i32 x, i32 y) { }
    virtual void OnMouseMove(WPARAM btnState, i32 x, i32 y) { }

protected:
    bool InitMainWnd();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();

    void FlushCommandQueue();

    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const; 
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:
    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr; // aplication instance handle
    HWND mhMainWnd  = nullptr; // main window handle
    bool mAppPaused = false; // is the application paused?
    bool mMinimized = false; // is the application minimized?
    bool mMaximized = false; // is the application maximized?
    bool mResizing  = false; // are the resize bars being dragged?
    bool mFullscreenState = false; // fullscren enabled
    
    // Set true to use 4X MSAA. The default is false
    bool m4xMsaaState   = false; // 4X MSAA enabled?
    u32  m4xMsaaQuality = 0;     // quality level of 4X MSAA

    GameTimer mTimer;

    Microsoft::WRL::ComPtr<IDXGIFactory4>  mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device>   md3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence>    mFence;
    u64 mCurrentFence = 0;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    static const i32 SwapChainBufferCount = 2;
    i32 mCurrBackBuffer = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT     mScissorRect;

    u32 mRtvDescriptorSize = 0u;
    u32 mDsvDescriptorSize = 0u;
    u32 mCbvSrvUavDescriptorSize = 0u;

    // Derived class should set these in derived constructor to customize starting values.
    std::wstring mMainWndCaption = L"d3d App";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat  =  DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    i32 mClientWidth  = 800;
    i32 mClientHeight = 600;
};
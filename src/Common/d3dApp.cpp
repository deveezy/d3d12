#include <Common/d3dApp.hpp>
#include <Windowsx.h> 
#include <cassert>
#include <d3d12sdklayers.h>

using Microsoft::WRL::ComPtr;

D3DApp* D3DApp::mApp = nullptr;

D3DApp* D3DApp::GetApp()
{
    return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance) : mhAppInst(hInstance) 
{
    assert(mApp == nullptr);
    mApp = this;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp::~D3DApp() 
{
    if (md3dDevice != nullptr)
    {
        FlushCommandQueue();
    }
}

HINSTANCE D3DApp::AppInst() const
{
    return mhAppInst;
}

HWND D3DApp::MainWnd() const
{
    return mhMainWnd;
}

f32 D3DApp::AspectRatio() const
{
    return static_cast<f32>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState() const
{
    return m4xMsaaState; 
}

void D3DApp::Set4xMsaaState(bool value) 
{
    if (m4xMsaaState != value) 
    {
        m4xMsaaState = value;
        // Recreate the swapchain and buffer with new multisample settings.
        CreateSwapChain();
        OnResize();
    }
}

// TODO: add keyboard messages, killfocus, mouse wheel, CaptureWindow
LRESULT D3DApp::MsgProc(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam) 
{
    switch (msg)
    {
        // WM_ACTIVATE is sent when the window is activated or deactivated.  
        // We pause the game when the window is deactivated and unpause it 
        // when it becomes active.  
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                mAppPaused = true;
                mTimer.Stop();
            }
            else
            {
                mAppPaused = false;
                mTimer.Start();
            }
            return 0;
        }
        // WM_SIZE is sent when the user resizes the window.  
        case WM_SIZE:
        {

            // Save the new client area dimensions.
            mClientWidth  = LOWORD(lParam);
            mClientHeight = HIWORD(lParam);
            if (md3dDevice)
            {
                if (wParam == SIZE_MINIMIZED)            
                {
                    mAppPaused = true;
                    mMinimized = true;
                    mMaximized = false;
                }
                else if (wParam == SIZE_MAXIMIZED)
                {
                    mAppPaused = false;
                    mMinimized = false;
                    mMaximized = true;
                    OnResize();
                }
                else if (wParam == SIZE_RESTORED)
                {
                    // Restored from minimized state?
                    if (mMinimized)
                    {
                        mAppPaused = false;
                        mMinimized = false;
                        OnResize();
                    }
                    // Restored from maximized state?
                    if (mMaximized)
                    {
                        mAppPaused = false;
                        mMaximized = false;
                        OnResize();
                    }
                    else if (mResizing)
                    {
                        // If user is dragging the resize bars, we do not resize 
                        // the buffers here because as the user continuously 
                        // drags the resize bars, a stream of WM_SIZE messages are
                        // sent to the window, and it would be pointless (and slow)
                        // to resize for each WM_SIZE message received from dragging
                        // the resize bars.  So instead, we reset after the user is 
                        // done resizing the window and releases the resize bars, which 
                        // sends a WM_EXITSIZEMOVE message.
                    }
                    else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                    {
                        OnResize();
                    }
                }
            }
            return 0;
        }

        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
        case WM_ENTERSIZEMOVE:
        {
            mAppPaused = true;
            mResizing  = true;
            mTimer.Stop();
            OnResize();
            return 0;
        }
        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
        {
            mAppPaused = false;
            mResizing  = false;
            mTimer.Start();
            OnResize();
            return 0;
        }
        // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        // The WM_MENUCHAR message is sent when a menu is active and the user presses 
        // a key that does not correspond to any mnemonic or accelerator key.
        case WM_MENUCHAR:
        {
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);
        }
        // Catch this message so to prevent the window from becoming too small.
        case WM_GETMINMAXINFO:
        {
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            const POINTS pt = MAKEPOINTS( lParam );
            OnMouseDown(wParam, pt.x, pt.y);
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            const POINTS pt = MAKEPOINTS( lParam );
            OnMouseUp(wParam, pt.x, pt.y);
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            const POINTS pt = MAKEPOINTS( lParam );
            OnMouseMove(wParam, pt.x, pt.y);
            return 0;
        }
        case WM_KEYUP:
        {
            if(wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            else if((int)wParam == VK_F2)
            {
                Set4xMsaaState(!m4xMsaaState);
            }
            return 0;
        }
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWnd() 
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhAppInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWindow";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.",0, 0);
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = {0, 0, mClientWidth, mClientHeight};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    i32 width = R.right - R.left;
    i32 height = R.bottom - R.top;

    mhMainWnd = CreateWindow(L"MainWindow", mMainWndCaption.c_str(), 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

    if (!mhMainWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }
    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);

    return true;
}

i32 D3DApp::Run() 
{
    MSG msg = {0};
    mTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        // if there are Window message then process them.
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, do animation/game stuff.
        else
        {
            mTimer.Tick();

            if (!mAppPaused)
            {
                CalculateFrameStats();
                Update(mTimer);
                Draw(mTimer);
            }
            else
            {
                Sleep(1000);
            }
        }
    }
    return (i32)msg.wParam;
}

bool D3DApp::Initialize() 
{
    if (!InitMainWnd()) 
    {
        return false;
    }
    if (!InitDirect3D())
    {
        return false; 
    }

    // Do the initial resize code.
    OnResize();
    return true;
}

void EnableShaderBasedValidation()
{
    ComPtr<ID3D12Debug> spDebugController0;
    ComPtr<ID3D12Debug1> spDebugController1;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0)));
    ThrowIfFailed(spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1)));
    spDebugController1->SetEnableGPUBasedValidation(true);
    spDebugController0->EnableDebugLayer();
    spDebugController1->SetEnableSynchronizedCommandQueueValidation(true);
}

bool D3DApp::InitDirect3D() 
{
#if defined(DEBUG) || defined(_DEBUG) 
    //Enable the d312 debug layer.
    EnableShaderBasedValidation(); 
#endif
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
    // Try to create hardware device.
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, // default adapter
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&md3dDevice));
    
    // Fallback to WARP device.
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice)));
    }

    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    mRtvDescriptorSize       = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize       = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));
    
    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#if defined(DEBUG) || defined(_DEBUG)
    LogAdapters();
#endif

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptionHeaps();

    return true;
}

void D3DApp::CreateCommandObjects() 
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

    ThrowIfFailed(md3dDevice->CreateCommandList(
        0, 
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        mDirectCmdListAlloc.Get(), // Associated command allocator
        nullptr,                   // Initiall PipelineStateObject
        IID_PPV_ARGS(mCommandList.GetAddressOf())));
    // Start off in a closed state. This is because the first time we refer
    // to the command list we will Reset it, and it needs to be closed before
    // calling Reset.
    mCommandList->Close();
}

void D3DApp::CreateSwapChain() 
{
    // Release the previous swapchain we will be recreating.
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaState - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
        mCommandQueue.Get(),
        &sd,
        mSwapChain.GetAddressOf()));
}

// Allocate heap memory on the GPU for further descriptors
void D3DApp::CreateRtvAndDsvDescriptionHeaps() 
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::FlushCommandQueue() 
{
    // Advance the fence value to mark commands up to this fence point.
    mCurrentFence++;

    // Add an instruction to the command queue to set a new fence point. Because we
    // are on the GPU timeline, the new fence point won't be set until the GPU finishes
    // processing all the commands prior to this Signal().
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

    // Wait until the GPU has completed commands up to this fence point.
    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        
        // Wait until the GPU hits current fence event is fired.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void D3DApp::OnResize() 
{
    assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
    {
		mSwapChainBuffer[i].Reset();
    }
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

        // Next entry in heap.
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvView = DepthStencilView();
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, dsvView);

    // Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	
    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<f32>(mClientWidth);
	mScreenViewport.Height   = static_cast<f32>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

ID3D12Resource* D3DApp::CurrentBackBuffer() const
{
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView() const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats() 
{
    // Code computes the average frames per second, and also the
    // average time it takes to render one frame. 
    // These stats are appened to the window caption bar
    static i32 frameCnt = 0;
    static f32 timeElapsed = 0.f;

    frameCnt++;

    if ( ( mTimer.TotalTime() - timeElapsed ) >= 1.f) 
    {
        f32 fps = (f32)frameCnt; // fps = frameCnt 
        f32 mspf = 1000.f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);
        std::wstring windowText = mMainWndCaption + 
            L"   fps:  " + fpsStr +
            L"   mspf:  " + mspfStr;

        SetWindowText(mhMainWnd, windowText.c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.f;
    }
}

void D3DApp::LogAdapters() 
{
    u32 adapter_num = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (mdxgiFactory->EnumAdapters(adapter_num, &adapter) != DXGI_ERROR_NOT_FOUND) 
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
        adapterList.push_back(adapter);

        ++adapter_num;
    }

    for (size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter) 
{
    u32 i = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());
        LogOutputDisplayModes(output, mBackBufferFormat);
        ReleaseCom(output);
        ++i;
    }
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) 
{
    u32 count = 0;
    u32 flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);
    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& x : modeList)
    {
        u32 n = x.RefreshRate.Numerator;
        u32 d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}

/***********************************************************************************************************
 * @file DirectX.cpp
 *
 * @brief Implements functions of the DirectX class found in DirectX.h
 *
 * @ingroup Syren Render
 *
 * @author Ashvin Perera
 * Contact: ashvin.perera.97@gmail.com
 * 
 * @details
 * A graphics API provides the user with the ability to
 *  - Initialise the member variables
 *  - Query the available graphics adapters
 *  - Query the available output devices for a given adapter
 *  - Query the available device descriptions for a given output device
 *  - Run a render loop
 *  - Update the frames of a render loop
 * 
 **********************************************************************************************************/

#include "pch.h"
#include "DirectX.h"


 /***********************************************************************************************************
  * DirectX entry and exit member functions
  *
  **********************************************************************************************************/

/** Constructor for the DirectX class.
 *
 * @details
 * Initializes member variables. This prepares the DirectX object for further configuration and
 * initialisation.
 */
SyrenEngine::DirectX::DirectX(HWND phMainWnd) {
	mhMainWnd = phMainWnd;

	mFactory = nullptr;
	md3dDevice = nullptr;
	mFence = nullptr;

	mRtvDescriptorSize = 0;
	mDsvDescriptorSize = 0;
	mCbvSrvDescriptorSize = 0;

	md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
}

/** Destructor for the DirectX class.
 *
 * @details
 * Releases the DirectX COM objects if they have been created.
 */
SyrenEngine::DirectX::~DirectX() {
	if (md3dDevice != nullptr) {
		flushCommandQueue();
	}
	
	if (mFactory) {
		mFactory->Release();
		mFactory = nullptr;
	}
}

/***********************************************************************************************************
 * DirectX private member functions
 *
 **********************************************************************************************************/

/** Initializes the DXGI factory.
 *
 * @details
 * Creates the DXGIFactory object for a DirectX graphics interface through which graphics 
 * devices can be enumerated. If the factory cannot be created, an error message detailing
 * the failure will be returned.
 *
 * @retval FunctionResult containing the outcome of the factory initialisation.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseDXGI() {
	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)(&mFactory));

	if (FAILED(hr)) {
		std::string message = "Could not create GXGI Factory.";
		_com_error err(hr);
		std::wstring tempErrMsg = err.ErrorMessage();
		std::string errMsg = std::string(tempErrMsg.begin(), tempErrMsg.end());
		message = message + '\n' + errMsg;
		return(SyrenEngine::FunctionResult(false, RESULT::FAIL, message));
	}

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "DXGI initalised successfully."));
}

/** Initializes the D3D12 device.
 *
 * @details
 * Attempts to create the D3D12 device using the preferred graphics adapter. If failed,
 * it attempts to create a device using the WARP adapter. Returns a result indicating
 * the success or failure of the device creation attempt.
 *
 * @retval FunctionResult indicating the success of the D3D12 device initialisation.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseD3D12() {
	IDXGIAdapter* adapter;
	FunctionResult result = getAdapter(0, adapter);

	HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));

	if (FAILED(hr))
	{
		adapter->Release();
		hr = mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		
		if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create the warp adapter."));

		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
		if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create the D3D12 device."));

		return(FunctionResult(true, RESULT::SSUCCESS, "Successfully created the D3D12 device using a warp adapter."));
	}

	else {
		return(FunctionResult(true, RESULT::SSUCCESS, "Successfully created the D3D12 device."));
	}
		
}

/** Initializes the fence for synchronization.
 *
 * @details
 * Creates a fence object that allows for synchronization between the CPU and GPU
 * operations. Returns a result indicating the success or failure of the fence creation.
 *
 * @retval FunctionResult indicating the success of the fence initialisation.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseFence() {
	HRESULT hr = md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));

	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create a fence object."));
	
	return(FunctionResult(true, RESULT::SSUCCESS, "Successfully created a fence object."));
}

/** Caches the descriptor sizes for various Direct3D 12 object types.
 *
 * @details
 * Retrieves and stores the size of descriptor handles for RTV, DSV, and CBV/SRV/UAV
 * types for efficient GPU resource management.
 */
void SyrenEngine::DirectX::cacheDescriptorSizes() {
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/** Checks if multisampling is supported.
 *
 * @details
 * Queries the D3D12 device for the availability of 4X MSAA quality levels.
 * Returns a result indicating whether 4X MSAA is supported or not.
 *
 * @retval FunctionResult indicating the outcome of the multisampling check.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::checkMultisampling() {
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	
	HRESULT hr = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Feature check for 4X MSAA support failed."));
	
	if(msQualityLevels.NumQualityLevels <= 0) return(FunctionResult(false, RESULT::FAIL, "Unexpected MSAA quality level."));

	return(FunctionResult(true, RESULT::SSUCCESS, "4X MSAA Supported."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	
	HRESULT hr = md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create the primary command queue."));

	hr = md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf()));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create a command allocator for the primary command queue."));

	hr = md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
		mDirectCmdListAlloc.Get(), /*!< Associated command allocator */
		nullptr,                   /*!< Initial PipelineStateObject */ 
		IID_PPV_ARGS(mCommandList.GetAddressOf()));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create the command list for the primary command queue."));

	mCommandList->Close(); /*!< The command list will be reset the first time it is refered to */

	return(FunctionResult(true, RESULT::SSUCCESS, "Successfully created the primary command queue."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseSwapChain(const int rrNumerator, const int rrDenominator) {
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;

	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = rrNumerator;
	sd.BufferDesc.RefreshRate.Denominator = rrDenominator;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;

	sd.OutputWindow = mhMainWnd;
	
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr = mFactory->CreateSwapChain(mCommandQueue.Get(), &sd,mSwapChain.GetAddressOf());
	if (FAILED(hr)) {
		std::string message = "Failed to create the swap chain. \n";
		message += "Resolution: " + std::to_string(mClientWidth) + "x" + std::to_string(mClientHeight) + "\n";
		message += "Refresh Rate: " + std::to_string((rrNumerator/ rrDenominator)) + "\n";
		return(FunctionResult(false, RESULT::FAIL, message.data()));
	}

	return(FunctionResult(true, RESULT::SSUCCESS, "Successfully initialised the swap chain."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::initialiseRtvAndDsvDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	
	HRESULT hr = md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create descriptor heap (RTV)."));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	
	hr = md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()));
	if (FAILED(hr)) return(FunctionResult(false, RESULT::FAIL, "Failed to create descriptor heap (DSV)."));

	return(FunctionResult(true, RESULT::SSUCCESS, "Successfully created the render target view and depth stencil view."));
}

/** Retrieves the graphics adapter at a given index position.
 *
 * @details
 * The graphics adapter is selected from a list of available adapters that are ordered by their performance 
 * from highest performance to lowest performance. Index position 0 returns the adapter with the highest 
 * performance. 
 *
 * @param[in]  adapterIndex: Index positition of the adapter
 * @param[in]  pAdapter: Pointer to a graphics adapter (ideally a null pointer)
 * 
 * @retval FunctionResult containing the outcome of an attempt to get an adapter.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::getAdapter(int adapterIndex, IDXGIAdapter*& pAdapter) {
	if (mFactory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter), (void**)(&pAdapter)) != DXGI_ERROR_NOT_FOUND) {
		return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Adapter returned."));
	}

	pAdapter = nullptr;
	std::string message = "Could not find adapter at index position ";
	message = message + std::to_string(adapterIndex) + std::string(".");
	return(SyrenEngine::FunctionResult(false, RESULT::FAIL, message.data()));
}

/** Retrieves the output device at a specified index position of a specified adapter.
 *
 * @details
 * Obtains a specific output device based on its index for the given adapter.
 *
 * @param[in]  pAdapterIndex: Index of the graphics adapter.
 * @param[in]  pOutputIndex: Index of the output device.
 * @param[out] pOutput: Pointer that will receive the output device.
 *
 * @retval FunctionResult indicating the success or failure of retrieving the output.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::getOutput(int pAdapterIndex, int pOutputIndex, IDXGIOutput*& pOutput) {
	IDXGIAdapter* adapter;
	FunctionResult result = getAdapter(pAdapterIndex, adapter);

	if (!result.is_successfull) {
		return(result);
	}

	if (adapter->EnumOutputs(pOutputIndex, &pOutput) != DXGI_ERROR_NOT_FOUND) {
		return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Output device returned."));
	}

	adapter->Release();
	pOutput = nullptr;
	std::string message = "Could not find output device at index position ";
	message = message + std::to_string(pOutputIndex) + std::string(".");
	return(SyrenEngine::FunctionResult(false, RESULT::FAIL, message.data()));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::flushCommandQueue() {
	mCurrentFence++;

	HRESULT hr = mCommandQueue->Signal(mFence.Get(), mCurrentFence);
	if(FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to signal the command queue."));
	
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);

		hr = mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
		if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to fire event on fence completion."));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	
	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Successfully flushed the command queue."));
}

D3D12_CPU_DESCRIPTOR_HANDLE SyrenEngine::DirectX::DepthStencilView() const {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE SyrenEngine::DirectX::CurrentBackBufferView() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
}

ID3D12Resource* SyrenEngine::DirectX::CurrentBackBuffer() const {
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

/***********************************************************************************************************
 * DirectX public member functions
 *
 **********************************************************************************************************/

/** Retrieves a list of available graphics adapters.
 *
 * @details
 * Enumerates through available graphics adapters and stores their descriptions
 * in the provided list. Returns a result indicating the success of the operation.
 *
 * @param[out] adapters: List of graphics adapters discovered.
 *
 * @retval FunctionResult indicating the success or failure of retrieving the adapters.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::getAdapters(GraphicsAdapterList& adapters) {
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;

	while (mFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter), (void**)(&adapter)) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		std::wstring LadapterName = desc.Description;
		std::string adapterName = std::string(LadapterName.begin(), LadapterName.end());

		adapters.push_back(std::make_shared<GraphicsAdapter>(i, adapterName));
		++i;
	}

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Adapters returned."));
}

/** Retrieves the outputs for a specified adapter.
 *
 * @details
 * Enumerates the output devices connected to the given adapter and stores their
 * descriptions in the provided list.
 *
 * @param[in]  index: Index of the adapter to get outputs for.
 * @param[out] outputs: List that will contain descriptions of the output devices.
 *
 * @retval FunctionResult indicating the success or failure of retrieving the outputs.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::getOutputs(int index, GraphicsOutputList& outputs) {
	IDXGIAdapter* adapter = nullptr;
	FunctionResult result = getAdapter(index, adapter);

	UINT i = 0;
	IDXGIOutput* output = nullptr;

	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);
		std::wstring LoutputName = desc.DeviceName;
		std::string outputName = std::string(LoutputName.begin(), LoutputName.end());
		
		outputs.push_back(std::make_shared<GraphicsOutput>(i, outputName));
		++i;
	}

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Output devices returned."));
}

/** Retrieves a list of display modes available for a specific output device.
 *
 * @details
 * Enumerates the display modes connected to the given output device and stores their
 * resolution and refresh rate in the provided list.
 * 
 * @param[in] pAdapterIndex: The index of the graphics adapter.
 * @param[in] pOutputIndex: The index of the output device.
 * @param[out] pDisplayModes: A list to store the retrieved display modes.
 *
 * @return FunctionResult indicating success or failure of display mode retrieval.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::getDisplayModes(int pAdapterIndex, int pOutputIndex, DisplayModeList& pDisplayModes) {
	UINT count = 0;
	UINT flags = 0;
	
	IDXGIOutput* output = nullptr;
	FunctionResult result = getOutput(pAdapterIndex, pOutputIndex, output);
	
	if (!result.is_successfull) return(result);
	
	output->GetDisplayModeList(DXGI_FORMAT_R16G16B16A16_FLOAT, flags, &count, nullptr);
	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(DXGI_FORMAT_R16G16B16A16_FLOAT, flags, &count, &modeList[0]);
	
	for (int i = 0; i < modeList.size(); i++)
	{
		UINT n = modeList[i].RefreshRate.Numerator;
		UINT d = modeList[i].RefreshRate.Denominator;
		
		pDisplayModes.push_back(std::make_shared<DisplayMode>(i, modeList[i].Width, modeList[i].Height, int(n / d)));
	}

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Display modes returned."));
}

/** Initializes DirectX.
 *
 * @details
 * Calls the necessary initialization functions in sequence and
 * checks for successful completion of each step.
 *
 * @return FunctionResult indicating success or failure of the initialization process.
 */
SyrenEngine::FunctionResult SyrenEngine::DirectX::initialise() {
	std::string message = "Initialising DirectX. \n";
	
	SyrenEngine::FunctionResult initialised = initialiseDXGI();
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;
	

	initialised = initialiseD3D12();
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;

	initialised = initialiseFence();
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;

	cacheDescriptorSizes();

	initialised = initialiseCommandObjects();
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;

	initialised = initialiseSwapChain(60, 1);
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;

	initialised = initialiseRtvAndDsvDescriptorHeaps();
	message += initialised.message + "\n";
	if (!initialised.is_successfull) return initialised;

	message += "Initialising DirectX was successful.";

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, message));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::onResize() {
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	FunctionResult result = flushCommandQueue();
	if (!result.is_successfull) return result;

	HRESULT hr = mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to reset command list."));

	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	hr = mSwapChain->ResizeBuffers(SwapChainBufferCount, mClientWidth, mClientHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to resize swap chain buffers."));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		hr = mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
		if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to get a buffer from the swap chain."));

		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	
	hr = md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
	);
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to create a commited resource."));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	hr = mCommandList->Close();
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to close the command list."));

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	result = flushCommandQueue();
	if (!result.is_successfull) return(result);

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Succseeful."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::render() {
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);
	
	HRESULT hr = mDirectCmdListAlloc->Reset();	
	if(FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to reset the command list allocator."));

	hr = mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr);
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to reset the command list."));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = mCommandList->Close();
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to close the command list."));

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	hr = mSwapChain->Present(0, 0);
	if (FAILED(hr)) return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Failed to present the swap chain."));

	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	flushCommandQueue();

	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Successfully rendered frame."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::update() {
	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Succseeful."));
}

SyrenEngine::FunctionResult SyrenEngine::DirectX::destroy() {
	return(SyrenEngine::FunctionResult(true, RESULT::SSUCCESS, "Succseeful."));
}


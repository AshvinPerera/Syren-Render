/***********************************************************************************************************
 * @file DirectX.h
 * 
 * @brief Initialises and implements Direct X as the underlying graphics API for the Syren Render Engine
 *
 * @ingroup Syren Render
 * 
 * @author Ashvin Perera
 * Contact: ashvin.perera.97@gmail.com
 * 
 **********************************************************************************************************/


#pragma once
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d12.lib")


#include "./D3DX12/d3dx12.h"

#include <Windows.h>
#include <wrl.h>

#include <comdef.h>
#include <comutil.h>
#include <combaseapi.h>

#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include <DirectXColors.h>

#include <memory>
#include <vector>
#include <string>

#include "GraphicsAPI.h"
#include "common.h"

using namespace DirectX;


namespace SyrenEngine {
	class DirectX : public GraphicsAPI {
	private:
		HWND mhMainWnd;
		
		int mClientWidth = 800;
		int mClientHeight = 600;
		
		DXGI_FORMAT mBackBufferFormat;
		DXGI_FORMAT  mDepthStencilFormat;
		D3D_DRIVER_TYPE md3dDriverType;

		UINT64 mCurrentFence = 0;

		bool m4xMsaaState = false; /*!< 4X MSAA enabled */
		UINT m4xMsaaQuality = 0;   /*!< Quality level of 4X MSAA */   

		static const int SwapChainBufferCount = 2;
		int mCurrBackBuffer = 0;

		Microsoft::WRL::ComPtr<IDXGIFactory6> mFactory;
		Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

		Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

		D3D12_VIEWPORT mScreenViewport;
		D3D12_RECT mScissorRect;

		UINT mRtvDescriptorSize;
		UINT mDsvDescriptorSize;
		UINT mCbvSrvDescriptorSize;
	public:
		DirectX(HWND phMainWnd);
		~DirectX();

		virtual FunctionResult initialise();
		virtual FunctionResult onResize();
		virtual FunctionResult render();
		virtual FunctionResult update();
		virtual FunctionResult destroy();

		FunctionResult getAdapters(GraphicsAdapterList& adapters);
		FunctionResult getOutputs(int index, GraphicsOutputList& outputs);
		FunctionResult getDisplayModes(int pAdapterIndex, int pOutputIndex, DisplayModeList& pDisplayModes);
	private:
		DirectX() = delete;
		DirectX(const DirectX& rhs) = delete;
		DirectX& operator=(const DirectX& rhs) = delete;
		
		FunctionResult initialiseDXGI();
		FunctionResult initialiseD3D12();
		FunctionResult initialiseFence();
		FunctionResult initialiseCommandObjects();
		FunctionResult initialiseSwapChain(const int rrNumerator, const int rrDenominator);
		FunctionResult initialiseRtvAndDsvDescriptorHeaps();

		void cacheDescriptorSizes();
		FunctionResult checkMultisampling();
		FunctionResult flushCommandQueue();
		
		FunctionResult getAdapter(int index, IDXGIAdapter*& pAdapter);
		FunctionResult getOutput(int pAdapterIndex, int pOutputIndex, IDXGIOutput*& pOutput);

		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
		ID3D12Resource* CurrentBackBuffer()const;
	};
}



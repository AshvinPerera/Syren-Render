#ifdef SYRENRENDER_EXPORTS
#define SYRENRENDER_API __declspec(dllexport)
#else
#define SYRENRENDER_API __declspec(dllimport)
#endif

#pragma once

#include <memory>

#include "common.h"
#include "GraphicsAPI.h"
#include "DirectX.h"


namespace SyrenEngine {
	class SYRENRENDER_API SyrenRender {
	private:
		bool m_is_initialised;
		
		GraphicsConfig m_config;
		std::shared_ptr<GraphicsAPI> m_API;
	public:
		SyrenRender(void);

		FunctionResult initialise(HWND phMainWnd);
		FunctionResult onResize(void);
		FunctionResult draw();

		bool isInitialised() const;

		FunctionResult getAdapters(GraphicsAdapterList& adapters);
		FunctionResult getOutputs(int index, GraphicsOutputList& adapters);
		FunctionResult getDisplayModes(int adapterIndex, int OutputIndex, DisplayModeList& displayModes);
	private:
		std::shared_ptr<GraphicsAPI> select_api(SyrenEngine::API p_api, HWND phMainWnd);
		FunctionResult loadConfig(GraphicsConfig& config);
	};

}



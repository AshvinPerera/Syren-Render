#ifdef SYRENRENDER_EXPORTS
#define SYRENRENDER_API __declspec(dllexport)
#else
#define SYRENRENDER_API __declspec(dllimport)
#endif

#pragma once

#include <string>
#include <fstream>
#include <sstream>

#include <vector>


namespace SyrenEngine {
	enum class API { DIRECTX, OPENGL, VULKAN, NONE };
	enum class RESULT { FAIL = 0, WSUCCESS = 1, SSUCCESS = 2 };

	struct SYRENRENDER_API FunctionResult {
		bool is_successfull;
		RESULT result;
		std::string message;

		FunctionResult(bool p_is_successfull, RESULT p_result, const char* p_message) : is_successfull(p_is_successfull), result(p_result), message(p_message) {};
		FunctionResult(bool p_is_successfull, RESULT p_result, const std::string p_message) : is_successfull(p_is_successfull), result(p_result), message(p_message) {};
	};

	struct GraphicsConfig {
		API GraphicsAPI;
	};

	struct GraphicsAdapter {
		int index;
		std::string adapterName;

		GraphicsAdapter(int pIndex, std::string pAdapterName) : index(pIndex), adapterName(pAdapterName) {};
	};

	struct GraphicsOutput {
		int index;
		std::string outputName;

		GraphicsOutput(int pIndex, std::string pOutputName) : index(pIndex), outputName(pOutputName) {};
	};

	struct DisplayMode {
		int index;
		unsigned int resolutionWidth;
		unsigned int resolutionHeight;
		unsigned int refreshRate;
		
		DisplayMode(int pIndex, unsigned int pResolutionWidth, unsigned int pResolutionHeight, unsigned int pRefreshRate) 
			: index(pIndex), resolutionWidth(pResolutionWidth), resolutionHeight(pResolutionHeight), refreshRate(pRefreshRate) {};
	};

	typedef std::vector<std::shared_ptr<GraphicsAdapter> > SYRENRENDER_API GraphicsAdapterList;
	typedef std::vector<std::shared_ptr<GraphicsOutput> > SYRENRENDER_API GraphicsOutputList;
	typedef std::vector<std::shared_ptr<DisplayMode> > SYRENRENDER_API DisplayModeList;
}

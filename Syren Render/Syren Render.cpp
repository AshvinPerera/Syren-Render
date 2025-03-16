#include <fstream>

#include "pch.h"
#include "framework.h"

#include "Syren Render.h"


SyrenEngine::SyrenRender::SyrenRender() {
    m_is_initialised = FALSE;
    m_config.GraphicsAPI = API::NONE;
}

bool SyrenEngine::SyrenRender::isInitialised() const {
    return m_is_initialised;
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::initialise(HWND phMainWnd) {
    FunctionResult initialised = loadConfig(m_config);
    if (initialised.is_successfull) {
        if (initialised.result == RESULT::SSUCCESS) { 
            m_API = select_api(m_config.GraphicsAPI, phMainWnd);
            initialised = m_API->initialise();
            m_is_initialised = TRUE;
            return (initialised);
        }
    }

    m_config.GraphicsAPI = API::DIRECTX;
    m_API = select_api(API::DIRECTX, phMainWnd);
    initialised = m_API->initialise();
    m_is_initialised = TRUE;

    return (initialised);
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::onResize() {
    FunctionResult result = m_API->onResize();
    if (!result.is_successfull) return(result);

    return (FunctionResult(true, RESULT::SSUCCESS, "Successfully resized window."));
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::draw() {
    return(m_API->render());
}

std::shared_ptr<SyrenEngine::GraphicsAPI> SyrenEngine::SyrenRender::select_api(SyrenEngine::API p_api, HWND phMainWnd) {
    return(std::make_shared<SyrenEngine::DirectX>(phMainWnd));
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::getAdapters(GraphicsAdapterList& adapters) {
    if (m_is_initialised) {
        return(m_API->getAdapters(adapters));
    }
    return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Graphics API has not been initialised."));
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::getOutputs(int index, GraphicsOutputList& outputs) {
    if (m_is_initialised) {
        return(m_API->getOutputs(index, outputs));
    }
    return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Graphics API has not been initialised."));
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::getDisplayModes(int adapterIndex, int OutputIndex, DisplayModeList& displayModes) {
    if (m_is_initialised) {
        return(m_API->getDisplayModes(adapterIndex, OutputIndex, displayModes));
    }
    return(SyrenEngine::FunctionResult(false, RESULT::FAIL, "Graphics API has not been initialised."));
}

SyrenEngine::FunctionResult SyrenEngine::SyrenRender::loadConfig(GraphicsConfig& config) {
    std::ifstream fconfig("render.cfg");

    if (fconfig.is_open()) {
        std::string line;
        while (getline(fconfig, line)) {
            std::istringstream sin(line.substr(line.find(":") + 1));
            if (line.find("api") != std::string::npos) {

                std::string api_check;
                sin >> api_check;

                if (api_check == std::string("directx")) config.GraphicsAPI = API::DIRECTX;
                else if (api_check == std::string("opengl")) config.GraphicsAPI = API::OPENGL;
                else if (api_check == std::string("vulkan")) config.GraphicsAPI = API::VULKAN;
                else { fconfig.close(); return(FunctionResult(false, RESULT::FAIL, "invalid graphics API entry in render.config.")); }
            }
        }

        fconfig.close();

        if (config.GraphicsAPI == API::NONE) return(FunctionResult(true, RESULT::WSUCCESS, "missing graphics API entry in render.config."));

        return(FunctionResult(true, RESULT::SSUCCESS, "config file loaded successfully."));
    }

    std::string message = "Error opening file: render.cfg";
    if (fconfig.bad()) message = message + '\n' + "Fatal error : badbit is set.";
    if (fconfig.fail()) {
        char buffer[1024];
        strerror_s(buffer, sizeof(buffer), errno);
        message = message + '\n' + "Error details: " + buffer;
    }
    return(FunctionResult(false, RESULT::FAIL, message));
}


#pragma once

#include "common.h"


namespace SyrenEngine {
    class GraphicsAPI {
    public:
        virtual FunctionResult initialise() = 0;
        virtual FunctionResult onResize() = 0;
        virtual FunctionResult render() = 0;
        virtual FunctionResult update() = 0;
        virtual FunctionResult destroy() = 0;

        virtual FunctionResult getAdapters(GraphicsAdapterList& adapters) = 0;
        virtual FunctionResult getOutputs(int index, GraphicsOutputList& adapters) = 0;
        virtual FunctionResult getDisplayModes(int adapterIndex, int outputIndex, DisplayModeList& displayModes) = 0;
    };
}


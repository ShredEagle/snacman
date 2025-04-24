#pragma once

#include "imguiui/ImguiUi.h"
#include "reflexion/Concept.h"
#include "ImguiSerialization.h"

template<class T_renderable>
concept ImguiDefaultRenderable = requires(const char * n, T_renderable v) {
    debugRender(n, v);
};

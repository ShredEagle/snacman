#pragma once

#include "ImguiSerialization.h"

#include "imguiui/ImguiUi.h"
#include <reflexion/Concept.h>

template<class T_renderable>
concept ImguiDefaultRenderable = requires(const char * n, T_renderable v) {
    debugRender(n, v);
};

#pragma once


namespace ad::renderer {


constexpr unsigned int gSdfSpread = 8;


const std::vector<graphics::MacroDefine> gClientConstantDefines{
    "CLIENT_SDF_DOUBLE_SPREAD " + std::to_string(2 * gSdfSpread),
};


} // namespace ad::renderer
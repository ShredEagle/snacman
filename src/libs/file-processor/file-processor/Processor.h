#pragma once


#include <filesystem>
#include <string_view>


namespace ad::renderer {


// TODO Rewrite as a class, there is state to be maintained accross the implementating functions.
void processModel(const std::filesystem::path & aFile);


} // namespace ad::renderer
#pragma once


namespace ad::renderer {


class LoggerInitialization
{
private:
    // __declspec is not forcing the export for static libs, apparently.
    /*__declspec(dllexport)*/ static const LoggerInitialization gInitialized;
    LoggerInitialization();
};


} // namespace ad::renderer


extern "C"
{
void ad_renderer_loggerinitialization();
}
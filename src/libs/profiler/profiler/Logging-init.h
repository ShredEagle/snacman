#pragma once


namespace ad::profiler {


class LoggerInitialization
{
private:
    // __declspec is not forcing the export for static libs, apparently.
    /*__declspec(dllexport)*/ static const LoggerInitialization gInitialized;
    LoggerInitialization();
};


} // namespace ad::profiler


extern "C"
{
void ad_profiler_loggerinitialization();
}
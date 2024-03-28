#pragma once


namespace ad::processor {


class LoggerInitialization
{
private:
    static const LoggerInitialization gInitialized;
    LoggerInitialization();
};


} // namespace ad::processor


extern "C"
{
void ad_processor_loggerinitialization();
}
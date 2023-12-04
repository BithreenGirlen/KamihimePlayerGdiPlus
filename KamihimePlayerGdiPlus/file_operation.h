#ifndef FILE_OPERATION_H_
#define FILE_OPERATION_H_

#include <shobjidl.h>
#include <atlbase.h>

struct ComInit
{
    HRESULT hr;
    ComInit() : hr(::CoInitialize(nullptr)) {}
    ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};

wchar_t* SelectWorkingFolder();

#endif //FILE_OPERATION_H_
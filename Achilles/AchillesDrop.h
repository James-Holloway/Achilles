#pragma once

#include <vector>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

class Achilles;

class AchillesDrop : public IDropTarget
{
protected:
    volatile unsigned long referenceCount;
    HWND hWnd;

public:
    AchillesDrop(HWND _hWnd);

    std::vector<std::wstring> GetFiles(IDataObject* pDataObj);

    //Override this to create custom conditions for the FORMATETC
    virtual bool QueryDataObject(IDataObject* pDataObj);

    // Inherited via IDropTarget
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
    virtual ULONG __stdcall AddRef(void) override;
    virtual ULONG __stdcall Release(void) override;
    virtual HRESULT __stdcall DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    virtual HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    virtual HRESULT __stdcall DragLeave(void) override;
    virtual HRESULT __stdcall Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
};


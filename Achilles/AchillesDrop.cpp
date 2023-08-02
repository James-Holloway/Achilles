#include "AchillesDrop.h"
#include "Achilles.h"

AchillesDrop::AchillesDrop(HWND _hWnd) : hWnd(_hWnd), referenceCount(1UL)
{

}

std::vector<std::wstring> AchillesDrop::GetFiles(IDataObject* pDataObj)
{
    FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM med;
    pDataObj->GetData(&fmtetc, &med);
    HDROP hdrop = reinterpret_cast<HDROP>(med.hGlobal);

    UINT fileCount = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);
    std::vector<std::wstring> files;
    files.reserve(fileCount);
    for (UINT i = 0; i < fileCount; i++)
    {
        WCHAR szFile[MAX_PATH];
        UINT cch = DragQueryFileW(hdrop, i, szFile, MAX_PATH);
        if (cch > 0 && cch < MAX_PATH) {
            files.push_back(szFile);
        }
    }
    ReleaseStgMedium(&med);
    return files;
}

bool AchillesDrop::QueryDataObject(IDataObject* pDataObject)
{
    // Does the data object support CF_HDROP using a HGLOBAL?
    FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    return pDataObject->QueryGetData(&fmtetc) == S_OK ? true : false;
}

HRESULT __stdcall AchillesDrop::QueryInterface(REFIID riid, void** ppvObject)
{
    // Always set out parameter to NULL, validating it first.
    if (!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;
    if (riid == IID_IUnknown)
    {
        // Increment the reference count and return the pointer.
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG __stdcall AchillesDrop::AddRef(void)
{
    InterlockedIncrement(&referenceCount);
    return referenceCount;
}

ULONG __stdcall AchillesDrop::Release(void)
{
    // Decrement the object's internal counter.
    ULONG ulRefCount = InterlockedDecrement(&referenceCount);
    if (referenceCount == 0)
    {
        delete this;
    }
    return ulRefCount;
}

HRESULT __stdcall AchillesDrop::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    Achilles* instance = Achilles::GetAchillesInstance(hWnd);
    if (instance && instance->acceptingFiles)
    {
        *pdwEffect = DROPEFFECT_LINK;
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

HRESULT __stdcall AchillesDrop::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    *pdwEffect = DROPEFFECT_LINK;
    return S_OK;
}

HRESULT __stdcall AchillesDrop::DragLeave(void)
{
    return S_OK;
}

HRESULT __stdcall AchillesDrop::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    Achilles* instance = Achilles::GetAchillesInstance(hWnd);
    if (instance && instance->acceptingFiles)
    {
        std::vector<std::wstring> files = GetFiles(pDataObj);
        instance->HandleDroppedFiles(files);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

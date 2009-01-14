#include "ktest.h"
#include "sip.h"
#include "ids.h"
//#include "crt.h"

void D(LPCWSTR fmt, ... )
{
    static WCHAR       buf[8192];
    va_list     ap;

    va_start( ap, fmt );

    _vsnwprintf( buf, sizeof(buf)/2-1, fmt, ap );
    buf[sizeof(buf)/2-1] = L'\0';

    MessageBox( 0, buf, L"D", 0 );
    va_end( ap );
}


HWND hwndLog;
void LOG(LPCWSTR fmt, ... )
{
    va_list     ap;
    int len = GetWindowTextLength(hwndLog);
    PWCHAR pw = new WCHAR[len+8192];
    len = GetWindowText(hwndLog, pw, len+1);
    pw[len++] = '\r';
    pw[len++] = '\n';
    pw[len] = '\0';

    va_start( ap, fmt );
    int x = _vsnwprintf( pw+len, 8192/2-1, fmt, ap );
    if(x>0)
        len += x;
    pw[len] = L'\0';
    va_end( ap );

    if( len > 8192 )
    {
        SetWindowText(hwndLog, pw+len-8192);
    } else
    {
        SetWindowText(hwndLog, pw);
    }
    delete[] pw;
}

class CCallback : public IIMCallback
{
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppobj)
    {
        if( IID_IUnknown == riid || IID_IIMCallback == riid )
        {
            *ppobj = this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 1;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE SetImInfo(IMINFO __RPC_FAR *pimi)
    {
        LOG(L"SetImInfo pimi->fdwFlags     = 0x%X", pimi->fdwFlags     );
        LOG(L"SetImInfo pimi->hImageNarrow = 0x%X", pimi->hImageNarrow );
        LOG(L"SetImInfo pimi->hImageWide   = 0x%X", pimi->hImageWide   );
        LOG(L"SetImInfo pimi->iNarrow      = 0x%X", pimi->iNarrow      );
        LOG(L"SetImInfo pimi->iWide        = 0x%X", pimi->iWide        );
        LOG(L"SetImInfo pimi->rcSipRect    = 0x%X", pimi->rcSipRect    );
        return NOERROR;
    }

    virtual HRESULT STDMETHODCALLTYPE SendVirtualKey(BYTE bVK, DWORD dwFlags)
    {
        LOG(L"SendVirtualKey(0x%02X, 0x%08X)", bVK, dwFlags);
        return NOERROR;
    }

    virtual HRESULT STDMETHODCALLTYPE SendCharEvents(
        UINT uVK,
        UINT uKeyFlags,
        UINT uChars,
        UINT *puShift,
        UINT *puChars)
    {
        LOG(L"SendCharEvents(0x%02X, 0x%08X, 0x%02X, 0x%08X, 0x%02X)", uVK, uKeyFlags, uChars, puShift[0], puChars[0]);
        return NOERROR;
    }

    virtual HRESULT STDMETHODCALLTYPE SendString(BSTR ptszStr, DWORD dwChars)
    {
        LOG(L"SendString(%s, 0x%08X)", ptszStr, dwChars);
        return NOERROR;
    }

};

CCallback g_callback;

typedef HRESULT (STDAPICALLTYPE *TDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
IInputMethod* LoadKbd(HWND hWnd)
{
    HINSTANCE hLib = LoadLibrary(L"eurokbd-x86.dll");
    if(!hLib)
    {
        MessageBox(0, L"LoadLibrary", L"ERROR", 0);
        return 0;
    }

    TDllGetClassObject pfnDllGetClassObject = (TDllGetClassObject)GetProcAddress(hLib, "DllGetClassObject");
    if(!pfnDllGetClassObject)
    {
        MessageBox(0, L"GetProcAddress", L"ERROR", 0);
        return 0;
    }

    IClassFactory* pCF;
    HRESULT hr = pfnDllGetClassObject(CLSID_EuroKbd, IID_IClassFactory, (LPVOID*)&pCF);
    if(FAILED(hr))
    {
        MessageBox(0, L"GetClassObject", L"ERROR", 0);
        return 0;
    }

    IInputMethod* pIM;
    hr = pCF->CreateInstance(0, IID_IInputMethod, (void**)&pIM);
    if(FAILED(hr))
    {
        MessageBox(0, L"CreateInstance", L"ERROR", 0);
        pCF->Release();
        return 0;
    }
    pCF->Release();

    hr = pIM->RegisterCallback( &g_callback );
    if(FAILED(hr))
    {
        MessageBox(0, L"RegisterCallback", L"ERROR", 0);
        pIM->Release();
        return 0;
    }

    IMINFO iminfo;
    ZeroMemory(&iminfo, sizeof(IMINFO));
    iminfo.cbSize = sizeof(IMINFO);
    pIM->GetInfo(&iminfo);
//  D(L"iminfo.fdwFlags     = 0x%X\n", iminfo.fdwFlags     ); // vgakey=3 SIPF_ON|SIPF_DOCKED
//  D(L"iminfo.hImageNarrow = 0x%X\n", iminfo.hImageNarrow );
//  D(L"iminfo.hImageWide   = 0x%X\n", iminfo.hImageWide   );
//  D(L"iminfo.iNarrow      = 0x%X\n", iminfo.iNarrow      );
//  D(L"iminfo.iWide        = 0x%X\n", iminfo.iWide        );
//  D(L"iminfo.rcSipRect    = (%d,%d,%d,%d)\n",
//      iminfo.rcSipRect.top,
//      iminfo.rcSipRect.bottom,
//      iminfo.rcSipRect.left,
//      iminfo.rcSipRect.right
//      );

    pIM->Select(hWnd);

    SIPINFO si;
    ZeroMemory(&si, sizeof(SIPINFO));
    si.cbSize = sizeof(SIPINFO);
    GetWindowRect(hWnd, &si.rcSipRect);
    int iHeight = 200; //iminfo.rcSipRect.bottom - iminfo.rcSipRect.top;
    if(iHeight)
    {
        si.rcSipRect.top = si.rcSipRect.bottom - iHeight;
    }
    pIM->ReceiveSipInfo(&si);


//  iim->Showing();
//  iim->Release();
//
//  icf->Release();
    return pIM;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            IInputMethod* pIM = LoadKbd(hWnd);
            pIM->Release();
            pIM = LoadKbd(hWnd);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}



int main()
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    HINSTANCE hInstance = LoadLibrary(0);
    wc.hInstance     = hInstance;
    wc.hCursor = LoadCursor(LoadLibrary(L"USER32"), MAKEINTRESOURCE(100));
    wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
    wc.lpszClassName = L"KTestCLASS";

    if( !RegisterClass(&wc) )
    {
        MessageBox(0, L"RegisterClass", L"ERROR", 0);
        return 0;
    }

    HWND hWnd = CreateWindow(wc.lpszClassName, L"KTest", WS_OVERLAPPEDWINDOW,
        600, 400, 660, 800, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(0, L"CreateWindow", L"ERROR", 0);
        return 0;
    }

    hwndLog = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 160, 640, 600, hWnd, NULL, hInstance, NULL);
    if (!hwndLog)
    {
        MessageBox(0, L"CreateWindow(EDIT)", L"ERROR", 0);
        return 0;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

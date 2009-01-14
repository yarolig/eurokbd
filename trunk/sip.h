#ifdef _WIN32_WCE
#include <sip.h>
#else

#define SIPF_OFF    0x00000000
#define SIPF_ON     0x00000001
#define SIPF_DOCKED 0x00000002
#define SIPF_LOCKED 0x00000004

#define SPI_SETCOMPLETIONINFO   223
#define SPI_SETSIPINFO          224
#define SPI_GETSIPINFO          225
#define SPI_SETCURRENTIM        226
#define SPI_GETCURRENTIM        227
#define SPI_SIPMOVE         250

typedef struct  tagSIPINFO
{
    DWORD   cbSize;
    DWORD   fdwFlags;
    RECT    rcVisibleDesktop;
    RECT    rcSipRect;
    DWORD   dwImDataSize;
    void   *pvImData;
} SIPINFO;

typedef struct  _tagImInfo
    {
    DWORD cbSize;
    HANDLE hImageNarrow;
    HANDLE hImageWide;
    int iNarrow;
    int iWide;
    DWORD fdwFlags;
    RECT rcSipRect;
    }   IMINFO;


interface DECLSPEC_UUID("42429669-ae04-11d0-a4f8-00aa00a749b9")
IIMCallback : public IUnknown
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetImInfo(
        IMINFO __RPC_FAR *pimi) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendVirtualKey(
        BYTE bVK,
        DWORD dwFlags) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendCharEvents(
        UINT uVK,
        UINT uKeyFlags,
        UINT uChars,
        UINT __RPC_FAR *puShift,
        UINT __RPC_FAR *puChars) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendString(
        BSTR ptszStr,
        DWORD dwChars) = 0;

};

interface DECLSPEC_UUID("42429666-ae04-11d0-a4f8-00aa00a749b9")
IInputMethod : public IUnknown
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Select(
        /* [in] */ HWND hwndSip) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Deselect( void) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Showing( void) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Hiding( void) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetInfo(
        /* [out] */ IMINFO __RPC_FAR *pimi) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReceiveSipInfo(
        /* [in] */ SIPINFO __RPC_FAR *psi) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterCallback(
        /* [in] */ IIMCallback __RPC_FAR *lpIMCallback) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetImData(
        /* [in] */ DWORD dwSize,
        /* [out] */ void __RPC_FAR *pvImData) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetImData(
        /* [in] */ DWORD dwSize,
        /* [in] */ void __RPC_FAR *pvImData) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE UserOptionsDlg(
        /* [in] */ HWND hwndParent) = 0;

};
#endif

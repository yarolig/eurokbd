#include "eurokbd.h"
#include "regkey.h"
#include "eurokbd_rc.h"
#include "sip.h"
#include "ids.h"
//#include "crt.h"
#include "eurokbd_config.h"


// таймеры
#define IDT_POPUP_DELAY         101 // задержка до появления попапа
#define IDT_AUTOREPEAT          102 // обе задержки автоповтора (до первого срабатываняи чуть дольше чем для последующих)
//#define IDT_PRESSED           103 // минимальное время в течении которого кнопка отображается в нажатом состоянии
                                    // если отпускание кнопки произошло слишком быстро, отрисовка ее отжатой случится только через это время
#define IDT_CLOCKWATCH          104 // идущие часики на кнопке "вставить время"

//#define PAINT_VIA_BITMAP

LONG g_dwObjectCount = 0;
static HINSTANCE  g_hInstDll = 0;
static WCHAR*     g_pwDllFile;
static WCHAR*     g_pwDllDir;


static void DrawButton(HDC hdc, RECT& r, HBRUSH hBrushBg /*,HPEN hPenTL, HPEN hPenBR*/)
{
    FillRect(hdc, &r, hBrushBg);
	//RECT r_ = r;
    //HBRUSH oldBr = HBRUSH(SelectObject(hdc, hBrushBg));
    DrawEdge(hdc, &r, BDR_RAISEDINNER/*|BDR_RAISEDOUTER*/, BF_RECT|BF_SOFT);
	//SelectObject(hdc, oldBr);
    /*const POINT pntsTL[3] = {
        {r.left,    r.bottom-2},
        {r.left,    r.top},
        {r.right-1, r.top},
    };
    SelectObject(hdc, hPenTL);
    Polyline(hdc, pntsTL, 3);

    const POINT pntsBR[3] = {
        {r.left,    r.bottom-1},
        {r.right-1, r.bottom-1},
        {r.right-1, r.top-1},
    };
    SelectObject(hdc, hPenBR);
    Polyline(hdc, pntsBR, 3);*/
}


class CKbd : public IInputMethod
{
            LONG                m_cRef;

            HIMAGELIST          m_hilWide;
            HIMAGELIST          m_hilNarrow;

            // окна и объекты GDI - создаются в Select (или после по мере надобности) и убиваются в Deselect
            HWND                m_hwndMain;
            HWND                m_hwndPop;
            CGdiObj<HIMAGELIST> m_hilSymbols16;
            CGdiObj<HIMAGELIST> m_hilSymbols32;
            CGdiObj<HFONT>      m_hFontBig;
            CGdiObj<HFONT>      m_hFontSmall;
            CGdiObj<HFONT>      m_hFontIndex;
            CGdiObj<HBITMAP>    m_hbmSkin;

            // динамика состояния
            IIMCallback*        m_pIMCallback;
            KEYENTRY*           m_keyCurrent;
            #define SUBKEY_PARENT ((SUBKEYENTRY*)1) // magic
            //#define SUBKEY_UNKNOWN ((SUBKEYENTRY*)2) // magic
            SUBKEYENTRY*        m_subkeyCurrent;
            bool                m_bPopupActive;
            //PWCHAR              m_pwCurrentConfigName;
            CPtr<WCHAR>         m_pwCurrentConfigName;
            POINT               m_ptBtnDownPos;  // точка в которой сталус начал свой путь
            POINT               m_ptCursorPos;   // т.к. в Windows Mobile не работает GetCursorPos, приходицца делать вот так :(
            POINT               m_ptPopShift;
            BYTE                m_vga; // 1 or 2, масштабирующий коэфф
            RECT                m_rsi;
            RECT                m_rsiVD;

            // конфиг
            CPtr<CKbdConfig>    m_config;

            // конфиг в реестре (параметры не связанные с каким-то скином)
            bool                m_bShowControl;
            bool                m_bShowGreek;
            bool                m_bShowLatinEx;
            bool                m_bShowCyrEx;

            LONG                m_nPopupDelay; // ms
            LONG                m_nAutorepeatDelay;
            LONG                m_nAutorepeatPeriod;


    static  const WCHAR         s_szMainWndClassName[];
    static  const WCHAR         s_szPopWndClassName[];
public:

    CKbd() :
        m_cRef(0)
    {
        LOG(L"CKbd::CKbd()");
        G_OBJECTCOUNT++;

        m_hilWide   = 0;
        m_hilNarrow = 0;

        m_hwndMain         = 0;
        m_hwndPop          = 0;
        m_pIMCallback      = 0;


        m_ptPopShift.x = 0;
        m_ptPopShift.y = 0;
        m_keyCurrent       = 0;
        m_subkeyCurrent    = 0;

        m_bPopupActive = false;

        m_vga              = 1+1;
        m_rsi.left = m_rsi.top = m_rsi.right = m_rsi.bottom = 0;
        m_rsiVD.left = m_rsiVD.top = m_rsiVD.right = m_rsiVD.bottom = 0;

        m_bShowControl   = true;
        m_bShowGreek     = true;
        m_bShowLatinEx   = true;
        m_bShowCyrEx     = true;
        m_nPopupDelay       =  300;
        m_nAutorepeatDelay  = 2000;
        m_nAutorepeatPeriod =  200;

        m_ptBtnDownPos.x = 0;
        m_ptBtnDownPos.y = 0;
        m_ptCursorPos.x  = 0;
        m_ptCursorPos.y  = 0;
        LoadConfig(L"lat");
    }

    void DestroyGdiObjects()
    {
        if(m_hwndMain)
        {
            DestroyWindow( m_hwndMain );
            m_hwndMain = 0;
        }
        UnregisterClass( s_szMainWndClassName, g_hInstDll );

        if(m_hwndPop)
        {
            DestroyWindow( m_hwndPop );
            m_hwndPop = 0;
        }
        UnregisterClass( s_szPopWndClassName, g_hInstDll );

        m_hFontBig     = 0;
        m_hFontSmall   = 0;
        m_hFontIndex   = 0;
        m_hilSymbols16 = 0;
        m_hilSymbols32 = 0;
        m_hbmSkin      = 0;
    }


    ~CKbd()
    {
        LOG(L"CKbd::~CKbd()");
        G_OBJECTCOUNT++;
        DestroyGdiObjects();
        //free(m_pwCurrentConfigName);
        //delete m_config;
    }


    STDMETHOD_(ULONG,AddRef)()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHOD_(ULONG,Release)()
    {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if( 0==cRef )
            delete this;
        return cRef;
    }

    STDMETHOD(QueryInterface)(REFIID riid,void **ppvObject)
    {
        if(IsEqualIID(riid, IID_IUnknown) ||
           IsEqualIID(riid, IID_IInputMethod))
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

protected:

    // т.к. в Windows Mobile не работает GetCursorPos, приходицца делать вот так :(
    void SaveCursorPos(HWND hwnd, LPARAM lParam)
    {
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        //m_ptCursorClientPos = pt;
        if( ClientToScreen(hwnd, &pt) )
        {
            m_ptCursorPos = pt;
        }
    }


    void DrawText(HDC hdc, keydata kd, LPRECT lpRect, UINT uFormat, COLORREF fgColor)
    {
        if( kd.w <= 0xFFFF)
        {
            int i = -1;
            switch( kd.w )
            {
            case 0x000D: i=0; break; // enter
            case 0x2190: i=3; break; // leftward arrow
            case 0x2191: i=1; break; // upward arrow
            case 0x2192: i=4; break; // rightward arrow
            case 0x2193: i=2; break; // downward arrow
            case 0x0009: i=5; break; // tab
            case 0x0008: i=6; break; // backspace
			case 0x21DE: i=9; break; // PgUp
			case 0x21DF: i=10; break; // PgDwn


			default:    {	SetTextColor(hdc, fgColor);
							if (kd.w > 0x1F) 
							{ 
                ::DrawText(hdc, (LPCWSTR)&kd.w, 1, lpRect, uFormat);
							}
							else 
							{
								WCHAR s[2]=L"^";
								s[1]=kd.w+0x40;
								::DrawText(hdc, (LPCWSTR)&s, 2, lpRect, uFormat);
							}
							return;

						};
            }

            if( GetRValue(fgColor)+GetGValue(fgColor)+GetBValue(fgColor) > 3*128 ) // светлый цвет
            {
                i += NUM_SYMBOLS; // второй набор картинок
            }

            HIMAGELIST hil;
            int n;
			if((lpRect->bottom-lpRect->top)/(((uFormat & 0xC) == DT_TOP)?2:1) < 18 ||
               (lpRect->right-lpRect->left)/(((uFormat & 0x3) == DT_RIGHT)?2:1) < 18 )
            {
                hil = m_hilSymbols16;
                n = 16;
            } else
            {
                hil = m_hilSymbols32;
                n = 32;
            }
            if(hil)
            {int x, y;
			 switch (uFormat & 0x3){
			  case DT_LEFT:  { x=min(lpRect->left,(lpRect->left+lpRect->right-n)/2);break;}
			  case DT_RIGHT: { x=max(lpRect->right-n,(lpRect->left+lpRect->right-n)/2);break;}
			  default: {x=(lpRect->left+lpRect->right-n)/2;}
			 };
			 switch (uFormat & 0xC){
			  case DT_TOP:   { y=min(lpRect->top,(lpRect->bottom+lpRect->top-n)/2);break;}
			  case DT_BOTTOM:{ y=max(lpRect->bottom-n,(lpRect->bottom+lpRect->top-n)/2);break;}
			  default: {y=(lpRect->bottom+lpRect->top-n)/2;}
			 };
			 ImageList_Draw(hil, i, hdc, x, y, ILD_TRANSPARENT);
            } else
            {
                SetTextColor(hdc, fgColor);
                ::DrawText(hdc, (LPCWSTR)&kd.w, 1, lpRect, uFormat);
            }
        } else
        {
            SetTextColor(hdc, fgColor);
            ::DrawText(hdc, kd.pw, -1, lpRect, uFormat);
            return;
        }
    }

    void LoadConfig(LPCWSTR pcwName)
    {
        LONG iLen = wcslen(g_pwDllDir)+1+wcslen(pcwName)+4+1;
        WCHAR* pw = new WCHAR[iLen];
        _snwprintf(pw, iLen, L"%s\\%s.txt", g_pwDllDir, pcwName);
        CKbdConfig *pConfig = new CKbdConfig(pw);
        if( pConfig->Ensure()==CKbdConfig::E_OK )
        {
            m_pwCurrentConfigName = _wcsdup(pcwName);

            m_config = pConfig;

            ReloadSkin();
        } else
        {
            BOX(L"ERROR", L"Error load config '%s'", pw);
            m_config = pConfig;
        }
        delete []pw;
    }

    void ReloadSkin()
    {
        LPWSTR pcwSkinFileName = 0;
        if(m_vga==2 && m_config->m_pwSkinVGA)
            pcwSkinFileName = m_config->m_pwSkinVGA.ptr();
        else if(m_vga==1 && m_config->m_pwSkinQVGA)
            pcwSkinFileName = m_config->m_pwSkinQVGA.ptr();
        if(pcwSkinFileName)
        {
            WCHAR sw[MAX_PATH]; // malloc?
            PathCombine(sw, g_pwDllDir, pcwSkinFileName);
#ifdef _WIN32_WCE
            m_hbmSkin    = SHLoadImageFile(sw);
#else
            m_hbmSkin    = (HBITMAP)LoadImage(NULL, sw, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
#endif
        } else
        {
            m_hbmSkin    = 0;
        }
        KeyboardSizeChanged();
        if (m_hwndMain!=0)
        {
            InvalidateRect(m_hwndMain, 0, FALSE);
        }
    }

    void ChangeResolution(int vga)
    {
        m_vga = vga;
        m_hFontBig   = 0;
        m_hFontSmall = 0;
        m_hFontIndex = 0;
        ReloadSkin();
    }

    bool EnsureConfig()
    {
        if( m_config->Ensure()!=CKbdConfig::E_UNCHANGED ) // хоть 1 раз, но тут будет true
        {
            m_hFontBig   = 0;
            m_hFontSmall = 0;
            m_hFontIndex = 0;
            ReloadSkin();
            return true;
        }
        return false;
    }

// нажали кнопку, надо об этом сообщить наверх через
//  m_pIMCallback->SendVirtualKey
//  m_pIMCallback->SendCharEvents
//  m_pIMCallback->SendString
    void SendText(LPCWSTR pcw)
    {
        if(m_pIMCallback==0)
            return;
        UINT uLastChar = L'\0';
        for(; *pcw; pcw++)
        {
            UINT uChar = *pcw;
            UINT uShift = KeyStateDownFlag;
            UINT vk;

            // в клипборде обычно \r\n, а надо передать только 1 Enter
            if(uLastChar==L'\r' && uChar==L'\n') continue;

            if(uChar<=0x20 || ('A'<=uChar && uChar<='Z')) // очень нужно для кнопки Enter
            {
                vk = uChar;
            } else if('a'<=uChar && uChar<='z')
            {
                vk = uChar & ~0x20;
            } else
            {
                vk = VK_SPACE;
            }
            m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &uChar);
            uShift = KeyShiftNoCharacterFlag|KeyStatePrevDownFlag;
            m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &uChar);
            uLastChar = uChar;
        }
    }

    void SendKey(UINT vk, keydata data, UINT group)
    {
        if(m_pIMCallback==0)
            return;

        if(vk==_VK_MOD)
        {
            if( m_subkeyCurrent!=SUBKEY_PARENT )
            {
                LoadConfig(m_subkeyCurrent->data.pw);
            }
        } else if(vk==_VK_FN)
        {
//      } else if(vk==_VK_CUT)
//      {
//          BOX(L"TODO", L"CUT");
//      } else if(vk==_VK_COPY)
//      {
//          BOX(L"TODO", L"COPY");
        } else if(vk==_VK_PASTE)
        {
            if( OpenClipboard(0) )
            {
                HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
                if(hClip)
                {
                    LPCWSTR lpData = (LPCWSTR) GlobalLock (hClip);
                    if(lpData)
                    {
                        SendText(lpData);
                    }
                    GlobalUnlock (hClip); // remark #174: expression has no effect
                    CloseClipboard();
                } else
                {
                    CloseClipboard();
                    //TODO: send WM_PASTE
                }
            }
        } else if(group==SK_GROUP_CONTROL)
        {
            m_pIMCallback->SendVirtualKey(VK_CONTROL, KEYEVENTF_SILENT);
            if( data.w == 0x0000) // не-буква, а стрелка, F1, ...
		      {m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT);
               m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
              } else if( data.w <= 0xFFFF)
			  {
				UINT uShift = KeyStateDownFlag|KeyShiftAnyCtrlFlag;
				m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &data.w);
				uShift = KeyShiftNoCharacterFlag|KeyStatePrevDownFlag;
				m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &data.w);
			  };
            m_pIMCallback->SendVirtualKey(VK_CONTROL, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
        } else if(group==SK_GROUP_ALT)
        {
            m_pIMCallback->SendVirtualKey(VK_MENU, KEYEVENTF_SILENT);
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT);
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
            m_pIMCallback->SendVirtualKey(VK_MENU, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
        } else if(group==SK_GROUP_SHIFT)
        {
            //nShiftState |= KeyShiftAnyAltFlag;
            //nShiftState |= KeyShiftAnyCtrlFlag;
            //nShiftState |= KeyShiftNoCharacterFlag;
            //nShiftState |= KeyShiftAnyShiftFlag;
            m_pIMCallback->SendVirtualKey(VK_SHIFT, KEYEVENTF_SILENT);
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT);
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
            m_pIMCallback->SendVirtualKey(VK_SHIFT, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
        } else if( data.w == 0x0000) // не-буква, а стрелка, F1, ...
        {
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT);
            m_pIMCallback->SendVirtualKey(vk, KEYEVENTF_SILENT|KEYEVENTF_KEYUP);
        } else if( data.w <= 0xFFFF)
        {
            UINT uShift = KeyStateDownFlag;
            m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &data.w);
            uShift = KeyShiftNoCharacterFlag|KeyStatePrevDownFlag;
            m_pIMCallback->SendCharEvents(vk, uShift, 1, &uShift, &data.w);

            // немного магии
            if( group==SK_GROUP_LATIN &&
                m_pwCurrentConfigName &&
                0==wcsncmp(m_pwCurrentConfigName.ptr(),L"rus",3) )
            {
                //D(L"LoadConfig(L'lat');");
				LoadConfig((GetSystemMetrics(SM_CXSCREEN)>GetSystemMetrics(SM_CYSCREEN))?L"lat_land":L"lat");
            } else if(  group==SK_GROUP_CYRILLIC &&
                        m_pwCurrentConfigName &&
                        0==wcsncmp(m_pwCurrentConfigName.ptr(),L"lat",3) )
            {
                //D(L"LoadConfig(L'rus');");
				LoadConfig((GetSystemMetrics(SM_CXSCREEN)>GetSystemMetrics(SM_CYSCREEN))?L"rus_land":L"rus");
            }
        } else
        {
            SendText(data.pw);
        }
    }

    KEYENTRY* WINAPI GetKeyFromQVGACoord(int x, int y) const
    {
        // проверять на клик в порядке обратном рисованию
        for(KEYENTRY* p=m_config->m_keys.last; p; p=p->prev)
        {
            if( (p->top <= y && y < p->bottom &&
                 p->left <= x && x < p->right)
                //(keys[i].top2 <= prcKey->top && prcKey.top < keys[i].bottom2 &&
                // keys[i].left2 <= prcKey->left && prcKey.left < keys[i].right2)
                 )
            {
                return p;
            }
        }
        return 0;
    }


    SUBKEYENTRY* WINAPI GetSubKeyFromQVGACoord(int x, int y) const
    {
        if(m_keyCurrent)
        {
            x += m_ptPopShift.x - m_config->m_rPopBorder.left;
            y += m_ptPopShift.y - m_config->m_rPopBorder.top;

            if(0<=x && x<m_keyCurrent->right-m_keyCurrent->left &&
               0<=y && y<m_keyCurrent->bottom-m_keyCurrent->top)
            {
                return SUBKEY_PARENT;
            }
            for(SUBKEYENTRY* p=m_keyCurrent->subkeys.first; p; p=p->next)
            {
                if( p->top <= y && y < p->bottom &&
                    p->left <= x && x < p->right)
                {
                    return p;
                }
            }
        }
        return 0;
    }

    static void InitFont(CGdiObj<HFONT>& hFont, LPCWSTR pcwFontFace, LONG iFontSize, bool bBold)
    {
        if(!hFont)
        {
            LOGFONT lf;
            ZeroMemory(&lf, sizeof(lf));
            lf.lfHeight = -iFontSize;
            lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lstrcpyW(lf.lfFaceName, pcwFontFace);
            hFont = CreateFontIndirect(&lf);
            if(!hFont)
            {
                hFont = (HFONT)GetStockObject(SYSTEM_FONT);
                assert(hFont);
            }
        }
    }

    void InitFonts()
    {
        InitFont(m_hFontBig,   m_config->m_pwFontBig.ptr(),   m_config->m_iFontBig  *m_vga, m_config->m_bFontBigBold  );
        InitFont(m_hFontSmall, m_config->m_pwFontSmall.ptr(), m_config->m_iFontSmall*m_vga, m_config->m_bFontSmallBold);
        InitFont(m_hFontIndex, m_config->m_pwFontIndex.ptr(), m_config->m_iFontIndex*m_vga, m_config->m_bFontIndexBold);
        if(!m_hilSymbols32)
        {
            m_hilSymbols32  = ImageList_Create(32, 32, ILC_COLOR | ILC_MASK, 1, 1 );
#ifdef _WIN32_WCE
            CGdiObj<HBITMAP> hbm ( SHLoadImageResource(g_hInstDll, IDB_SYMBOLS32) );
#else
            CGdiObj<HBITMAP> hbm ( LoadBitmap( g_hInstDll, MAKEINTRESOURCE(IDB_SYMBOLS32) ) );
#endif
            if(hbm)
            {
                ImageList_AddMasked( m_hilSymbols32, hbm, RGB(0x00,0x66,0x00) );
            }
        }
        if(!m_hilSymbols16)
        {
            m_hilSymbols16  = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1 );
#ifdef _WIN32_WCE
            CGdiObj<HBITMAP> hbm ( SHLoadImageResource(g_hInstDll, IDB_SYMBOLS16) );
#else
            CGdiObj<HBITMAP> hbm ( LoadBitmap( g_hInstDll, MAKEINTRESOURCE(IDB_SYMBOLS16) ) );
#endif
            if(hbm)
            {
                ImageList_AddMasked( m_hilSymbols16, hbm, RGB(0x00,0x66,0x00) );
            }
        }
    }


    void InitImageLists()
    {
        if( !m_hilWide )
        {
            m_hilWide   = ImageList_Create(32, 16, ILC_COLOR | ILC_MASK, 1, 1 );
            assert(m_hilWide);
            HBITMAP hbm = LoadBitmap( g_hInstDll, MAKEINTRESOURCE(IDB_WIDE1) );
            if(hbm)
            {
                ImageList_AddMasked( m_hilWide, hbm, RGB(255,0,255) );
                DeleteObject(hbm);
            }
        }
        if( !m_hilNarrow )
        {
            m_hilNarrow = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1 );
            assert(m_hilNarrow);
            HBITMAP hbm = LoadBitmap( g_hInstDll, MAKEINTRESOURCE(IDB_NARROW1) );
            if(hbm)
            {
                ImageList_AddMasked( m_hilNarrow, hbm, RGB(255,0,255) );
                DeleteObject(hbm);
            }
        }
    }

    void PopupOnMouseMove(int x, int y)
    {
        SUBKEYENTRY* pSubkey = GetSubKeyFromQVGACoord(x/m_vga, y/m_vga);
        if(m_subkeyCurrent != pSubkey) // сменилась кнопка
        {
            SUBKEYENTRY* pSubkeyOld = m_subkeyCurrent;
            m_subkeyCurrent = pSubkey;

            KillTimer(m_hwndPop, IDT_AUTOREPEAT);
            if(0!=m_nAutorepeatDelay &&
               m_subkeyCurrent!=0 &&
               (m_subkeyCurrent==SUBKEY_PARENT ||
                // эти кнопки не авторепетяцца
                (m_subkeyCurrent->vk!=_VK_FN &&
                 m_subkeyCurrent->vk!=_VK_MOD &&
                 m_subkeyCurrent->vk!=_VK_TIME &&
                 m_subkeyCurrent->vk!=_VK_DATE &&
                 m_subkeyCurrent->vk!=_VK_CUT &&
                 m_subkeyCurrent->vk!=_VK_COPY &&
                 m_subkeyCurrent->vk!=_VK_PASTE)))
            {
                SetTimer(m_hwndPop, IDT_AUTOREPEAT, m_nAutorepeatDelay, 0);
            }


            if(m_subkeyCurrent!=0)
            {
                RECT r;
                SubkeyRect(r, m_subkeyCurrent);
                InvalidateRect(m_hwndPop, &r, FALSE);
            }
            if(pSubkeyOld!=0 /*&& pSubkeyOld!=SUBKEY_UNKNOWN*/)
            {
                RECT r;
                SubkeyRect(r, pSubkeyOld);
                InvalidateRect(m_hwndPop, &r, FALSE);
            }
        }
    }

    void SubkeyRect(RECT& r, const SUBKEYENTRY* p)
    {
        if(p==SUBKEY_PARENT)
        {
            r.left   = m_vga*(m_config->m_rPopBorder.left + 0         - m_ptPopShift.x);
            r.top    = m_vga*(m_config->m_rPopBorder.top  + 0         - m_ptPopShift.y);
            r.right  = m_vga*(m_config->m_rPopBorder.left + m_keyCurrent->right-m_keyCurrent->left - m_ptPopShift.x);
            r.bottom = m_vga*(m_config->m_rPopBorder.top  + m_keyCurrent->bottom-m_keyCurrent->top - m_ptPopShift.y);
        } else
        {
            r.left   = m_vga*(m_config->m_rPopBorder.left + p->left   - m_ptPopShift.x);
            r.top    = m_vga*(m_config->m_rPopBorder.top  + p->top    - m_ptPopShift.y);
            r.right  = m_vga*(m_config->m_rPopBorder.left + p->right  - m_ptPopShift.x);
            r.bottom = m_vga*(m_config->m_rPopBorder.top  + p->bottom - m_ptPopShift.y);
        }
    }

    void UpdateClockWatch()
    {
        if(m_keyCurrent)
        {
            SYSTEMTIME st;
            GetLocalTime(&st);
            WCHAR sw[64];

            for(SUBKEYENTRY* p=m_keyCurrent->subkeys.first; p; p=p->next)
            {
                if(p->vk==_VK_TIME)
                {
                    //_snwprintf(sw, 64, L"paste \x201E%02d:%02d:%02d\x201D", st.wHour, st.wMinute, st.wSecond);
                    _snwprintf(sw, 64, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
                    if(0!=wcscmp(p->data.pw, sw))
                    {
                        free(p->data.pw);
                        p->data.pw = _wcsdup(sw);

                        RECT r;
                        SubkeyRect(r, p);
                        InvalidateRect(m_hwndPop, &r, FALSE);
                    }
                } else if(p->vk==_VK_DATE)
                {
                    //_snwprintf(sw, 64, L"paste \x201E%02d.%02d.%04d\x201D", st.wDay, st.wMonth, st.wYear);
                    _snwprintf(sw, 64, L"%02d.%02d.%04d", st.wDay, st.wMonth, st.wYear);
                    if(0!=wcscmp(p->data.pw, sw))
                    {
                        free(p->data.pw);
                        p->data.pw = _wcsdup(sw);

                        RECT r;
                        SubkeyRect(r, p);
                        InvalidateRect(m_hwndPop, &r, FALSE);
                    }
                } else if(p->vk==_VK_PASTE)
                {
                    sw[0] = 0;
                    if( OpenClipboard(0) )
                    {
                        HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
                        if(hClip)
                        {
                            wchar_t* lpData = (wchar_t*) GlobalLock (hClip);
                            if(lpData)
                            {
                                size_t len = wcslen(lpData);
                                if(len>30)
                                    _snwprintf(sw, 64, L"paste \x201E%.30s%s\x2026\x201D (%d)", lpData, len>30 ? L"" : L"", len);
                                else
                                    _snwprintf(sw, 64, L"paste \x201E%s\x201D", lpData);
                            }
                            GlobalUnlock (hClip); // remark #174: expression has no effect
                        } else
                        {
                            sw[0] = 0;
                            int format = 0;
                            while ((format = EnumClipboardFormats (format)) != 0)
                            {
                                if(format==CF_BITMAP)
                                    swprintf( sw, L"paste CF_BITMAP");
                                else
                                    swprintf( sw, L"paste format %d", format);
                                //GetClipboardFormatName(format, sw+6, 64-6);
                                break;
                            }
                        }
                        CloseClipboard();
                    } else
                    {
                        swprintf( sw, L"err OpenClipboard() %d ", GetLastError());
                    }
                    free(p->data.pw);
                    p->data.pw = _wcsdup(sw);

                    RECT r;
                    SubkeyRect(r, p);
                    InvalidateRect(m_hwndPop, &r, FALSE);
                }
            }
        }
    }

    static LRESULT WINAPI PopWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        CKbd* _this;
        if(msg==WM_CREATE)
        {
            SetWindowLong(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT*)lParam)->lpCreateParams);
            _this = (CKbd*)((CREATESTRUCT*)lParam)->lpCreateParams;
            assert(_this->m_hwndPop==0);
            _this->m_hwndPop = hWnd;
        } else
        {
            _this = (CKbd*)GetWindowLong(hWnd, GWL_USERDATA);
        }
        LRESULT lResult;
        if(_this)
        {
            assert(_this->m_hwndPop==hWnd);
            lResult = _this->PopWndProc( msg, wParam, lParam );
        } else
        {
            lResult = DefWindowProc(hWnd, msg, wParam, lParam );
        }
        return lResult;
    }

    LRESULT PopWndProc( UINT msg, WPARAM wParam, LPARAM lParam )
    {
        switch(msg)
        {
        case WM_PAINT:
            {
            UpdateClockWatch();

#ifdef PAINT_VIA_BITMAP
            CDblBufPaintDC hdc(m_hwndPop);
#else
            CPaintDC hdc(m_hwndPop);
#endif

            CGdiObj<HBRUSH> hBrush ( CreateSolidBrush(m_config->m_colorPopBorder));
            assert(hBrush);
            FillRect(hdc, &hdc.rect(), hBrush);

            //CGdiObj<HPEN> hPenTL   ( CreatePen(PS_SOLID, 1, RGB(0xCC,0xCC,0xCC)));
            //assert(hPenTL);
            //CGdiObj<HPEN> hPenBR   ( CreatePen(PS_SOLID, 1, RGB(0x66,0x66,0x66)));
            //assert(hPenBR);

            InitFonts();

            SetBkMode(hdc, TRANSPARENT);

            for(const SUBKEYENTRY* p=m_keyCurrent->subkeys.first; p; p=p->next)
            {
                if(p->vk==_VK_TIME || p->vk==_VK_DATE)
                {
                    SetTimer(m_hwndPop, IDT_CLOCKWATCH, 500, 0);
                }

                if(!m_bShowControl && (p->group==SK_GROUP_CONTROL||p->group==SK_GROUP_ALT) ) continue;
                if(!m_bShowGreek   && p->group==SK_GROUP_GREEK   ) continue;
                if(!m_bShowLatinEx && p->group==SK_GROUP_LATIN_EX) continue;
                if(!m_bShowCyrEx   && p->group==SK_GROUP_CYR_EX  ) continue;

                RECT r;
                SubkeyRect(r, p);
                if ( InRect(hdc.rect(), r) )
                {
                    const bool bBlack = m_subkeyCurrent==p;
                    const COLORREF crFore = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colors[p->group][0];
                    const COLORREF crBack = bBlack ? RGB(0x00,0x00,0x00) : m_config->m_colors[p->group][1];

                    CGdiObj<HBRUSH> hBrushBg ( CreateSolidBrush(crBack) );
                    assert(hBrushBg);
                    // если нет обоев
                    DrawButton(hdc, r, hBrushBg/*, hPenTL, hPenBR*/);

                    RECT rect = { r.left+1, r.top+1, r.right-m_vga, r.bottom-1 };
                    if( p->group==SK_GROUP_SMALL ||
                        p->group==SK_GROUP_CONTROL ||
                        p->group==SK_GROUP_ALT ||
                        p->group==SK_GROUP_SHIFT)
                    {
                        SelectObject(hdc, m_hFontSmall);
                    } else
                    {
                        SelectObject(hdc, m_hFontBig);
                    }

                    keydata kdButtonText;
                    if (0!=p->desc.pw) // обратный порядок: desc приоритетнее
                    {
                        kdButtonText = p->desc;
                    } else if (0!=p->data.pw)
                    {
                        kdButtonText = p->data;
                    }
                    DrawText(hdc, kdButtonText, &rect, DT_NOPREFIX | DT_CENTER | DT_VCENTER, crFore);
                }
            }

            // повтор в попапе самой кнопки из основной клавы (нажатие кот-й вызвало попап)
            RECT r;
            SubkeyRect(r, SUBKEY_PARENT);
            if ( InRect(hdc.rect(), r) )
            {
                const bool bBlack = m_subkeyCurrent==SUBKEY_PARENT;
                const COLORREF crFore = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colors[m_keyCurrent->group][0];
                const COLORREF crBack = bBlack ? RGB(0x00,0x00,0x00) : m_config->m_colors[m_keyCurrent->group][1];

                CGdiObj<HBRUSH> hBrushBg ( CreateSolidBrush(crBack) );
                assert(hBrushBg);
                DrawButton(hdc, r, hBrushBg/*, hPenTL, hPenBR*/);

                RECT rect = { r.left+2*m_vga, r.top+1, r.right-1*m_vga, r.bottom-1 };
                if( m_keyCurrent->group==SK_GROUP_SMALL ||
                    m_keyCurrent->group==SK_GROUP_CONTROL ||
                    m_keyCurrent->group==SK_GROUP_ALT ||
                    m_keyCurrent->group==SK_GROUP_SHIFT)
                {
                    SelectObject(hdc, m_hFontSmall);
                } else
                {
                    SelectObject(hdc, m_hFontBig);
                }

                keydata kdButtonText;

                if (0!=m_keyCurrent->data.pw)
                {
                    kdButtonText = m_keyCurrent->data;
                } else if (0!=m_keyCurrent->desc.pw)
                {
                    kdButtonText = m_keyCurrent->desc;
                }
                DrawText(hdc, kdButtonText, &rect, DT_NOPREFIX | DT_CENTER | DT_VCENTER, crFore);
            }

            return 0;
            }

        case WM_MOVE:
        case WM_SIZE:
            //break;
            // уточнить положение мыши и текущую кнопку
            {
                POINT pos = m_ptCursorPos;
                if( ScreenToClient(m_hwndPop, &pos) )
                {
                    PopupOnMouseMove(pos.x, pos.y);
                }
            }
            return 0;


        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MOUSEMOVE:
            SaveCursorPos(m_hwndPop, lParam);
            PopupOnMouseMove(LOWORD(lParam), HIWORD(lParam));
            return 0;


        case WM_TIMER:
            if(wParam==IDT_AUTOREPEAT)
            {
                if(0!=m_nAutorepeatPeriod)
                {
                    SetTimer(m_hwndPop, IDT_AUTOREPEAT, m_nAutorepeatPeriod, 0);
                }
                if( m_subkeyCurrent==0 )
                {
                } else if (m_subkeyCurrent==SUBKEY_PARENT)
                {
                    SendKey(m_keyCurrent->vk, m_keyCurrent->data, m_keyCurrent->group);
                } else
                {
                    SendKey(m_subkeyCurrent->vk, m_subkeyCurrent->data, m_subkeyCurrent->group);
                }
            }

            // идущие часики на кнопке "вставить время"
            if(wParam==IDT_CLOCKWATCH)
            {
                UpdateClockWatch();
            }
            return 0;


        case WM_LBUTTONUP:
            SaveCursorPos(m_hwndPop, lParam);
            KillTimer(m_hwndPop, IDT_AUTOREPEAT);

            KillTimer(m_hwndPop, IDT_CLOCKWATCH);

            if( m_subkeyCurrent==0 )
            {
            } else if (m_subkeyCurrent==SUBKEY_PARENT)
            {
                SendKey(m_keyCurrent->vk, m_keyCurrent->data, m_keyCurrent->group);
            } else
            {
                SendKey(m_subkeyCurrent->vk, m_subkeyCurrent->data, m_subkeyCurrent->group);
            }

            ReleaseCapture();
            ShowWindow(m_hwndPop, SW_HIDE);
            m_bPopupActive = false;
            m_keyCurrent = 0;
            m_subkeyCurrent = 0;
#ifndef _WIN32_WCE
            SetActiveWindow(GetParent(m_hwndMain)); // tmp
#endif
            return 0;

        }
        return DefWindowProc(m_hwndPop, msg, wParam, lParam );
    }


    void PopUp()
    {
        if( 0==m_hwndPop)
        {
            WNDCLASS wc;
            ZeroMemory( &wc, sizeof(wc) );
            wc.style = CS_SAVEBITS ; //| CS_DROPSHADOW;
            wc.lpfnWndProc = PopWndProc;
            wc.hInstance = g_hInstDll;
#ifndef _WIN32_WCE
            wc.hCursor = LoadCursor(LoadLibrary(L"USER32"), MAKEINTRESOURCE(100));
#endif
            wc.lpszClassName = s_szPopWndClassName;
            if(!RegisterClass(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            {
                BOX(L"ERROR", L"RegisterClass err=%d", GetLastError());
            } else
            {
                m_hwndPop = 0;
                CreateWindowEx( WS_EX_TOPMOST,
                                s_szPopWndClassName,
                                L"",
                                WS_POPUP,
                                0, 0, 100, 100,
                                m_hwndMain,
                                (HMENU)NULL,
                                g_hInstDll,
                                this );
                if(!m_hwndPop)
                {
                    BOX(L"ERROR", L"CreateWindow err=%x", GetLastError());
                }
            }
        }
        if(m_hwndPop)
        {
            // кнопка Mod
            if(m_keyCurrent->vk==_VK_MOD)
            {
                m_keyCurrent->subkeys.destroy();

                WIN32_FIND_DATA ffdata;
                WCHAR* pwMask = new WCHAR[wcslen(g_pwDllDir)+6+1];
                wcscpy(pwMask, g_pwDllDir);
                wcscat(pwMask, L"\\*.txt");
                //UINT iLenMask = wcslen(swMask);
                HANDLE hFind = FindFirstFile(pwMask, &ffdata);
                if( hFind!=INVALID_HANDLE_VALUE )
                {
                    int i = 0;
                    do
                    {
                        SUBKEYENTRY* psk = new SUBKEYENTRY;
                        psk->vk     = _VK_MOD;
                        //psk->data.pw= new WCHAR[iLenMask-5+wcslen(ffdata.cFileName)+1];
                        //wcscpy(psk->data.pw, swMask);
                        //wcscpy(psk->data.pw+iLenMask-5, ffdata.cFileName);
                        ffdata.cFileName[wcslen(ffdata.cFileName)-4] = '\0'; // trim .txt
                        psk->data.pw= _wcsdup(ffdata.cFileName);
                        psk->left   = 0;
                        psk->top    = (i+1)*-16;
                        psk->right  = 80;
                        psk->bottom = (i+0)*-16;
                        psk->group  = SK_GROUP_LATIN;
                        psk->desc.pw= 0;
                        m_keyCurrent->subkeys.add_first(psk);
                        i ++;
                    } while(FindNextFile(hFind, &ffdata));
                    FindClose(hFind);
                }
                delete []pwMask;
            }

            // кнопка Fn
            if(m_keyCurrent->vk==_VK_FN)
            {
            }

            m_subkeyCurrent = 0;

            // размеры попапа чтобы в нём поместились все его кнопки
            RECT rmax = {0, 0, m_keyCurrent->right-m_keyCurrent->left, m_keyCurrent->bottom-m_keyCurrent->top};
            for(const SUBKEYENTRY* p=m_keyCurrent->subkeys.first; p; p=p->next)
            {
                if(!m_bShowControl && (p->group==SK_GROUP_CONTROL||p->group==SK_GROUP_ALT) ) continue;
                if(!m_bShowGreek   && p->group==SK_GROUP_GREEK   ) continue;
                if(!m_bShowLatinEx && p->group==SK_GROUP_LATIN_EX) continue;
                if(!m_bShowCyrEx   && p->group==SK_GROUP_CYR_EX  ) continue;

                rmax.left   = min(rmax.left,   p->left  );
                rmax.top    = min(rmax.top,    p->top   );
                rmax.right  = max(rmax.right,  p->right );
                rmax.bottom = max(rmax.bottom, p->bottom);
            }
            // (0,0) координат кнопок относительно попап-окна
            m_ptPopShift.x = rmax.left;
            m_ptPopShift.y = rmax.top;


            //POINT ptMain = {0, 0};
            //ClientToScreen(m_hwndMain, &ptMain);

            // еще одно смещение, чтобы середина при появлении попапа середина кнопки оказалась под курсором
            POINT ptMain = {
                m_ptBtnDownPos.x - ((m_keyCurrent->right + m_keyCurrent->left)/2)*m_vga,
                m_ptBtnDownPos.y - ((m_keyCurrent->bottom + m_keyCurrent->top)/2)*m_vga
                };

            SetWindowPos( m_hwndPop,
                            HWND_TOPMOST, //HWND_TOP,
                            ptMain.x + m_vga*(m_keyCurrent->left + rmax.left - m_config->m_rPopBorder.left),
                            ptMain.y + m_vga*(m_keyCurrent->top  + rmax.top  - m_config->m_rPopBorder.top ),
                            m_vga*(m_config->m_rPopBorder.left + m_config->m_rPopBorder.right  + RectWidth(rmax)),
                            m_vga*(m_config->m_rPopBorder.top  + m_config->m_rPopBorder.bottom + RectHeight(rmax)),
                            SWP_SHOWWINDOW | SWP_NOACTIVATE );

            SetCapture( m_hwndPop );
            m_bPopupActive = true;
        }
    }

    static LRESULT WINAPI MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        CKbd* _this;
        //LOG(L"MSG(%x %x %x)",msg, wParam, lParam);
        if(msg==WM_CREATE)
        {
            SetWindowLong(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT*)lParam)->lpCreateParams);
            _this = (CKbd*)((CREATESTRUCT*)lParam)->lpCreateParams;
            assert(_this->m_hwndMain==0);
            _this->m_hwndMain = hWnd;
        } else
        {
            _this = (CKbd*)GetWindowLong(hWnd, GWL_USERDATA);
        }
        LRESULT lResult;
        if(_this)
        {
            assert(_this->m_hwndMain==hWnd);
            lResult = _this->MainWndProc( msg, wParam, lParam );
        } else
        {
            lResult = DefWindowProc(hWnd, msg, wParam, lParam );
        }
        //LOG(L"MSG(%x %x %x) --> %x",msg, wParam, lParam, lResult);
        return lResult;
    }
    LRESULT MainWndProc( UINT msg, WPARAM wParam, LPARAM lParam )
    {
        switch( msg )
        {
        case WM_PAINT:
            {
                SYSTEMTIME st;
                GetLocalTime(&st);

#ifdef PAINT_VIA_BITMAP
                CDblBufPaintDC hdc(m_hwndMain);
#else
                CPaintDC hdc(m_hwndMain);
#endif
                CGdiObj<HBRUSH> hBrush ( CreateSolidBrush(RGB(0xCC,0xCC,0xCC)) );
                assert(hBrush);
                FillRect(hdc, &hdc.rect(), hBrush);

                // если есть обои
                if(m_hbmSkin)
                {
                    HDC     hdcSkin = CreateCompatibleDC( NULL );
                    assert(hdcSkin);
                    HBITMAP hbmSkinOld = (HBITMAP)SelectObject( hdcSkin, m_hbmSkin );
                    assert(hbmSkinOld);

                    BitBlt(hdc,
                        hdc.rect().left,
                        hdc.rect().top,
                        RectWidth(hdc.rect()),
                        RectHeight(hdc.rect()),
                        hdcSkin,
                        hdc.rect().left,
                        hdc.rect().top,
                        SRCCOPY );

                    SelectObject( hdcSkin, hbmSkinOld );
                    DeleteDC( hdcSkin );
                }

                //CGdiObj<HBRUSH>  hBrushBg = (HBRUSH)GetStockObject(WHITE_BRUSH);
                //assert(hBrushBg);
                //CGdiObj<HPEN>   hPenTL   ( CreatePen(PS_SOLID, 1, RGB(0xCC,0xCC,0xCC)) );
                //assert(hPenTL);
                //CGdiObj<HPEN>   hPenBR   ( CreatePen(PS_SOLID, 1, RGB(0x66,0x66,0x66)) );
                //assert(hPenBR);

                InitFonts();
                SetBkMode(hdc, TRANSPARENT);

                for(const KEYENTRY* p=m_config->m_keys.first; p; p=p->next)
                {
                    RECT r0 = { p->left*m_vga, p->top*m_vga, p->right*m_vga, p->bottom*m_vga };
                    if ( InRect(hdc.rect(), r0) )
                    {
                        const bool bBlack = p==m_keyCurrent && !m_bPopupActive;

                        const COLORREF crBackground = bBlack ? RGB(0x00,0x00,0x00) : m_config->m_colors[p->group][1];
                        const COLORREF crFontBig    = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colors[p->group][0];
                        const COLORREF crDesc       = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colorDesc;
                        const COLORREF crDesc2      = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colorDesc2;
                        const COLORREF crSmall      = bBlack ? RGB(0xFF,0xFF,0xFF) : m_config->m_colors[p->group][0];

                        // если нет обоев - рисуем кнопочку, если кнопка текущая - рисуем чёрную кнопочку
                        if(!m_hbmSkin || bBlack)
                        {
                            CGdiObj<HBRUSH> hBrushBg2 ( CreateSolidBrush(crBackground) );
                            assert(hBrushBg2);
                            DrawButton(hdc, r0, hBrushBg2/*, hPenTL, hPenBR*/);
                        }

                        RECT rText = { r0.left+3*m_vga, r0.top, r0.right-1*m_vga, r0.bottom-1*m_vga };
						if ((0!=p->data.pw)&&(p->vk!=_VK_MOD)) //для клавиши MOD data не выводим на клавишу, т.к. поле используется для имени файла раскладки
                        {
                            SelectObject(hdc, m_hFontBig);
                            DrawText(hdc, p->data, &rText, DT_NOPREFIX | DT_BOTTOM | DT_LEFT, crFontBig);

                            if (0!=p->desc.pw) // надстрочный знак
                            {
                                SelectObject(hdc, m_hFontIndex);
                                //if(0!=p->desc2.pw) // есть оба знака - надстрочный двинем чуть левее
                                //    rText.right -= 7*m_vga;
                                DrawText(hdc, p->desc, &rText, DT_NOPREFIX | DT_RIGHT | DT_TOP, crDesc);
                                //if(0!=p->desc2.pw) // откат
                                //    rText.right += 7*m_vga;
                            }
                            if (0!=p->desc2.pw) // знак из другого алфавита
                            {
                               // rText.top += 4*m_vga;
                                SelectObject(hdc, m_hFontIndex);
                                DrawText(hdc, p->desc2, &rText, DT_NOPREFIX | DT_RIGHT | DT_BOTTOM, crDesc2); // выводим надпись внизу клавиши
                            }
                        } else if (0!=p->desc.pw) // текст вроде 'Caps'
                        {
                            SelectObject(hdc, m_hFontSmall);
                            DrawText(hdc, p->desc, &rText, DT_NOPREFIX | DT_CENTER | DT_VCENTER, crSmall);
                        }
                    }
                }
                return 0;
            }

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            SaveCursorPos(m_hwndMain, lParam);
            m_ptBtnDownPos = m_ptCursorPos;

            if(m_keyCurrent==0)
            {
                EnsureConfig();

                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                KEYENTRY* pKey = GetKeyFromQVGACoord( x/m_vga, y/m_vga);

                if(pKey)
                {
                    m_keyCurrent = pKey;
                    if(0==m_nPopupDelay ||
						(m_keyCurrent->vk==_VK_MOD)&&(m_keyCurrent->data.pw==0) ||
                       m_keyCurrent->vk==_VK_FN )
                    {
                        PopUp();
                    } else
                    {
                        // кнопка в нажатом состоянии
                        RECT r = {  m_keyCurrent->left*m_vga,
                                    m_keyCurrent->top*m_vga,
                                    m_keyCurrent->right*m_vga,
                                    m_keyCurrent->bottom*m_vga };
                        InvalidateRect(m_hwndMain, &r, FALSE);
                        //UpdateWindow(m_hwndMain);

                        assert( GetCapture()==0);
                        SetCapture(m_hwndMain);
                        SetTimer(m_hwndMain, IDT_POPUP_DELAY, m_nPopupDelay, 0);
                    }
                } else
                {
                    // нажатие мимо клавиши
                }

            }
            return 0;

        case WM_TIMER:
            if(wParam==IDT_POPUP_DELAY)
            {
                KillTimer(m_hwndMain, IDT_POPUP_DELAY);

                assert( 0!=m_keyCurrent);
                assert( !m_bPopupActive );
                assert( GetCapture()==m_hwndMain);

                ReleaseCapture();

                // потому что кнопка была чёрной
                RECT r = {  m_keyCurrent->left*m_vga,
                            m_keyCurrent->top*m_vga,
                            m_keyCurrent->right*m_vga,
                            m_keyCurrent->bottom*m_vga };
                InvalidateRect(m_hwndMain, &r, FALSE);

                PopUp();
            }
            /*if(wParam==IDT_CLOCKWATCH)
            {
                for(const KEYENTRY* p=m_config->m_keys.first; p; p=p->next)
                {
                    if(p->vk==_VK_TIME)
                    {
                        RECT r = {  p->left*m_vga,
                                    p->top*m_vga,
                                    p->right*m_vga,
                                    p->bottom*m_vga };
                        InvalidateRect(m_hwndMain, &r, FALSE);
                    }
                }
            }*/

            return 0;

        case WM_LBUTTONUP:
            SaveCursorPos(m_hwndMain, lParam);
            // при активизации попапа main получает WM_LBUTTONUP т.к. теряет фокус
            // а вот если попапа нету - то значит кнопку нажали и отпустили раньше, чем он возник
            if(m_keyCurrent && !m_bPopupActive)
            {
                KillTimer(m_hwndMain, IDT_POPUP_DELAY);

                ReleaseCapture();

                // потому что кнопка была чёрной
                RECT r = {  m_keyCurrent->left*m_vga,
                            m_keyCurrent->top*m_vga,
                            m_keyCurrent->right*m_vga,
                            m_keyCurrent->bottom*m_vga };
                InvalidateRect(m_hwndMain, &r, FALSE);

                short x = LOWORD(lParam);
                short y = HIWORD(lParam);
				if (m_keyCurrent->vk==_VK_MOD)
				    {LoadConfig(m_keyCurrent->data.pw);}
				else
				{ KEYENTRY* pKey = GetKeyFromQVGACoord( x/m_vga, y/m_vga );
                if (( m_keyCurrent != pKey )&& m_config->m_GesturesOn)
                  { switch (GetGesture( x/m_vga, y/m_vga))
					{
					  //case 0: BOX(L"GetGesture",L"%i, %i 0", x, y); break;
					  case 1: {keydata data = m_keyCurrent->data;
						       WCHAR*  p = 0;
						      if( data.w == 0x0000) // не-буква, а стрелка, F1, ...
							    {SendKey(m_keyCurrent->vk, data, SK_GROUP_SHIFT);}
							  else 							    
							    {if (data.w > 0xffff)
								  {p = new WCHAR[lstrlen(data.pw)+1];
							       data.pw = lstrcpy(p,data.pw);
							      };
						          data.pw = CharUpper(data.pw);
								  if ((data.w == m_keyCurrent->data.w)&&(m_keyCurrent->desc.w))
								    {data = m_keyCurrent->desc;
								     if (data.w > 0xffff)
									   {while ((*(data.pw)==L' ')&& *(data.pw)) data.pw++;
									    //data.w = uint(*(data.pw));
									   };
									 };
                                  SendKey(m_keyCurrent->vk, data, m_keyCurrent->group);
								  delete [] p;
								  };
							  break;};
		              case 2: SendText(L"\b"); break;
				      case 3: SendText(L" ");  break;
					  case 4: SendText(L"\r"); break;
					  case 0:
					  case 5: SendKey(m_keyCurrent->vk, m_keyCurrent->data, m_keyCurrent->group);
					}
                  } else
				  { 
				    SendKey(m_keyCurrent->vk, m_keyCurrent->data, m_keyCurrent->group);
				  };
				};
                m_keyCurrent = 0;
            }
            return 0;

        case WM_MOUSEMOVE:
            SaveCursorPos(m_hwndMain, lParam);
            // интересны только движения мыши с нажатой левой кнопкой
            {
                if(m_keyCurrent && !m_bPopupActive)
                {
                    assert(GetCapture()==m_hwndMain);

                    // если мышка уехала за пределы key на котором она была нажата ...
                    short x = LOWORD(lParam);
                    short y = HIWORD(lParam);
                    KEYENTRY* pKey = GetKeyFromQVGACoord( x/m_vga, y/m_vga );
                    if( m_keyCurrent != pKey )
                    {
                        KillTimer(m_hwndMain, IDT_POPUP_DELAY);

                      //  ReleaseCapture();

                        // потому что кнопка была чёрной
                        RECT r = {  m_keyCurrent->left*m_vga,
                                    m_keyCurrent->top*m_vga,
                                    m_keyCurrent->right*m_vga,
                                    m_keyCurrent->bottom*m_vga };
                        InvalidateRect(m_hwndMain, &r, FALSE);

                        if (!m_config->m_GesturesOn || GetGesture(x/m_vga, y/m_vga)==5)
						  {   //жесты запрещены или диагональный жест
						    ReleaseCapture();
						    PopUp();
						  };
                    }
                }
            }
            return 0;


        } // switch( message )

        return DefWindowProc(m_hwndMain, msg, wParam, lParam );
    }

    void InitMainWindow(HWND hwndParent)
    {
        WNDCLASS wc;

        ZeroMemory( &wc, sizeof(wc) );
        wc.lpfnWndProc = MainWndProc;
        wc.hInstance = g_hInstDll;
        wc.lpszClassName = s_szMainWndClassName;
#ifndef _WIN32_WCE
        wc.hCursor = LoadCursor(LoadLibrary(L"USER32"), MAKEINTRESOURCE(100));
#endif
        if(!RegisterClass( &wc ) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
        {
            BOX(L"ERROR", L"RegisterClass err=%d", GetLastError());
            return;
        }

        m_hwndMain = 0;
        CreateWindow(s_szMainWndClassName, L"", WS_CHILD,
            0, 0, m_config->m_size.cx*m_vga, m_config->m_size.cy*m_vga,
            hwndParent,
            (HMENU)NULL,
            g_hInstDll,
            this );
        if( !m_hwndMain )
        {
            BOX(L"ERROR", L"CreateWindow err=%d", GetLastError());
            return;
        }
    }

public:
    STDMETHOD(Select)(HWND hwndSip)
    {
        LOG(L"->Select(0x%X)", hwndSip);

        EnsureConfig();

        if (m_hwndMain==0)
        {
            InitMainWindow(hwndSip);
        }
        ShowWindow( m_hwndMain, SW_SHOWNOACTIVATE ); //
        return NOERROR;
    }

    STDMETHOD(Deselect)(void)
    {
        LOG(L"->Deselect()");
        ShowWindow( m_hwndMain, SW_HIDE );
#if 1
        DestroyGdiObjects();
#endif
        return NOERROR;
    }

    STDMETHOD(Showing)(void)
    {
        LOG(L"->Showing()");
        return NOERROR;
    }

    STDMETHOD(Hiding)(void)
    {
        LOG(L"->Hiding()");
        return NOERROR;
    }

    STDMETHOD(GetInfo)(IMINFO* pimi)
    {
        LOG(L"->GetInfo(%x)", pimi);
        if(!pimi)
            return E_FAIL;

        ZeroMemory(pimi, sizeof(IMINFO));
        pimi->cbSize            = sizeof(IMINFO);
        pimi->fdwFlags          = SIPF_DOCKED | SIPF_ON;
#ifdef _WIN32_WCE
        InitImageLists();
        pimi->hImageNarrow      = (HANDLE)m_hilNarrow;
        pimi->hImageWide        = (HANDLE)m_hilWide;
#endif
        pimi->rcSipRect.right   = m_config->m_size.cx*m_vga;
        pimi->rcSipRect.bottom  = m_config->m_size.cy*m_vga;
        return NOERROR;
    }

    void KeyboardSizeChanged()
    {
        if (m_pIMCallback)
        {
            IMINFO imi;
            ZeroMemory(&imi, sizeof(IMINFO));
            imi.cbSize              = sizeof(IMINFO);
            imi.fdwFlags            = SIPF_DOCKED | SIPF_ON;
#ifdef _WIN32_WCE
            InitImageLists();
            imi.hImageNarrow        = (HANDLE)m_hilNarrow;
            imi.hImageWide          = (HANDLE)m_hilWide;
#endif
            //RECT r;
            //GetWindowRect(GetParent(m_hwndMain), &r);
            LOG(L"m_rsi=%d %d m_rsiVD=%d %d m_config->m_size=%d %d",
                RectWidth(m_rsi), RectHeight(m_rsi),
                RectWidth(m_rsiVD), RectHeight(m_rsiVD),
                m_config->m_size.cx, m_config->m_size.cy);
            imi.rcSipRect.left      = max(0, (RectWidth(m_rsiVD)-m_config->m_size.cx*m_vga)/2);
            imi.rcSipRect.top       = m_rsi.bottom - m_config->m_size.cy*m_vga;
            imi.rcSipRect.right     = imi.rcSipRect.left    + m_config->m_size.cx*m_vga;
            imi.rcSipRect.bottom    = m_rsi.bottom;

            m_pIMCallback->SetImInfo(&imi);
        }
    }

    STDMETHOD(ReceiveSipInfo)(SIPINFO* psi)
    {
        if(!psi)
        {
            return E_FAIL;
        }

        LOG(L"->ReceiveSipInfo("
            L"  cbSize           = %d\n"
            L"  fdwFlags         = %x\n"
            L"  rcVisibleDesktop = %d %d %d %d\n"
            L"  rcSipRect        = %d %d %d %d\n"
            L"  dwImDataSize     = %d\n"
            L"  pvImData         = %x\n)",
                psi->cbSize,
                psi->fdwFlags,
                psi->rcVisibleDesktop.left, psi->rcVisibleDesktop.top, psi->rcVisibleDesktop.right, psi->rcVisibleDesktop.bottom,
                psi->rcSipRect.left,        psi->rcSipRect.top,        psi->rcSipRect.right,        psi->rcSipRect.bottom,
                psi->dwImDataSize,
                psi->pvImData);

        m_rsiVD = psi->rcVisibleDesktop; // save it
        m_rsi = psi->rcSipRect; // save it
        const SIZE sz = { RectWidth(psi->rcSipRect), RectHeight(psi->rcSipRect) };

        if(sz.cx<480 && m_vga>1)
            ChangeResolution(1);
        else if(sz.cx>=480 && m_vga<2)
            ChangeResolution(2);

        MoveWindow(m_hwndMain, 0, 0, sz.cx, sz.cy, FALSE );

        if (sz.cx != m_config->m_size.cx*m_vga ||
            sz.cy != m_config->m_size.cy*m_vga)
        {
            KeyboardSizeChanged();
        }
        return NOERROR;
    }

    STDMETHOD(RegisterCallback)(IIMCallback* lpIMCallback)
    {
        LOG(L"->RegisterCallback(0x%X)", lpIMCallback);
        m_pIMCallback = lpIMCallback;
        return NOERROR;
    }

    STDMETHOD(GetImData)(DWORD dwSize, void* pvImData)
    {
        LOG(L"->GetImData(0x%X, 0x%X)", dwSize, pvImData);
        return E_NOTIMPL;
    }

    STDMETHOD(SetImData)(DWORD dwSize, void* pvImData)
    {
        LOG(L"->SetImData(0x%X, 0x%X)", dwSize, pvImData);
        return E_NOTIMPL;
    }

    STDMETHOD(UserOptionsDlg)(HWND hwndParent)
    {
        D(L"UserOptionsDlg");
        return E_NOTIMPL;
    }

	unsigned char GetGesture(short x, short y);
};
const WCHAR     CKbd::s_szMainWndClassName[] = L"eurokbd_main";
const WCHAR     CKbd::s_szPopWndClassName[]  = L"eurokbd_pop";



STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    LOG(L"DllGetClassObject()");
    if( IsEqualCLSID(rclsid, CLSID_EuroKbd))
    {
        CClassFactory<CKbd>* pCF = new CClassFactory<CKbd>;
        HRESULT hr = pCF->QueryInterface(riid, ppv);
        pCF->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
    if( G_OBJECTCOUNT )
    {
        LOG(L"DllCanUnloadNow() -> S_FALSE");
        return S_FALSE;
    }
    LOG(L"DllCanUnloadNow() -> S_OK");
    return S_OK;
}

STDAPI DllUnregisterServer()
{
#ifdef _WIN32_WCE
    WCHAR swBase[64];
    _snwprintf( swBase,
                64,
                L"CLSID\\{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                CLSID_EuroKbd.Data1,
                CLSID_EuroKbd.Data2,
                CLSID_EuroKbd.Data3,
                CLSID_EuroKbd.Data4[0],
                CLSID_EuroKbd.Data4[1],
                CLSID_EuroKbd.Data4[2],
                CLSID_EuroKbd.Data4[3],
                CLSID_EuroKbd.Data4[4],
                CLSID_EuroKbd.Data4[5],
                CLSID_EuroKbd.Data4[6],
                CLSID_EuroKbd.Data4[7]);
    RegDeleteKey(HKEY_CLASSES_ROOT, swBase);

    WCHAR swStartup[MAX_PATH];
    WCHAR swLnk[MAX_PATH];
    if( SHGetSpecialFolderPath(0, swStartup, CSIDL_STARTUP, FALSE) &&
        PathCombine(swLnk, swStartup, L"startkey.exe.lnk") )
    {
        DeleteFile(swLnk);
    }
#endif
    return S_OK;
}


STDAPI DllRegisterServer()
{
#ifdef _WIN32_WCE
    WCHAR swBase[64];
    _snwprintf( swBase,
                64,
                L"CLSID\\{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                CLSID_EuroKbd.Data1,
                CLSID_EuroKbd.Data2,
                CLSID_EuroKbd.Data3,
                CLSID_EuroKbd.Data4[0],
                CLSID_EuroKbd.Data4[1],
                CLSID_EuroKbd.Data4[2],
                CLSID_EuroKbd.Data4[3],
                CLSID_EuroKbd.Data4[4],
                CLSID_EuroKbd.Data4[5],
                CLSID_EuroKbd.Data4[6],
                CLSID_EuroKbd.Data4[7]);

    CRegKey rk;

    if(!(rk.Open(HKEY_CLASSES_ROOT, KEY_WRITE, swBase) &&
         rk.SetString(L"", REG_SZ, L"Keyboard - \x20AC") &&
         rk.Open(HKEY_CLASSES_ROOT, KEY_WRITE, L"%s\\InProcServer32", swBase) &&
         rk.SetString(L"", REG_SZ, L"%s", g_pwDllFile) &&
         rk.Open(HKEY_CLASSES_ROOT, KEY_WRITE, L"%s\\DefaultIcon", swBase) &&
         rk.SetString(L"", REG_SZ, L"%s,0", g_pwDllFile) &&
         rk.Open(HKEY_CLASSES_ROOT, KEY_WRITE, L"%s\\IsSIPInputMethod", swBase) &&
         rk.SetString(L"", REG_SZ, L"1")
        ))
    {
        DllUnregisterServer();
        return E_FAIL;
    }

    WCHAR swStartup[MAX_PATH];
    WCHAR swLnk[MAX_PATH];
    WCHAR swStartKeyExe[MAX_PATH+2];
    if( PathCombine(swStartKeyExe+1, g_pwDllDir, L"startkey.exe") &&
        (swStartKeyExe[0]='"', wcscat(swStartKeyExe,L"\""), TRUE) &&
        SHGetSpecialFolderPath(0, swStartup, CSIDL_STARTUP, FALSE) &&
        PathCombine(swLnk, swStartup, L"startkey.exe.lnk") &&
        (DeleteFile(swLnk), SHCreateShortcut(swLnk, swStartKeyExe))
        /* && SetFileAttributes(swLnk, FILE_ATTRIBUTE_READONLY) */
        )
    {
        SHELLEXECUTEINFO sei;
        ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.lpFile = swStartKeyExe;
        ShellExecuteEx(&sei);

        SipShowIM(SIPF_ON);
    }
#endif
    return S_OK;
}


extern "C"
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
//  WCHAR sw[MAX_PATH];
//  GetModuleFileName(0, sw, MAX_PATH-1);
//  LOG(L"DllMain(%x,%x) %s",hInstance, dwReason, sw);

    if(dwReason==DLL_PROCESS_ATTACH)
    {
        g_hInstDll = (HINSTANCE)hInstance;
#ifndef _WIN32_WCE
        DisableThreadLibraryCalls(g_hInstDll);
#endif
        if(!g_pwDllDir)
        {
            WCHAR szDllFileName[MAX_PATH];

            GetModuleFileName(g_hInstDll, szDllFileName, MAX_PATH-1);

            g_pwDllFile = _wcsdup(szDllFileName);

            for(int i=wcslen(szDllFileName)-1; i>=0; i--)
            {
                if(szDllFileName[i]=='\\' || szDllFileName[i]=='/')
                {
                    szDllFileName[i] = '\0';
                    break;
                }
                szDllFileName[i] = '\0';
            }
            g_pwDllDir = _wcsdup(szDllFileName);
        }
    }
    return TRUE;
}
int sqr(const short x) {return x*x;};

BYTE CKbd::GetGesture(short x, short y)
{	POINT pt = {m_ptBtnDownPos.x/m_vga,m_ptBtnDownPos.y/m_vga};
    ScreenToClient(m_hwndMain,&pt);
	int d = sqr(x - pt.x) + sqr(y - pt.y); //квадрат расстояния от точки касания
    //BOX(L"GetGesture", L"%d %d %d %d %d", d, m_ptBtnDownPos.x, m_ptBtnDownPos.y,x,y);
	if (d < 400) { return 0;};
	if (y < m_keyCurrent->top)
	  { return ((x > m_keyCurrent->left)&& (x < m_keyCurrent->right))?1:5;}
	else 
	  { if (y < m_keyCurrent->bottom)
	//жест влево
	    { if ((x < m_keyCurrent->left)) 
	      { return 2;//SendText(L"\b");
	      } else 
	      { return (x > m_keyCurrent->right)?3:0;}//жест вправо
	    }	else 
	    { return ((x > m_keyCurrent->left)&& (x < m_keyCurrent->right))?4:5;};
	  };
}

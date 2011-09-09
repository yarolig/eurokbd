#include "config.h"
#include "deq.h"


#define SK_GROUP_LATIN     0   // базовый латинский
#define SK_GROUP_LATIN_EX  1   // чешский, турецкий, вьетнамский, ...
#define SK_GROUP_DIGIT     2   // цифры
#define SK_GROUP_SMALL     3   // Ctrl-, Alt-
#define SK_GROUP_CYRILLIC  4   // русский
#define SK_GROUP_CYR_EX    5   // казахский, хорватский, украинский, ...
#define SK_GROUP_GREEK     6
#define SK_GROUP_THAI      7
#define SK_GROUP_ARABIC    8
#define SK_GROUP_HEBREW    9
#define SK_GROUP_CONTROL   10
#define SK_GROUP_ALT       11
#define SK_GROUP_SHIFT     12
#define SK_GROUP_MAX       13


union keydata
{
    WCHAR* pw;
    UINT w;
};

class SUBKEYENTRY : public listentry<SUBKEYENTRY>
{
public:
    SHORT       left, top, right, bottom;  // тут может быть отрицательное число, посему будет SHORT
    BYTE        group;
    BYTE        vk;
    keydata     data;
    keydata     desc;

    SUBKEYENTRY()
    {
        data.w  = 0;
        desc.w  = 0;
    }
    ~SUBKEYENTRY()
    {
        if(data.w >= 0xFFFF) free((void*)data.pw);
        if(desc.w >= 0xFFFF) free((void*)desc.pw);
    }
};

class KEYENTRY : public deqentry<KEYENTRY>
{
public:
    BYTE        vk;
    keydata     data;
    SHORT       left, top, right, bottom;
    keydata     desc;
    keydata     desc2;
    BYTE        group;
    list<SUBKEYENTRY>        subkeys;
    //BYTE        left2, top2, right2, bottom2;

    KEYENTRY()
    {
        data.w  = 0;
        desc.w  = 0;
        desc2.w = 0;
    }
    ~KEYENTRY()
    {
        if(data.w >= 0xFFFF) free((void*)data.pw);
        if(desc.w >= 0xFFFF) free((void*)desc.pw);
        if(desc2.w >= 0xFFFF) free((void*)desc2.pw);

        subkeys.destroy();
    }
};

inline bool ishex(WCHAR c) { return ('0'<=c && c<='9') || ('A'<=c && c<='F') || ('a'<=c && c<='f'); }

inline int fromhex(WCHAR c)
{
    if('0'<=c && c<='9')
        return c - '0';
    if('A'<=c && c<='F')
        return c - 'A' + 10;
    if('a'<=c && c<='f')
        return c - 'a' + 10;
    return 0;
}

inline void* _readfile(HANDLE hFileIn, DWORD *pdwFileSize)
{
  BYTE* pBin = 0;
  if(hFileIn!=INVALID_HANDLE_VALUE)
  {
    DWORD dwFileSize = GetFileSize(hFileIn,NULL);
    pBin = (BYTE*)malloc(dwFileSize+4);
    if(pBin)
    {
      if( !ReadFile(hFileIn,pBin,dwFileSize,&dwFileSize,NULL) )
      {
        dwFileSize=0;
      }
      CloseHandle(hFileIn);
      if(pdwFileSize) *pdwFileSize=dwFileSize;
      pBin[dwFileSize] = 0;
      pBin[dwFileSize+1] = 0;
      pBin[dwFileSize+2] = 0;
      pBin[dwFileSize+3] = 0;
    }
  }
  return pBin;
}

inline void* readfileW(const wchar_t* pcwFileName, DWORD *pdwFileSize=0)
{
    HANDLE hFileIn = CreateFileW(pcwFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    return _readfile(hFileIn, pdwFileSize);
}


// перекодировка UTF16LE, UTF16BE, UTF8 в UTF16LE
// вход и выход: malloc-строка
static LPWSTR strToUTF16LE(void* p)
{
    BYTE *pb = (BYTE *)p;

    if( pb[0]==0xFF && pb[1]==0xFE ) // UTF16LE, ok
    {
        return (LPWSTR)p;
    }
    if( pb[0]==0xFE && pb[1]==0xFF ) // UTF16BE, recode in place
    {
        pb += 2;
        for(BYTE* q=pb; q[0] || q[1]; q+=2)
        {
            char c = q[0];
            q[0] = q[1];
            q[1] = c;
        }
        return (LPWSTR)pb;
    }
    if( pb[0]==0xEF && pb[1]==0xBB && pb[2]==0xBF ) // UTF8, recode
    {
        pb += 3;
        size_t iLen = strlen((const char*)pb);
        WCHAR* pw = (WCHAR*)malloc((iLen+1)*2);
        WCHAR* qw = pw;
        for(const BYTE* q=pb; *q; q++)
        {
            if((q[0]&0x80)==0x00)
            {
                *qw++ = q[0];
            } else if((q[0]&0xE0)==0xC0 && (q[1]&0xC0)==0x80)
            {
                *qw++ = ((q[0]&0x1F)<<6) | (q[1]&0x3F);
                q += 1;
            } else if((q[0]&0xF0)==0xE0 && (q[1]&0xC0)==0x80 && (q[2]&0xC0)==0x80)
            {
                *qw++ = ((q[0]&0x0F)<<12) | ((q[1]&0x3F)<<6) | (q[2]&0x3F);
                q += 2;
            } else if((q[0]&0xF8)==0xF0 && (q[1]&0xC0)==0x80 && (q[2]&0xC0)==0x80 && (q[3]&0xC0)==0x80)
            {
                *qw++ = ((q[0]&0x07)<<18) | ((q[1]&0x3F)<<12) | ((q[2]&0x3F)<<6) | (q[3]&0x3F);
                q += 3;
            } else
            {
                *qw++ = L'?';
            }
        }
        *qw = L'\0';
        assert(qw <= pw + iLen);
        free(p);
        return pw;
    }
    return (LPWSTR)p;
}

class CParseError
{
    int m_errcode;
    //WCHAR m_sw[64];
public:
    CParseError(int errcode)
    {
        m_errcode = errcode;
    }
    int errcode() const { return m_errcode; }
    //CParseError(LPCWSTR fmt, ...);
};

static PWCHAR trystr(LPCWSTR p, int len, bool bNoThrowException = false)
{
    if(len>=3 && p[0]=='"' && p[len-1]=='"')
    {
        p++,len-=2;
        PWCHAR pw = new WCHAR[len+1];
        PWCHAR pw0 = pw;
        for(; len; p++,len--)
        {
            if(*p=='\\')
            {
                p++,len--;
                if(!len) break;
                if(*p=='t')
                {
                    *pw++ = '\t', p+=1, len-=1;
                    if(!len) break;
                } else if(*p=='n')
                {
                    *pw++ = '\n', p+=1, len-=1;
                    if(!len) break;
                } else if(*p=='r')
                {
                    *pw++ = '\r', p+=1, len-=1;
                    if(!len) break;
                } else if(*p=='x' && len>=3 && ishex(p[1]) && ishex(p[2]))
                {
                    *pw++ = (fromhex(p[1])<<4) | fromhex(p[2]);
                    p+=3,len-=3;
                    if(!len) break;
                } else if(*p=='u' && len>=5 && ishex(p[1]) && ishex(p[2]) && ishex(p[3]) && ishex(p[4]))
                {
                    *pw++ = (fromhex(p[1])<<12) | (fromhex(p[2])<<8) | (fromhex(p[3])<<4) | fromhex(p[4]);
                    p+=5,len-=5;
                    if(!len) break;
                }
            }
            *pw++ = *p;
        }
        *pw = 0;
        return pw0;
    }
    if(bNoThrowException)
        return 0;
    else
        throw CParseError(__LINE__);
}

static int tryint(LPCWSTR p, int len)
{
    int rc = 0;
    bool bMinus = false;

    if(len>=1 && p[0]=='-') p++,len--,bMinus=true;

    if(len>=3 && p[0]=='0' && (p[1]=='x' || p[1]=='X'))
    {
        if(len>10)
            throw CParseError(__LINE__);

        for(p+=2,len-=2; len; p++,len--)
        {
            rc <<= 4;
            if( !ishex(*p) )
                throw CParseError(__LINE__);

            rc += fromhex(*p);
        }
        if(bMinus) rc = -rc;
        return rc;
    } else if(len>=3 && p[0]=='\'' && p[len-1]=='\'')
    {
        if(len>6)
            throw CParseError(__LINE__);

        for(p++,len-=2; len; p++,len--)
        {
            if(*p=='\\') p++, len--;
            if(!len) break;
            rc <<= 8;
            rc += USHORT(*p);
        }
        return rc;
    } else
    {
        for(; len; *p++,len--)
        {
            rc *= 10;
            if(!('0'<=*p && *p<='9'))
                throw CParseError(__LINE__);

            rc += *p - '0';
        }
        if(bMinus) rc = -rc;
        return rc;
    }
//    throw CParseError(__LINE__);
}

static keydata trykdata(LPCWSTR p, int len)
{
    keydata kd = {0};
    if(!(len==0 || (len==1 && p[0]=='0') || (len>=1 && p[0]=='#'))) // остаётся ноль
    {
        kd.pw = trystr(p, len, true);
        if(0==kd.pw)
        {
            kd.w = tryint(p, len) & 0xFFFF;
        }
    }
    return kd;
}

static COLORREF intToColorref(int x)
{
    return ((x<<16)&0xFF0000) | (x&0x00FF00) | ((x>>16)&0x0000FF);
}


class CKbdConfig : public CConfig
{
private:
    CKbdConfig(const CKbdConfig &);
public:
    deq<KEYENTRY>       m_keys;
    CPtr<WCHAR>         m_pwSkinVGA;        // имя файла со скином для 240xXXX (и 320xXXX)
    CPtr<WCHAR>         m_pwSkinQVGA;       // имя файла со скином для 480xXXX

    SIZE                m_size;

    // пока? константа
    RECT                m_rPopBorder;       // это не настоящий прямоугольнег, это ширина бордюра с 4-х сторон
    COLORREF            m_colorPopBorder;   // а это его цвет

    COLORREF            m_colors[SK_GROUP_MAX][2];
    COLORREF            m_colorDesc;
    COLORREF            m_colorDesc2;

    CPtr<WCHAR>         m_pwFontBig;
    LONG                m_iFontBig;
    bool                m_bFontBigBold;

    CPtr<WCHAR>         m_pwFontSmall;
    LONG                m_iFontSmall;
    bool                m_bFontSmallBold;

    CPtr<WCHAR>         m_pwFontIndex;
    LONG                m_iFontIndex;
    bool                m_bFontIndexBold;
    
    bool                m_GesturesOn;


protected:
    static struct keyentry  default_keys[];

    void SetDefaultKeys()
    {
        struct key_s
        {
            BYTE vk, data, left, top, right, bottom, group;
            LPWSTR desc;
        };
#ifndef _WIN32_WCE
#define VK_LBRACKET     VK_OEM_4
#define VK_RBRACKET     VK_OEM_6
#define VK_HYPHEN       VK_OEM_MINUS
#define VK_SEMICOLON    VK_OEM_1
#define VK_APOSTROPHE   VK_OEM_7
#define VK_COMMA        VK_OEM_COMMA
#define VK_PERIOD       VK_OEM_PERIOD
#define VK_SLASH        VK_OEM_2
#endif
        const key_s keys[] = {
        {   '1',          '1',    0,  0,  20, 16,  SK_GROUP_DIGIT, 0        },
        {   '2',          '2',   20,  0,  40, 16,  SK_GROUP_DIGIT, 0        },
        {   '3',          '3',   40,  0,  60, 16,  SK_GROUP_DIGIT, 0        },
        {   '4',          '4',   60,  0,  80, 16,  SK_GROUP_DIGIT, 0        },
        {   '5',          '5',   80,  0, 100, 16,  SK_GROUP_DIGIT, 0        },
        {   '6',          '6',  100,  0, 120, 16,  SK_GROUP_DIGIT, 0        },
        {   '7',          '7',  120,  0, 140, 16,  SK_GROUP_DIGIT, 0        },
        {   '8',          '8',  140,  0, 160, 16,  SK_GROUP_DIGIT, 0        },
        {   '9',          '9',  160,  0, 180, 16,  SK_GROUP_DIGIT, 0        },
        {   '0',          '0',  180,  0, 200, 16,  SK_GROUP_DIGIT, 0        },
        {   VK_HYPHEN,    '-',  200,  0, 220, 16,  SK_GROUP_DIGIT, 0        },
        {   VK_BACK,      0x08, 220,  0, 240, 16,  SK_GROUP_SMALL, 0        },
        {   'Q',          'q',    0, 16,  20, 32,  SK_GROUP_LATIN, 0        },
        {   'W',          'w',   20, 16,  40, 32,  SK_GROUP_LATIN, 0        },
        {   'E',          'e',   40, 16,  60, 32,  SK_GROUP_LATIN, 0        },
        {   'R',          'r',   60, 16,  80, 32,  SK_GROUP_LATIN, 0        },
        {   'T',          't',   80, 16, 100, 32,  SK_GROUP_LATIN, 0        },
        {   'Y',          'y',  100, 16, 120, 32,  SK_GROUP_LATIN, 0        },
        {   'U',          'u',  120, 16, 140, 32,  SK_GROUP_LATIN, 0        },
        {   'I',          'i',  140, 16, 160, 32,  SK_GROUP_LATIN, 0        },
        {   'O',          'o',  160, 16, 180, 32,  SK_GROUP_LATIN, 0        },
        {   'P',          'p',  180, 16, 200, 32,  SK_GROUP_LATIN, 0        },
        {   VK_LBRACKET,  '[',  200, 16, 220, 32,  SK_GROUP_DIGIT, 0        },
        {   VK_RBRACKET,  ']',  220, 16, 240, 32,  SK_GROUP_DIGIT, 0        },
        {   VK_RETURN,    '\r', 220, 32, 240, 64,  SK_GROUP_DIGIT, 0        },
        {   'A',          'a',    0, 32,  20, 48,  SK_GROUP_LATIN, 0        },
        {   'S',          's',   20, 32,  40, 48,  SK_GROUP_LATIN, 0        },
        {   'D',          'd',   40, 32,  60, 48,  SK_GROUP_LATIN, 0        },
        {   'F',          'f',   60, 32,  80, 48,  SK_GROUP_LATIN, 0        },
        {   'G',          'g',   80, 32, 100, 48,  SK_GROUP_LATIN, 0        },
        {   'H',          'h',  100, 32, 120, 48,  SK_GROUP_LATIN, 0        },
        {   'J',          'j',  120, 32, 140, 48,  SK_GROUP_LATIN, 0        },
        {   'K',          'k',  140, 32, 160, 48,  SK_GROUP_LATIN, 0        },
        {   'L',          'l',  160, 32, 180, 48,  SK_GROUP_LATIN, 0        },
        {   VK_SEMICOLON, ';',  180, 32, 200, 48,  SK_GROUP_DIGIT, 0        },
        {   VK_APOSTROPHE,'\'', 200, 32, 220, 48,  SK_GROUP_DIGIT, 0        },
        {   'Z',          'z',    0, 48,  20, 64,  SK_GROUP_LATIN, 0        },
        {   'X',          'x',   20, 48,  40, 64,  SK_GROUP_LATIN, 0        },
        {   'C',          'c',   40, 48,  60, 64,  SK_GROUP_LATIN, 0        },
        {   'V',          'v',   60, 48,  80, 64,  SK_GROUP_LATIN, 0        },
        {   'B',          'b',   80, 48, 100, 64,  SK_GROUP_LATIN, 0        },
        {   'N',          'n',  100, 48, 120, 64,  SK_GROUP_LATIN, 0        },
        {   'M',          'm',  120, 48, 140, 64,  SK_GROUP_LATIN, 0        },
        {   VK_COMMA,     ',',  140, 48, 160, 64,  SK_GROUP_DIGIT, 0        },
        {   VK_PERIOD,    '.',  160, 48, 180, 64,  SK_GROUP_DIGIT, 0        },
        {   VK_SLASH,     '/',  180, 48, 200, 64,  SK_GROUP_DIGIT, 0        },
        {   VK_UP,        0,    200, 48, 220, 64,  SK_GROUP_SMALL, L"\x2191"},
        {   _VK_MOD,      0,      0, 64,  32, 80,  SK_GROUP_SMALL, L"Mod"   },
        {   VK_SPACE,     ' ',   32, 64, 180, 80,  SK_GROUP_DIGIT, 0        },
        {   VK_LEFT,      0,    180, 64, 200, 80,  SK_GROUP_SMALL, L"\x2190"},
        {   VK_DOWN,      0,    200, 64, 220, 80,  SK_GROUP_SMALL, L"\x2193"},
        {   VK_RIGHT,     0,    220, 64, 240, 80,  SK_GROUP_SMALL, L"\x2192"},
        {   0} };

        for(const key_s* p=keys; p->vk; p++)
        {
            KEYENTRY* pk = new KEYENTRY;
            pk->vk        = p->vk;
            pk->data.w    = p->data;
            pk->left      = p->left;
            pk->top       = p->top;
            pk->right     = p->right;
            pk->bottom    = p->bottom;
            pk->group     = p->group;
            if(!p->desc)
                pk->desc.pw  = 0;
            else if(p->desc[0] && p->desc[1]==0)
                pk->desc.w  = p->desc[0];
            else
                pk->desc.pw   = _wcsdup(p->desc);
            pk->desc2.pw  = 0;
            m_keys.add_first(pk);
        }
    }
    void SetDefaultEmpty()
    {
        m_keys.destroy();

        m_pwSkinQVGA = 0;
        m_pwSkinVGA = 0;

        // TODO: загрузка всего этого из файла
        m_size.cx = 240; // QVGA units
        m_size.cy = 80; // QVGA units

        m_rPopBorder.left  = 4;
        m_rPopBorder.top   = 4;
        m_rPopBorder.right = 4;
        m_rPopBorder.bottom= 4;
        m_colorPopBorder   = RGB(0x00,0x00,0x99); //RGB(0xFF,0x66,0x00);

        m_colors[SK_GROUP_LATIN   ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_LATIN   ][1] = RGB(0xFF,0xFF,0xFF);
        m_colors[SK_GROUP_LATIN_EX][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_LATIN_EX][1] = RGB(0xFF,0xFF,0xFF);
        m_colors[SK_GROUP_DIGIT   ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_DIGIT   ][1] = RGB(0xE0,0xE0,0xE0);
        m_colors[SK_GROUP_SMALL   ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_SMALL   ][1] = RGB(0xCC,0xCC,0xFF);
        m_colors[SK_GROUP_CYRILLIC][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_CYRILLIC][1] = RGB(0x66,0xFF,0x66);
        m_colors[SK_GROUP_CYR_EX  ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_CYR_EX  ][1] = RGB(0x66,0x99,0x66);
        m_colors[SK_GROUP_GREEK   ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_GREEK   ][1] = RGB(0xFF,0xFF,0x00);
        m_colors[SK_GROUP_THAI    ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_THAI    ][1] = RGB(0xFF,0xFF,0xFF);
        m_colors[SK_GROUP_ARABIC  ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_ARABIC  ][1] = RGB(0xFF,0xFF,0xFF);
        m_colors[SK_GROUP_HEBREW  ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_HEBREW  ][1] = RGB(0xFF,0xFF,0xFF);
        m_colors[SK_GROUP_CONTROL ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_CONTROL ][1] = RGB(0xFF,0x00,0xFF);
        m_colors[SK_GROUP_ALT     ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_ALT     ][1] = RGB(0xFF,0x00,0xFF);
        m_colors[SK_GROUP_SHIFT   ][0] = RGB(0x00,0x00,0x00);
        m_colors[SK_GROUP_SHIFT   ][1] = RGB(0xFF,0x00,0xFF);

        m_pwFontBig     = _wcsdup(L"Tahoma");
        m_iFontBig      = 12;
        m_bFontBigBold  = false;

        m_pwFontSmall   = _wcsdup(L"Tahoma");
        m_iFontSmall    = 8;
        m_bFontSmallBold= false;

        m_pwFontIndex   = _wcsdup(L"Tahoma");
        m_iFontIndex    = 10;
        m_bFontIndexBold= false;

        m_colorDesc  = RGB(0x99, 0x99, 0x99);
        m_colorDesc2 = RGB(0x00, 0x99, 0x00);
        
        m_GesturesOn=false;
    }


    virtual void _SetDefaults()
    {
        SetDefaultEmpty();
        SetDefaultKeys();
    }

    #define MAXITEMS 16

    static inline bool CMP(LPCWSTR p, size_t plen, LPCWSTR s, size_t slen)
    {
        return slen==plen && 0==memcmp(p, s, plen*2);
    }

    static int strToSkGroup(const WCHAR* p, size_t len)
    {
        if( CMP(p, len, L"LATIN",    5) ) return SK_GROUP_LATIN;
        if( CMP(p, len, L"LATIN_EX", 8) ) return SK_GROUP_LATIN_EX;
        if( CMP(p, len, L"DIGIT",    5) ) return SK_GROUP_DIGIT;
        if( CMP(p, len, L"SMALL",    5) ) return SK_GROUP_SMALL;
        if( CMP(p, len, L"CYRILLIC", 8) ) return SK_GROUP_CYRILLIC;
        if( CMP(p, len, L"CYR_EX",   6) ) return SK_GROUP_CYR_EX;
        if( CMP(p, len, L"GREEK",    5) ) return SK_GROUP_GREEK;
        if( CMP(p, len, L"THAI",     4) ) return SK_GROUP_THAI;
        if( CMP(p, len, L"ARABIC",   6) ) return SK_GROUP_ARABIC;
        if( CMP(p, len, L"HEBREW",   6) ) return SK_GROUP_HEBREW;
        if( CMP(p, len, L"CONTROL",  7) ) return SK_GROUP_CONTROL;
        if( CMP(p, len, L"ALT",      3) ) return SK_GROUP_ALT;
        if( CMP(p, len, L"SHIFT",    5) ) return SK_GROUP_SHIFT;
        return -1;
    }

    void processline(LPCWSTR pline, int len)
    {
        while(len && USHORT(pline[0])<=' ') pline++, len--;       // нахуй пробелы в начале строки
        while(len && USHORT(pline[len-1])<=' ') len--;            // ---//----//-- в конце строки
        if(!len)
        {
            return;
        }

        const WCHAR* ptrs[MAXITEMS] = {0};
        size_t      lens[MAXITEMS] = {0};
        int         nitems;

        for(nitems=0; len && nitems<=MAXITEMS; nitems++)
        {
            ptrs[nitems] = pline;

            // пробелы внутри кавычек
            if(len && (pline[0]==L'"' || pline[0]==L'\''))
            {
                WCHAR wQuote = pline[0];
                pline++, len--;
                while(len && USHORT(pline[0])>=' ')
                {
                    if(len>=2 && pline[0]=='\\' && pline[1]==wQuote) // искейпнутая кавычка
                    {
                        pline+=2, len-=2;
                    } else
                    {
                        pline++, len--;
                        if(pline[-1]==wQuote) break;
                    }
                }
            } else
            {
                while(len && USHORT(pline[0])>' ') pline++, len--;    // не пробелы
            }
            lens[nitems] = pline-ptrs[nitems];
            while(len && USHORT(pline[0])<=' ') pline++, len--;       // пробелы
        }

        if( CMP(ptrs[0], lens[0], L"=", 1))
        {
            if(nitems<8) throw CParseError(__LINE__);

            int     vk     = tryint  (ptrs[1], lens[1]);
            keydata data   = trykdata(ptrs[2], lens[2]);
            int     left   = tryint  (ptrs[3], lens[3]);
            int     top    = tryint  (ptrs[4], lens[4]);
            int     right  = tryint  (ptrs[5], lens[5]);
            int     bottom = tryint  (ptrs[6], lens[6]);
            keydata desc   = {0};
            keydata desc2  = {0};

            int group = strToSkGroup(ptrs[7], lens[7]);
            if(group==-1) throw CParseError(__LINE__);

            if(nitems>=9)
            {
                desc = trykdata(ptrs[8], lens[8] );
            }
            if(nitems>=10)
            {
                desc2 = trykdata(ptrs[9], lens[9] );
            }

            KEYENTRY* pk = new KEYENTRY;
            pk->vk        = vk;
            pk->data      = data;
            pk->left      = left;
            pk->top       = top;
            pk->right     = right;
            pk->bottom    = bottom;
            pk->group     = group;
            pk->desc      = desc;
            pk->desc2     = desc2;
            m_keys.add_first(pk);
        } else if( CMP(ptrs[0], lens[0], L"+", 1))
        {
            if(nitems<8) throw CParseError(__LINE__);

            int     vk     = tryint  (ptrs[1], lens[1]);
            keydata data   = trykdata(ptrs[2], lens[2]);
            int     left   = tryint  (ptrs[3], lens[3]);
            int     top    = tryint  (ptrs[4], lens[4]);
            int     right  = tryint  (ptrs[5], lens[5]);
            int     bottom = tryint  (ptrs[6], lens[6]);
            keydata desc   = {0};

            int group = strToSkGroup(ptrs[7], lens[7]);
            if(group==-1) throw CParseError(__LINE__);

            if(nitems>=9)
            {
                desc = trykdata(ptrs[8], lens[8] );
            }

            if(!m_keys.first)  // subkey есть, а key перед ним нет
                throw CParseError(__LINE__);

            SUBKEYENTRY* psk = new SUBKEYENTRY;
            psk->vk     = vk;
            psk->data   = data;
            psk->left   = left;
            psk->top    = top;
            psk->right  = right;
            psk->bottom = bottom;
            psk->group  = group;
            psk->desc   = desc;

            m_keys.first->subkeys.add_first(psk);
        } else if( CMP(ptrs[0], lens[0], L"SKINQVGA", 8))
        {
            m_pwSkinQVGA = trystr(ptrs[1], lens[1]);
        } else if( CMP(ptrs[0], lens[0], L"SKINVGA", 7))
        {
            m_pwSkinVGA = trystr(ptrs[1], lens[1]);
        } else if( CMP(ptrs[0], lens[0], L"SIZE", 4))
        {
            m_size.cx = tryint(ptrs[1], lens[1]);
            m_size.cy = tryint(ptrs[2], lens[2]);
        } else if( CMP(ptrs[0], lens[0], L"POPBORDER", 9))
        {
            m_rPopBorder.left   = tryint(ptrs[1], lens[1]);
            m_rPopBorder.top    = tryint(ptrs[2], lens[2]);
            m_rPopBorder.right  = tryint(ptrs[3], lens[3]);
            m_rPopBorder.bottom = tryint(ptrs[4], lens[4]);
            m_colorPopBorder    = intToColorref(tryint(ptrs[5], lens[5]));
        } else if( CMP(ptrs[0], lens[0], L"DESC", 4))
        {
            m_colorDesc   = tryint(ptrs[1], lens[1]);
            m_colorDesc2  = tryint(ptrs[2], lens[2]);
        } else if( CMP(ptrs[0], lens[0], L"FONT_BIG", 8))
        {
            m_pwFontBig         = trystr(ptrs[1], lens[1]);
            m_iFontBig          = tryint(ptrs[2], lens[2]);
            m_bFontBigBold      = CMP(ptrs[3], lens[3], L"BOLD", 4);
        } else if( CMP(ptrs[0], lens[0], L"FONT_SMALL", 10))
        {
            m_pwFontSmall       = trystr(ptrs[1], lens[1]);
            m_iFontSmall        = tryint(ptrs[2], lens[2]);
            m_bFontSmallBold    = CMP(ptrs[3], lens[3], L"BOLD", 4);
        } else if( CMP(ptrs[0], lens[0], L"FONT_INDEX", 10))
        {
            m_pwFontIndex       = trystr(ptrs[1], lens[1]);
            m_iFontIndex        = tryint(ptrs[2], lens[2]);
            m_bFontIndexBold    = CMP(ptrs[3], lens[3], L"BOLD", 4);
        } else if( CMP(ptrs[0], lens[0], L"GESTURES", 8))
        {
            m_GesturesOn    = CMP(ptrs[1], lens[1], L"ON", 2);
        }else
        {
            int group = strToSkGroup(ptrs[0], lens[0]);
            if(group!=-1)
            {
                m_colors[group][0] = intToColorref(tryint(ptrs[1], lens[1]));
                m_colors[group][1] = intToColorref(tryint(ptrs[2], lens[2]));
            } else
            {
                //throw CParseError(__LINE__);
            }
        }
    }

    virtual bool _Load()
    {
        bool rc = true;

        // чего-то может в файле не быть - тогда берутся значения по умолчанию, а не из прошлого конфига
        SetDefaultEmpty();

        int nline = 1;
        PWCHAR data = strToUTF16LE( readfileW(m_filename) );
        const WCHAR *p=data, *pline=data;
        try
        {
            while(1)
            {
                while(*p!='\0' && *p!='\r' && *p!='\n') p++;
                if(p!=pline)
                {
                    processline(pline, p-pline);
                }
                if(*p=='\0') break;
                while(*p=='\r' || *p=='\n') p++;
                pline = p;
                nline ++;
            }
        } catch(CParseError& e)
        {
            BOX(L"CONFIG ERROR", L"err %d in %s line %d\r\n[%.*s]", e.errcode(), m_filename, nline, p-pline, pline);
            rc = false;
            //_SetDefaults();
        }
        free(data);

        return rc;
    }

public:
    CKbdConfig(LPCWSTR filename) :
        CConfig(filename),
        m_keys()
    {
        SetDefaultEmpty();
    }

    ~CKbdConfig()
    {
        m_keys.destroy();
    }

    virtual bool _Save()
    {
        // not implemented
        return true;
    }
};

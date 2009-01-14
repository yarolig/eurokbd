#ifndef _MEMCHECK_H
#define _MEMCHECK_H

void BOX(LPCWSTR title, LPCWSTR fmt, ... );
void LOG(LPCWSTR fmt, ... );
void D(HWND hwnd, LPCWSTR fmt, ... );
void D(LPCWSTR fmt, ... );

#ifndef NDEBUG
#define ASSERT_PRINT(exp,file,line) ( \
    LOG(L"\\assert.log", L"%S (%d):\r\n%S\r\n", file, line, #exp), \
    BOX(L"ASSERTION FAILED", L"%S (%d):\r\n%S\r\n", file, line, #exp), \
    0)

#define ASSERT_AT(exp,file,line) (void)( (exp) || (ASSERT_PRINT(exp,file,line), ExitProcess(1), 0 ) )
#define assert(exp) ASSERT_AT(exp,__FILE__,__LINE__)
#define MEMCHECK 1
#else
#define ASSERT_AT(exp,file,line)
#define assert(exp)
#endif



#ifdef MEMCHECK

unsigned __int64 alloc_blocks = 0;
unsigned __int64 alloc_bytes = 0;
unsigned __int64 free_blocks = 0;
unsigned __int64 free_bytes = 0;

// для ARM указатель на DWORD должен быть aligned, а эти ф-ции могут работать с не-aligned указателем
inline void POKE32(void* p, unsigned i)
{
#ifdef ARM
    ((unsigned char*)p)[0] = i;
    ((unsigned char*)p)[1] = i>>8;
    ((unsigned char*)p)[2] = i>>16;
    ((unsigned char*)p)[3] = i>>24;
#else
    ((unsigned*)p)[0] = i;
#endif
}

inline unsigned PEEK32(void* p)
{
#ifdef ARM
    return  ((unsigned char*)p)[0] +
            (((unsigned char*)p)[1] << 8) +
            (((unsigned char*)p)[2] << 16) +
            (((unsigned char*)p)[3] << 24);
#else
    return ((unsigned*)p)[0];
#endif
}

__declspec(noinline) void *mc_malloc(size_t size, const char* file, int line)
{
    void* p = malloc(8+size+4);
    ASSERT_AT(p, file, line);
    memset(p, 0xCC, 8+size+4);
    *((unsigned char**)&p) += 8;
    POKE32((unsigned char*)p-8, size);
    POKE32((unsigned char*)p-4, 0xCCCCCCCC^(unsigned)p);
    POKE32((unsigned char*)p+size, 0xCCCCCCCC^(unsigned)p);
    alloc_blocks ++;
    alloc_bytes += size;
    return p;
}

__declspec(noinline) void mc_free(void *p, const char* file, int line)
{
    if(p!=0)
    {
        size_t oldsize = PEEK32((unsigned char*)p-8);
        ASSERT_AT( PEEK32((unsigned char*)p-4)==(0xCCCCCCCC^(unsigned)p), file, line);
        ASSERT_AT( PEEK32((unsigned char*)p+oldsize)==(0xCCCCCCCC^(unsigned)p), file, line);
        memset((unsigned char*)p-8, 0xCC, 8+oldsize+4);
        free((unsigned char*)p-8);
        free_blocks ++;
        free_bytes += oldsize;
    }
}

__declspec(noinline) void* mc_realloc(void *p, size_t size, const char* file, int line)
{
    size_t oldsize = PEEK32((unsigned char*)p-8);
    ASSERT_AT( size>oldsize, file, line);
    ASSERT_AT( PEEK32((unsigned char*)p-4)==(0xCCCCCCCC^(unsigned)p), file, line);
    ASSERT_AT( PEEK32((unsigned char*)p+oldsize)==(0xCCCCCCCC^(unsigned)p), file, line);
    p = realloc((char*)p-8, 8+size+4);
    memset((char*)p+8+oldsize+4, 0xCC, size-oldsize); // let be segmentation fault here :)

    *((unsigned char**)&p) += 8;
    *(size_t*)((char*)p-8)      = size;
    POKE32((unsigned char*)p-8, size);
    POKE32((unsigned char*)p-4, 0xCCCCCCCC^(unsigned)p);
    POKE32((unsigned char*)p+size, 0xCCCCCCCC^(unsigned)p);

    alloc_blocks ++;
    alloc_bytes += size;
    free_blocks ++;
    free_bytes += oldsize;

    return p;
}


__declspec(noinline) bool mc_check(const void *p)
{
    if(p==0)
        return true;
    size_t oldsize = PEEK32((unsigned char*)p-8);
    return PEEK32((unsigned char*)p-4)==(0xCCCCCCCC^(unsigned)p) && PEEK32((unsigned char*)p+oldsize)==(0xCCCCCCCC^(unsigned)p);
}

#define malloc(x)     mc_malloc((x),__FILE__,__LINE__)
#define free(p)       mc_free((p),__FILE__,__LINE__)
#define realloc(p,x)  mc_realloc((p),(x),__FILE__,__LINE__)
#define _check(p) assert(__check(p))


char* strdup(const char* str)
{
    char *mem;
    if (!str)
        return NULL;
    if (mem=(char*)malloc(strlen(str) + 1))
        return strcpy(mem,str);
    return NULL;
}

wchar_t* _wcsdup(const wchar_t* str)
{
    wchar_t *mem;
    if (!str)
        return NULL;
    if (mem=(wchar_t*)malloc(2*(wcslen(str) + 1)))
        return wcscpy(mem,str);
    return NULL;
}
#else
#define _check(p)
#endif // MEMCHECK

inline void* operator new(size_t size)
{
    return(malloc(size));
}

inline void* operator new[](size_t size)
{
    return(malloc(size));
}

inline void operator delete(void* mem)
{
    free(mem);
}

inline void operator delete[](void* mem)
{
    free(mem);
}


void BOX(LPCWSTR title, LPCWSTR fmt, ... )
{
    static WCHAR  buf[8192];
    va_list       ap;

    va_start( ap, fmt );
    _vsnwprintf( buf, sizeof(buf)/2-1, fmt, ap );
    buf[sizeof(buf)/2-1] = L'\0';

    MessageBox( 0, buf, title, 0 );
    va_end( ap );
}

void D(LPCWSTR fmt, ... )
{
    static WCHAR  buf[8192];
    va_list       ap;

    va_start( ap, fmt );
    _vsnwprintf( buf, sizeof(buf)/2-1, fmt, ap );
    buf[sizeof(buf)/2-1] = L'\0';

    MessageBox( 0, buf, L"D", 0 );
    va_end( ap );
}

#ifdef NDEBUG
#define LOG
#else
static bool _appendfile(HANDLE hFileOut, const void *pData, DWORD dwFileSize)
{
  if(hFileOut!=INVALID_HANDLE_VALUE)
  {
    SetFilePointer(hFileOut,0,0,FILE_END);
    if( WriteFile(hFileOut,pData,dwFileSize,&dwFileSize,NULL) )
    {
      CloseHandle(hFileOut);
      return true;
    }
    CloseHandle(hFileOut);
  }
  return false;
}

bool appendfileW(const wchar_t* pcwFileName,const void *pData,DWORD dwFileSize)
{
  return _appendfile(CreateFileW(pcwFileName,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL), pData, dwFileSize);
}

void LOG(LPCWSTR fmt, ... )
{
    static WCHAR  buf[8192];
    va_list       ap;

    va_start( ap, fmt );
#ifdef MEMCHECK
    swprintf( buf, L"%I64d/%I64d ", alloc_blocks-free_blocks, alloc_bytes-free_bytes );
    //swprintf( buf, L"%I64d/%I64d %I64d/%I64d ", alloc_blocks, alloc_bytes, free_blocks, free_bytes );
    size_t len = wcslen(buf);
    _vsnwprintf( buf+len, sizeof(buf)/2-len-1, fmt, ap );
#else
    _vsnwprintf( buf, sizeof(buf)/2-1, fmt, ap );
#endif
    buf[sizeof(buf)/2-2] = L'\0';
    buf[sizeof(buf)/2-1] = L'\0';
    len = wcslen(buf);
    buf[len] = L'\n';
    buf[len+1] = L'\0';

    appendfileW(L"\\1.log", buf, (len+1)*2);
    va_end( ap );
}
#endif


void D(HWND hwnd, LPCWSTR fmt, ... )
{
    static WCHAR  buf[8192];
    va_list       ap;

    va_start( ap, fmt );
    _vsnwprintf( buf, sizeof(buf)/2-1, fmt, ap );
    buf[sizeof(buf)/2-1] = L'\0';

    SetWindowText( hwnd, buf);
    va_end( ap );
}

#endif //_MEMCHECK_H
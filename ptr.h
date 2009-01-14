template<class T>
class CPtr
{
    T*      m_p;
            CPtr(const CPtr&);
    CPtr&   operator=(const CPtr&);
public:
            CPtr(T* p=0) : m_p(p)       { }
            ~CPtr()                     { delete m_p; }
    T*      operator =  (T* p)          { delete m_p; return m_p=p; }

    T*      operator -> ()      const   { return m_p; }
    T&      operator *  ()      const   { return m_p; }
    T&      operator[](int idx) const   { return m_p[idx]; }

    T*               ptr()      const   { return m_p; }
            operator bool()     const   { return 0!=m_p; }
    bool    operator !()        const   { return 0==m_p; }
};

template<class T>
class CGdiObj
{
    T       m_obj;
            CGdiObj(const CGdiObj&);
    CGdiObj&operator=(const CGdiObj&);
public:
            CGdiObj(T obj=0) : m_obj(obj) {}
            ~CGdiObj()                  { if(m_obj)DeleteObject(m_obj); }
    T       operator =(T obj)           { if(m_obj)DeleteObject(m_obj); return m_obj=obj; }
            operator T()        const   { return m_obj; }
            operator bool()     const   { return 0!=m_obj; }
    bool    operator !()        const   { return 0==m_obj; }
};


inline int RectWidth(const RECT& r)
{
    return r.right - r.left;
}

inline int RectHeight(const RECT& r)
{
    return r.bottom - r.top;
}

inline bool InRect(const RECT& r, int left, int top, int right, int bottom)
{
    return  (r.left < right || left < r.right) &&
            (r.top < bottom || top < r.bottom);
}

inline bool InRect(const RECT& r, const RECT& r2)
{
    return  (r.left < r2.right || r2.left < r.right) &&
            (r.top < r2.bottom || r2.top < r.bottom);
}

class CPaintDC
{
protected:
    PAINTSTRUCT     m_ps;
    const HWND      m_hwnd;
public:
    CPaintDC(HWND hwnd) : m_hwnd(hwnd)
    {
        BeginPaint(m_hwnd, &m_ps );
    }
    ~CPaintDC()
    {
        EndPaint(m_hwnd, &m_ps );
    }
    operator HDC() const { return m_ps.hdc; }
    const RECT& rect() const { return m_ps.rcPaint; }
};

// double buffered dc
class CDblBufPaintDC : public CPaintDC
{
protected:
    HBITMAP     m_hBitmapPaint;
    HBITMAP     m_hBitmapPaintOld;
    HDC         m_hdc;
public:
    CDblBufPaintDC(HWND hwnd) : CPaintDC(hwnd)
    {
        m_hdc = CreateCompatibleDC (m_ps.hdc);
        assert(m_hdc);
        m_hBitmapPaint = CreateCompatibleBitmap(m_ps.hdc, RectWidth(m_ps.rcPaint), RectHeight(m_ps.rcPaint));
        assert(m_hBitmapPaint);
        m_hBitmapPaintOld = (HBITMAP)SelectObject(m_hdc, m_hBitmapPaint);
        assert(m_hBitmapPaintOld);
        SetViewportOrgEx(m_hdc, -m_ps.rcPaint.left, -m_ps.rcPaint.top, 0);
    }
    ~CDblBufPaintDC()
    {
        BitBlt( m_ps.hdc,
                m_ps.rcPaint.left,
                m_ps.rcPaint.top,
                RectWidth(m_ps.rcPaint),
                RectHeight(m_ps.rcPaint),
                m_hdc,
                m_ps.rcPaint.left,
                m_ps.rcPaint.top,
                SRCCOPY);

        SelectObject(m_hdc, m_hBitmapPaintOld);
        DeleteObject (m_hBitmapPaint);
        DeleteDC(m_hdc);
    }
    operator HDC() const { return m_hdc; }
};


class CComPtr
{
};

class CClassFactoryBase : public IClassFactory
{
            LONG            m_cRef;
public:
    CClassFactoryBase() :
       m_cRef(1)
    {
#ifdef G_OBJECTCOUNT
        G_OBJECTCOUNT++;
#else
#warning
#endif
    }

    virtual ~CClassFactoryBase()
    {
#ifdef G_OBJECTCOUNT
        G_OBJECTCOUNT--;
#else
#warning
#endif
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

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)
    {
        if(IsEqualIID(riid, IID_IUnknown) ||
           IsEqualIID(riid, IID_IClassFactory))
        {
            *ppvObject = (IClassFactory*)this;
            AddRef();
            return S_OK;
        }
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

//  STDMETHOD(CreateInstance)(IUnknown*, REFIID, void**)
//  {
//      return E_NOTIMPL;
//  }

    STDMETHOD(LockServer)(BOOL bLock)
    {
        if(bLock)
            G_OBJECTCOUNT++;
        else
            G_OBJECTCOUNT--;
        return S_OK;
    }
};

template<typename T>
class CClassFactory : public CClassFactoryBase
{
public:
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void **ppvObject)
    {
        if(pUnkOuter)
            return CLASS_E_NOAGGREGATION;

        T* p = new T;
        if(!p)
            return E_OUTOFMEMORY;
        HRESULT hr = p->QueryInterface(riid, ppvObject);
        if( hr!=S_OK )
            delete p;
        return hr;
    }
};

#ifndef __REGKEY_H__
#define __REGKEY_H__

//#ifdef _WIN32_WCE
//#include <shlwapi.h>
//#endif

class CRegKey
{
protected:
    bool Open(HKEY hKeyParent, LPCWSTR pcwKey, DWORD dwMode=KEY_READ)
    {
        HKEY hkeyOld = m_hKey;
        LONG lResult;
        if(dwMode==KEY_READ)
        {
            lResult = RegOpenKeyEx(hKeyParent, pcwKey, 0, dwMode, &m_hKey);
        } else
        {
            DWORD dwDisposition;
            //lResult = RegCreateKeyEx(hKeyParent, pcwKey, 0, 0, REG_OPTION_NON_VOLATILE, dwMode, 0, &m_hKey, &dwDisposition);
            lResult = RegCreateKeyEx(hKeyParent, pcwKey, 0, L"REG_SZ", 0, dwMode, 0, &m_hKey, &dwDisposition);
        }
        if(hkeyOld) RegCloseKey(hkeyOld);
        return ERROR_SUCCESS==lResult;
    }
public:
    HKEY m_hKey;
//  WCHAR m_swValue[0x400];
//  BYTE  m_data[0x400];
    DWORD m_bcValue;
    DWORD m_bcData;
    DWORD m_dwType;

    CRegKey(HKEY hKey=0)
    {
        m_hKey = hKey;
//      m_swValue[0] = 0;
//      m_data[0] = 0;
        m_bcValue = 0;
        m_bcData = 0;
        m_dwType = 0;
    }
    ~CRegKey()
    {
        if(m_hKey) RegCloseKey(m_hKey);
    }
    bool Open(HKEY hKeyParent, DWORD dwMode/*=KEY_READ*/, LPCWSTR pcwFmt, ...)
    {
        va_list va;
        WCHAR swKey[1024];

        va_start(va, pcwFmt);
        _vsnwprintf(swKey, sizeof(swKey)/2-1, pcwFmt, va);
        swKey[sizeof(swKey)/2-1] = '\0';
        va_end(va);
        return Open(hKeyParent, swKey, dwMode);
    }
//  bool Enum(DWORD dwIndex)
//  {
//      m_bcValue = sizeof(m_swValue) - 1;
//      m_bcData = sizeof(m_data) - 1;
//      m_swValue[0] = 0;
//      m_data[0] = 0;
//      LONG res = RegEnumValue( m_hKey, dwIndex, m_swValue, &m_bcValue, 0, &m_dwType, m_data, &m_bcData );
//      m_swValue[m_bcValue] = 0;
//      m_data[m_bcData] = 0;
//      return ERROR_SUCCESS==res;
//  }
//  bool EnumKey(DWORD dwIndex)
//  {
//      m_bcValue = sizeof(m_swValue) - 1;
//      m_swValue[0] = 0;
//      LONG res = RegEnumKeyEx( m_hKey, dwIndex, m_swValue, &m_bcValue, 0, 0, 0, 0 );
//      m_swValue[m_bcValue] = 0;
//      return ERROR_SUCCESS==res;
//  }
//  bool QueryValue(const char* pchValue)
//  {
//      m_bcData = sizeof(m_data) - 1;
//      m_data[0] = 0;
//      LONG res = RegQueryValueEx( m_hKey, pchValue, 0, &m_dwType, m_data, &m_bcData );
//      m_data[m_bcData] = 0;
//      return ERROR_SUCCESS==res;
//  }
    bool SetValue(LPCWSTR pcwValue, DWORD dwType, const void* pcData, DWORD dwLen)
    {
        return ERROR_SUCCESS==RegSetValueEx(m_hKey, pcwValue, 0, dwType, (const BYTE*)pcData, dwLen );
    }
    bool SetString(LPCWSTR pcwValue, DWORD dwType, LPCWSTR pcwFmt, ...)
    {
      va_list va;
      WCHAR swData[1024];

      va_start(va, pcwFmt);
      _vsnwprintf(swData, sizeof(swData)/2-1, pcwFmt, va);
      swData[sizeof(swData)/2-1] = '\0';
      va_end(va);
      return SetValue(pcwValue, dwType, swData, 2*(1+wcslen(swData)));
    }
    bool DeleteValue(LPCWSTR pcwValue)
    {
        return ERROR_SUCCESS==RegDeleteValue(m_hKey, pcwValue);
    }
//  bool DeleteKey(LPCWSTR pcwKey)
//  {
//      return ERROR_SUCCESS==SHDeleteKey(m_hKey, pcwKey);
//  }
};

#endif // __REGKEY_H__

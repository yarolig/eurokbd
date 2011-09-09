class CConfig
{
    UINT64      m_mtimeOK;          // mtime конфиг-файла прочитанного в последний раз
                                    // 0 - еще ничего не читали
                                    // 1 - загружены значения по умолчанию
    UINT64      m_mtimeError;       // mtime конфиг-файла при чтении которого была ошибка
protected:
    LPWSTR      m_filename;
public:
    CConfig(LPCWSTR filename)
    {
        m_filename = _wcsdup(filename);
        m_mtimeOK = 0;
        m_mtimeError = 0;
    }

    ~CConfig()
    {
        free(m_filename);
    }

    enum {  E_OK=1,
            E_DEFAULTS_LOADED=2,
            E_UNCHANGED=3 };
    int Ensure()
    {
        int rc = E_UNCHANGED; // true, если что-то изменилось

        WIN32_FIND_DATA ffdata;
        HANDLE hFind = FindFirstFile(m_filename, &ffdata);
        if( hFind==INVALID_HANDLE_VALUE ) // нет такого файла :(
        {
            _SetDefaults();
            m_mtimeOK = 1; // типа magic что загружены установки по умолчанию
            rc = E_DEFAULTS_LOADED;
        } else
        {
            FindClose(hFind);

            if( *(UINT64*)&ffdata.ftLastWriteTime != m_mtimeOK &&
                *(UINT64*)&ffdata.ftLastWriteTime != m_mtimeError)
            {
                // за время между FindFirstFile и Load файл может измениться, но это не страшно
                // будет за-Load'ена более новая версия, а время в m_LastWriteTime будет от старой
                // она просто перечитается еще раз, учитывая низкую вероятность этого события, это нормально
                // ????: хотя это можно исключить, открывая файл тут и беря время из GetFileTime, но это может усложнить портирование
                //
                // Load может вернуть false, например если сейчас происходит запись в конфиг другим процессом
                // в этом случае остаются старые данные
                //
                if( _Load() )
                {
                    m_mtimeOK = *(UINT64*)&ffdata.ftLastWriteTime;
                    rc = E_OK;
                } else
                {
                    m_mtimeError = *(UINT64*)&ffdata.ftLastWriteTime;
                    if (0==m_mtimeOK) // если не было старых данных
                    {
                        _SetDefaults();
                        m_mtimeOK = 1; // типа magic что загружены установки по умолчанию, чтобы в след. раз не делать SetDefaults
                        rc = E_DEFAULTS_LOADED;
                    }
                }
            }
        }
        return rc;
    }

    virtual bool Save()
    {
        if( _Save() )
        {
            WIN32_FIND_DATA ffdata;
            HANDLE hFind = FindFirstFile(m_filename, &ffdata);
            if( hFind!=INVALID_HANDLE_VALUE )
            {
                m_mtimeOK = *(UINT64*)&ffdata.ftLastWriteTime;
                return true;
            }
        }
        return false;
    }

protected:
    virtual bool _Save() = 0;
    virtual void _SetDefaults() = 0;
    virtual bool _Load() = 0;
};

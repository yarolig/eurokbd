
// просто список, без всяких аллокаторов :)
template <class T>
class listentry
{
public:
    T* next;
};

template <class T>
class list
{
public:
    T* first;

    list()
    {
        first = 0;
    }

    inline void destroy()
    {
        T* p= first;
        while(p)
        {
            T* pnext = p->next;
            delete p;
            p = pnext;
        }
        first = 0;
    }

    inline size_t size() const
    {
        size_t i = 0;
        for(const T* q=first; q; q=q->next)
        {
            i ++;
        }
        return i;
    }

    inline bool inlist(const T* p) const
    {
        for(const T* q=first; q; q=q->next)
        {
            if(q==p) return true;
        }
        return false;
    }

    inline void add_first(T* p)
    {
        p->next = first;
        first = p;
    }

    inline void add_after(T* p, T* ref)
    {
        p->next = ref->next;
        ref->next = p;
    }

};

template <class T>
class deqentry
{
public:
    T *prev, *next;
};

template <class T>
class deq
{
public:
    T *first, *last;

    deq()
    {
        first = last = 0;
    }

    inline void destroy()
    {
        T* p= first;
        while(p)
        {
            T* pnext = p->next;
            delete p;
            p = pnext;
        }
        first = last = 0;
    }

    inline size_t size() const
    {
        size_t i = 0;
        for(const T* q=first; q; q=q->next)
        {
            i ++;
        }
        return i;
    }

    inline bool inlist(const T* p) const
    {
        for(const T* q=first; q; q=q->next)
        {
            if(q==p) return true;
        }
        return false;
    }

    inline void add_first(T* p)
    {
        p->next = first;
        p->prev = 0;
        (first ? first->prev : last) = p;
        first = p;
    }

    inline void add_last(T* p)
    {
        p->next = 0;
        p->prev = last;
        last->next = p;
        last = p;
    }

    inline void add_before(T* p, T* ref)
    {
        p->next = ref;
        p->prev = ref->prev;
        (ref->prev ? ref->prev->next : first) = p;
        ref->prev = p;
    }

    inline void add_after(T* p, T* ref)
    {
        p->prev = ref;
        p->next = ref->next;
        (ref->next ? ref->next->prev : last) = p;
        ref->next = p;
    }

    inline void remove(T* p)
    {
        (p->prev ? p->prev->next : first) = p->next;
        (p->next ? p->next->prev : last) = p->prev;
    }
};
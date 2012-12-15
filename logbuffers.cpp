#include <cstring>
#include <iostream>
#include <vector>
#include <type_traits>
#include <tuple>

using namespace std;

template <size_t I, size_t N>
struct TuplePrinter
{
    template <typename... T>
    static void Print(const tuple<T...>& x)
    {
        cout << get<I>(x);
        return TuplePrinter<I+1, N>::Print(x);
    }
};

template <size_t N>
struct TuplePrinter<N, N>
{
    template <typename... T>
    static void Print(const tuple<T...>& x)
    {
        cout << endl;
    }
};

template <typename... T>
void PrintTuple(const tuple<T...>& x)
{
    return TuplePrinter<0, sizeof...(T)>::Print(x);
}

template <typename TTuple>
size_t Print(void* input)
{
    TTuple* p = reinterpret_cast<TTuple*>(input);
    PrintTuple(*p);
    return sizeof *p;
}

struct BinaryTracer
{
    static std::vector<char> storage;
    
    static void Write(const char* buffer, size_t sz)
    {
        storage.insert(storage.end(), buffer, buffer + sz);
    }
};

std::vector<char> BinaryTracer::storage;

template <typename... T>
void Trace(const T&... x)
{
    auto vals = make_tuple(x...);
    size_t (*fn)(void*) = &Print<decltype(vals)>;
    char buffer[sizeof fn + sizeof vals];
    std::memcpy(buffer, &fn, sizeof fn);
    std::memcpy(buffer + sizeof fn, &vals, sizeof vals);
    BinaryTracer::Write(buffer, sizeof buffer);
}

void LoadBuffer(char* buffer, size_t sz)
{
    for (size_t pos = 0; pos < sz; )
    {
        size_t (*fn)(void*);
        std::memcpy(&fn, buffer + pos, sizeof fn);
        pos += sizeof fn;
        pos += fn(buffer + pos);
    }
}

template <typename... T>
struct TraceEntryContainer;

typedef const char* TraceArea;

template <>
struct TraceEntryContainer<>
{
    explicit TraceEntryContainer(TraceArea area) :
            mArea(area)
    { }
    
    template <typename... T>
    void ExecTrace(const T&... x)
    {
        Trace(mArea, " -- ", x...);
    }
    
    TraceArea mArea;
};

template <typename T, typename... TSrc>
struct TraceEntryContainer<T, TSrc...>
{
    TraceEntryContainer(TraceEntryContainer<TSrc...>& src,
                        const T& val
                     ) :
            mSrc(src),
            mVal(val)
    { }
    
    template <typename... TArgs>
    void ExecTrace(const TArgs&... x)
    {
        mSrc.ExecTrace(mVal, x...);
    }
    
    TraceEntryContainer<TSrc...>& mSrc;
    const T&                      mVal;
};

TraceEntryContainer<> TraceFor(TraceArea area)
{
    return TraceEntryContainer<>(area);
}

struct TraceEntryPusher
{
	template <typename... T>
	void operator,(TraceEntryContainer<T...>&& builder) const
	{
		builder.ExecTrace();
	}
};

#define STRING(a) #a
#define STRINGVAL(a) STRING(a)
#define LVAL(a) (" " #a "=") << (a)

#define TRACE_PREFIX (__FILE__ ":" STRINGVAL(__LINE__) "]] ")
#define TRACE(area)                                            \
    TraceEntryPusher(),                                        \
        TraceFor(#area) << TRACE_PREFIX

template <typename T, typename... TSrc>
TraceEntryContainer<T, TSrc...>
operator <<(TraceEntryContainer<TSrc...>&& src, const T& x)
{
    return TraceEntryContainer<T, TSrc...>(src, x);
}

int main()
{
    TRACE(Area1) << "Hello, " << "world!";
    TRACE(Area2) << "I like to write " << 5 << " numbers: "
                 << 1 << 2 << 3 << 4 << 5;
    int x = 4;
    float f = 3.1415;
    const char blah[] = "Yo!";
    TRACE(Area1) << LVAL(x) << LVAL(f) << LVAL(blah);
    TRACE(Really just a char*)
                 << "but not so in the real implementation"
                 << "...sound good?";
    
    LoadBuffer(BinaryTracer::storage.data(),
               BinaryTracer::storage.size()
              );
}


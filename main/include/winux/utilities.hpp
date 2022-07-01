#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

// 各平台条件编译宏检测
#include "system_detection.inl"

#if _MSC_VER > 0 && _MSC_VER < 1201
#pragma warning( disable: 4786 )
#endif
#if _MSC_VER > 0
#pragma warning( disable : 4996 )
#endif

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <tuple>
#include <queue>
#include <functional>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined(OS_WIN)
#include <windows.h>
#endif

/** \brief 跨平台基础功能库 */
namespace winux
{
// 一些宏定义 ----------------------------------------------------------------------------------
/*  Dll相关宏定义:
    WINUX_DLL_USE     - 此宏开关表示有DLL参与,包括生成DLL或者导入DLL,不定义此宏和一般的使用源码没区别
    WINUX_DLL_EXPORTS - 此宏开关表示是生成DLL(dllexport)还是导入DLL(dllimport),linux平台下忽略
    WINUX_DLL         - 标记函数、类、变量,用于标明其要从DLL导出还是导入,linux平台下忽略
    WINUX_API         - 标记函数调用约定,Win下Dll参与时为stdcall,否则为空白默认,linux平台下忽略
    WINUX_FUNC_DECL   - 标记函数声明
    WINUX_FUNC_IMPL   - 标记函数实现 */
#ifdef WINUX_DLL_USE
    #if defined(_MSC_VER) || defined(WIN32)
        #pragma warning( disable: 4251 )
        #pragma warning( disable: 4275 )
        #ifdef  WINUX_DLL_EXPORTS
            #define WINUX_DLL  __declspec(dllexport)
        #else
            #define WINUX_DLL  __declspec(dllimport)
        #endif

        #define WINUX_API __stdcall
    #else
        #define WINUX_DLL
        #define WINUX_API
    #endif
#else
    #define WINUX_DLL
    #define WINUX_API
#endif

#define WINUX_FUNC_DECL(ret) WINUX_DLL ret WINUX_API
#define WINUX_FUNC_IMPL(ret) ret WINUX_API

#ifndef countof
    #define countof(arr) ( sizeof(arr) / sizeof(arr[0]) )
#endif

#ifndef TEXT
    #ifdef UNICODE
        #define TEXT(__x) L##__x
    #else
        #define TEXT(__x) __x
    #endif
#endif

// 禁止类的对象赋值/拷贝构造
// DISABLE_OBJECT_COPY
#define DISABLE_OBJECT_COPY( clsname ) private:\
clsname & operator = ( clsname const & ) = delete;\
clsname( clsname const & ) = delete;

// C语言缓冲区转换为AnsiString二进制串
#define CBufferToAnsiString( buf, size ) winux::AnsiString( (char const *)(buf), (size_t)(size) )

// 如果指针非NULL
// IF_PTR
#define IF_PTR(ptr) if ( (ptr) != NULL ) (ptr)
// ASSIGN_PTR
#define ASSIGN_PTR(ptr) if ( (ptr) != NULL ) (*(ptr))

// 不使用一个变量
// UNUSED
#define UNUSED(s)

// 给类添加一个属性，和相关的数据成员，自动添加get/set方法
#define DEFINE_ATTR_MEMBER( ty, name, memname ) \
public:\
    ty const & get##name() const { return this->##memname; }\
    void set##name( ty const & v ) { this->##memname = v; }\
private:\
    ty memname;

// 给类添加一个只读属性，和相关的数据成员，自动添加get方法
#define DEFINE_ATTR_MEMBER_READONLY( ty, name, memname ) \
public:\
    ty const & get##name() const { return this->##memname; }\
private:\
    ty memname;

// 给类添加一个属性，自动添加get/set方法
#define DEFINE_ATTR( ty, name, getcode, setcode ) \
public:\
    ty get##name() const { getcode; }\
    void set##name( ty const & _VAL_ ) { setcode; }

// 给类添加一个只读属性，自动添加get方法
#define DEFINE_ATTR_READONLY( ty, name, getcode ) \
public:\
    ty get##name() const { getcode; }

// 给类定义一个NewInstance静态方法
#define DEFINE_FUNC_NEWINSTANCE( cls, ret, paramtypes, params ) \
inline static ret * NewInstance##paramtypes \
{ \
    return new cls##params;\
}

// 给类定义一个自定义事件相关的成员和函数，默认实现
#define DEFINE_CUSTOM_EVENT(evtname, paramtypes, calledparams) \
public: \
    using evtname##HandlerFunction = std::function< void paramtypes >; \
    void on##evtname##Handler( evtname##HandlerFunction handler ) \
    { \
        this->_##evtname##Handler = handler; \
    } \
protected: \
    evtname##HandlerFunction _##evtname##Handler; \
    virtual void on##evtname paramtypes \
    { \
        if ( this->_##evtname##Handler ) this->_##evtname##Handler calledparams; \
    }

// 给类定义一个自定义事件相关的成员和函数，带返回值并自己实现
#define DEFINE_CUSTOM_EVENT_RETURN_EX(ret, evtname, paramtypes) \
public: \
    using evtname##HandlerFunction = std::function< ret paramtypes >; \
    void on##evtname##Handler( evtname##HandlerFunction handler ) \
    { \
        this->_##evtname##Handler = handler; \
    } \
protected: \
    evtname##HandlerFunction _##evtname##Handler; \
    virtual ret on##evtname paramtypes


// 判断GCC版本大于给定版本
#define GCC_VERSION_GREAT_THAN(Major,Minor,Patchlevel) \
( __GNUC__ > Major || ( __GNUC__ == Major && ( __GNUC_MINOR__ > Minor || ( __GNUC_MINOR__ == Minor && __GNUC_PATCHLEVEL__ > Patchlevel ) ) ) )

// 基本类型 -----------------------------------------------------------------------------------
typedef int int32;
typedef unsigned int uint, uint32;
typedef unsigned long ulong;
typedef short int16;
typedef unsigned short ushort, uint16;
typedef char int8;
typedef unsigned char uint8;

typedef char16_t char16;
typedef char32_t char32;

#if defined(CL_VC)
typedef wchar_t wchar;
typedef unsigned __int64 uint64;
typedef unsigned __int64 ulonglong;
typedef __int64 int64;
typedef __int64 longlong;
#else
typedef wchar_t wchar;
typedef unsigned long long uint64;
typedef unsigned long long ulonglong;
typedef long long int64;
typedef long long longlong;
#endif

//#ifdef UNICODE
//typedef wchar tchar;
//#else
typedef char tchar;
//#endif

#ifndef byte
typedef unsigned char byte;
#endif

class Mixed;
// STL wrappers
template < typename _ChTy >
using XString = std::basic_string<_ChTy>;

typedef XString<char> AnsiString, Utf8String;
typedef XString<wchar> UnicodeString;
typedef XString<char16> UnicodeString16, Utf16String;
typedef XString<char32> UnicodeString32, Utf32String;
typedef XString<tchar> String;

template < typename _ChTy >
using XStringArray = std::vector< std::basic_string<_ChTy> >;

typedef XStringArray<char> AnsiStringArray, Utf8StringArray;
typedef XStringArray<wchar> UnicodeStringArray;
typedef XStringArray<char16> UnicodeString16Array;
typedef XStringArray<char32> UnicodeString32Array;
typedef XStringArray<char16> Utf16StringArray;
typedef XStringArray<char32> Utf32StringArray;
typedef XStringArray<tchar> StringArray;

typedef std::map<String, String> StringStringMap;
typedef std::pair<String, String> StringStringPair;

typedef std::vector<Mixed> MixedArray;
typedef std::map<String, Mixed> StringMixedMap;
typedef std::pair<String, Mixed> StringMixedPair;
//typedef std::map<Mixed, Mixed> MixedMixedMap;
//typedef std::pair<Mixed, Mixed> MixedMixedPair;

// 模板元编程支持 ---------------------------------------------------------------------------

/** \brief 检测map中是否有该键的值 */
template < typename _MAP, typename _KEY >
inline bool isset( _MAP const & m, _KEY const & k )
{
    return m.find(k) != m.end();
}

/** \brief 将C数组转换成vector */
template < typename _Ty >
std::vector<_Ty> ToArray( _Ty * arr, uint count )
{
    return std::vector<_Ty>( arr, arr + count );
}

template < typename _Ty, uint _N >
std::vector<_Ty> ToArray( _Ty (&arr)[_N] )
{
    return std::vector<_Ty>( arr, arr + _N );
}

/** \brief 调用一个返回void的函数或函数对象,返回一个数字
 *
 *  通常是为了在初始化语句中方便调用返回void的函数 */
template < typename _Fx, typename... _ArgType >
int VoidReturnInt( _Fx fn, _ArgType&& ...arg )
{
    fn( std::forward<_ArgType>(arg)... );
    return 1;
}

/** \brief 二进制数,编译时计算, 0开头(基于8进制) */
template < uint64 n > struct Bin0
{
    enum : uint64 { val = ( Bin0< n / 8 >::val << 1 ) + n % 8 };
};
template <> struct Bin0<0>
{
    enum : uint64 { val = 0 };
};

// 二进制数 macro包装
#define BinVal(x) winux::Bin0<0##x>::val

// 引用参数包装
template < typename _Ty >
class RefParam
{
    _Ty & _r;
public:
    typedef _Ty ParamType;
    typedef _Ty & ParamTypeRef;

    explicit RefParam( ParamTypeRef r ) : _r(r) { }
    operator ParamTypeRef () { return _r; }
};

/** \brief 向模板参数传递引用型参数 */
template < typename _Ty >
RefParam<_Ty> Ref( _Ty & r )
{
    return RefParam<_Ty>(r);
}

/** \brief Tuple参数序列 */
template < size_t... _Index >
struct IndexSequence { };

// 构建一个IndexSequence< 0, 1, 2, ..., _Num - 1 >
template < size_t _Num, typename _IdxSeq = IndexSequence<> >
struct MakeIndexSequence;

template < size_t _Num, size_t... _Index >
struct MakeIndexSequence< _Num, IndexSequence<_Index...> > : MakeIndexSequence< _Num - 1, IndexSequence< _Index..., sizeof...(_Index) > > { };

template < size_t... _Index >
struct MakeIndexSequence< 0, IndexSequence<_Index...> >
{
    using Type = IndexSequence<_Index...>;
};

/** \brief 函数特征 */
#include "func_traits.inl"

/** \brief Runable模板 */
#include "func_runable.inl"

/** \brief Invoker模板 */
#include "func_invoker.inl"

/** \brief 函数包装,用来将不同调用约定的函数统一包装成默认约定 */
template < typename _PfnType, _PfnType pfn >
struct FuncWrapper
{
    template < typename... _ArgType >
    static typename FuncTraits<_PfnType>::ReturnType func( _ArgType&& ...arg )
    {
        return (*pfn)( std::forward<_ArgType>(arg)... );
    }
};

/** \brief MAP赋值器 */
template < typename _KTy, typename _VTy >
class MapAssigner
{
    std::map< _KTy, _VTy > * _m;
public:
    MapAssigner( std::map< _KTy, _VTy > * m ) : _m(m) { }
    MapAssigner & operator()( _KTy const & k, _VTy const & v )
    {
        (*_m)[k] = v;
        //_m->insert( std::pair< _KTy, _VTy >( k, v ) );
        return *this;
    }
    operator std::map< _KTy, _VTy > & () { return *_m; }
};

/** \brief Array赋值器 */
template < typename _Ty >
class ArrayAssigner
{
    std::vector<_Ty> * _a;
public:
    ArrayAssigner( std::vector<_Ty> * a ) : _a(a) { }
    ArrayAssigner & operator () ( _Ty const & v )
    {
        _a->push_back(v);
        return *this;
    }
    operator std::vector<_Ty> & () { return *_a; }
};

/** \brief 给容器赋值 */
template < typename _KTy, typename _VTy >
MapAssigner< _KTy, _VTy > Assign( std::map< _KTy, _VTy > * m )
{
    return MapAssigner< _KTy, _VTy >(m);
}
/** \brief 给容器赋值 */
template < typename _Ty >
ArrayAssigner<_Ty> Assign( std::vector<_Ty> * a )
{
    return ArrayAssigner<_Ty>(a);
}

/** \brief 成员包装
 *
 *  此类的构造函数/析构函数不能直接调用create()/destroy()，因为目标类的实现未知，所以无法创建或销毁。
 *  必须要在使用本包装的类中的构造函数/析构函数中分别调用它们。
 *  operator=和拷贝构造也是类似.因为目标类的实现未知,所以无法依靠自动生成的函数来自动调用.必须重写使用本包装的类中的operator=和拷贝构造函数 */
template < typename _TargetCls >
class MembersWrapper
{
private:
    _TargetCls * _data;

    MembersWrapper( MembersWrapper const & other ) = delete;
public:
    MembersWrapper() : _data(nullptr) { }

    /** \brief 拷贝赋值，必须保证create()已经调用 */
    MembersWrapper & operator = ( MembersWrapper const & other )
    {
        if ( &other != this )
        {
            *_data = *other._data;
        }
        return *this;
    }

#ifndef MOVE_SEMANTICS_DISABLED
    MembersWrapper( MembersWrapper && other )
    {
        _data = other._data;
        other._data = nullptr;
    }
    /** \brief 移动赋值，不用保证create()已经调用 */
    MembersWrapper & operator = ( MembersWrapper && other )
    {
        if ( &other != this )
        {
            this->destroy();
            _data = other._data;
            other._data = nullptr;
        }
        return *this;
    }
#endif

    /** \brief 必须在使用者类的析构函数里最后一个调用 */
    void destroy()
    {
        if ( _data )
        {
            delete (_TargetCls *)_data;
            _data = nullptr;
        }
    }

    /** \brief 必须在使用者类的构造函数里第一个调用 */
    template < typename... _ArgType >
    void create( _ArgType&&... arg )
    {
        this->destroy();
        _data = new _TargetCls( std::forward<_ArgType>(arg)... );
    }

    _TargetCls * operator -> ()
    {
        return _data;
    }
    _TargetCls const * operator -> () const
    {
        return _data;
    }
    operator _TargetCls & ()
    {
        return *_data;
    }
    operator _TargetCls const & () const
    {
        return *_data;
    }
    operator bool() const
    {
        return _data != nullptr;
    }
};

// ----------------------------------------------------------------------------------------
/** \brief 错误类 */
class WINUX_DLL Error : public std::exception
{
private:
    int _errType;
    AnsiString _errStr;
public:
    Error() throw() : _errType(0) { }
    Error( int errType, AnsiString const & errStr ) throw() : _errType(errType), _errStr(errStr) { }
    virtual ~Error() throw() { }
    virtual int getErrType() const throw() { return _errType; }
    virtual char const * what() const throw() { return _errStr.c_str(); }
};

/** \brief 判断一个字符串值是否在一个字符串数组里,默认大小写敏感 */
WINUX_FUNC_DECL(bool) ValueIsInArray( StringArray const & arr, String const & val, bool caseInsensitive = false );

/** \brief 随机数,随机产生n1~n2的数字. 包括n1,n2本身 */
WINUX_FUNC_DECL(int) Random( int n1, int n2 );

// -------------------------------------------------------------------------------
class GrowBuffer;

/** \brief 缓冲区,表示内存中一块2进制数据(利用malloc/realloc进行内存分配) */
class WINUX_DLL Buffer
{
public:
    /** \brief 默认构造函数 */
    Buffer();
    /** \brief 构造函数1 从一个缓冲区创建Buffer,可以指定是否为窥视模式
     *
     *  处于窥视模式时将不负责管理资源的释放 */
    Buffer( void * buf, uint size, bool isPeek = false );
    /** \brief 构造函数2 从一个AnsiString创建Buffer,可以指定是否为窥视模式
     *
     *  处于窥视模式时将不负责管理资源的释放 */
    Buffer( AnsiString const & data, bool isPeek = false );
    virtual ~Buffer();
    Buffer( Buffer const & other );
    Buffer & operator = ( Buffer const & other );

#ifndef MOVE_SEMANTICS_DISABLED
    /** \brief 移动构造函数 */
    Buffer( Buffer && other );
    /** \brief 移动赋值操作 */
    Buffer & operator = ( Buffer && other );
    /** \brief 移动构造函数1 */
    Buffer( GrowBuffer && other );
    /** \brief 移动赋值操作1 */
    Buffer & operator = ( GrowBuffer && other );
#endif

    /** \brief 设置缓冲区,当isPeek为false时拷贝数据缓冲区 */
    void setBuf( void * buf, uint size, uint capacity, bool isPeek );

    /** \brief 设置缓冲区,当isPeek为false时拷贝数据缓冲区 */
    void setBuf( void * buf, uint size, bool isPeek ) { this->setBuf( buf, size, size, isPeek ); }

    /** \brief 分配容量大小,当setDataSize为true时设置数据长度 */
    void alloc( uint capacity, bool setDataSize = true );

    /** \brief 重新调整容量的大小,保留数据内容
     *
     *  如果新的容量小于数据大小,多余的数据被丢弃 */
    void realloc( uint newCapacity );

    /** \brief 把窥探模式变为拷贝模式，如果copyCapacity为true时连容量也一起拷贝，否则只拷贝数据。 */
    bool peekCopy( bool copyCapacity = false );

    /** \brief 释放缓冲区 */
    void free();

    /** \brief 暴露缓冲区指针 */
    void * getBuf() const { return _buf; }

    template < typename _Ty >
    _Ty * getBuf() const { return reinterpret_cast<_Ty *>(_buf); }

    winux::byte & operator [] ( int i ) { return reinterpret_cast<winux::byte *>(_buf)[i]; }
    winux::byte const & operator [] ( int i ) const { return reinterpret_cast<winux::byte const *>(_buf)[i]; }

    /** \brief 获取数据大小 */
    uint getSize() const { return _dataSize; }

    /** \brief 设置数据大小，不能超过容量大小（不建议外部调用） */
    void _setSize( uint dataSize ) { _dataSize = ( dataSize > _capacity ? _capacity : dataSize ); }

    /** \brief 获取容量大小 */
    uint getCapacity() const { return _capacity; }

    /** \brief 判断是否为一个有效的Buffer */
    operator bool() const { return _buf != NULL; }

    /** \brief 转换到字符串 */
    template < typename _ChTy >
    std::basic_string<_ChTy> toString() const
    {
        typedef typename std::basic_string<_ChTy>::value_type CharType;
        return std::basic_string<_ChTy>( (CharType*)_buf, _dataSize / sizeof(CharType) );
    }
    /** \brief 转换到AnsiString */
    AnsiString toAnsi() const { return this->toString<AnsiString::value_type>(); }
    /** \brief 转换到UnicodeString */
    UnicodeString toUnicode() const { return this->toString<UnicodeString::value_type>(); }

protected:
    static void * _Alloc( uint size );
    static void * _Realloc( void * p, uint newSize );
    static void _Free( void * p );

    void * _buf;
    uint _dataSize; // 数据的大小
    uint _capacity; // 容量
    bool _isPeek; // 是否为窥视模式

    friend class GrowBuffer;
};

/** \brief 高效的可增长缓冲区，1.33倍冗余量 */
class WINUX_DLL GrowBuffer : public Buffer
{
public:
    /** \brief 构造函数，初始化缓冲区的大小 */
    explicit GrowBuffer( uint capacity = 0 );
    GrowBuffer( GrowBuffer const & other );
    GrowBuffer & operator = ( GrowBuffer const & other );
    explicit GrowBuffer( Buffer const & other );
    GrowBuffer & operator = ( Buffer const & other );

#ifndef MOVE_SEMANTICS_DISABLED
    /** \brief 移动构造函数 */
    GrowBuffer( GrowBuffer && other );
    /** \brief 移动赋值操作 */
    GrowBuffer & operator = ( GrowBuffer && other );
    /** \brief 移动构造函数1 */
    GrowBuffer( Buffer && other );
    /** \brief 移动赋值操作1 */
    GrowBuffer & operator = ( Buffer && other );
#endif

    /** \brief 添加数据 */
    void append( void const * data, uint size );

    /** \brief 添加数据 */
    void append( AnsiString const & data ) { this->append( data.c_str(), (uint)data.size() ); }

    /** \brief 添加数据 */
    void append( Buffer const & data ) { this->append( data.getBuf(), data.getSize() ); }

    /** \brief 擦除数据，自动紧缩 */
    void erase( uint start, uint count = (uint)-1 );

protected:
    friend class Buffer;
};

// 混合体相关 ------------------------------------------------------------------------------
/** \brief 混合体错误 */
class WINUX_DLL MixedError : public Error
{
public:
    enum
    {
        meNoError,         ///< 没有错误
        meIsNull,          ///< 值是Null因此无法操作
        meCantConverted,   ///< 不能转换到某种类型
        meUnexpectedType,  ///< 意料外的类型,该类型不能执行这个操作
        meOutOfArrayRange, ///< 数组越界
    };

    MixedError( int errType, AnsiString const & errStr ) throw() : Error( errType, errStr ) { }
};

/** \brief 混合体,能表示多种类型的值 */
class WINUX_DLL Mixed
{
public:
    enum MixedType : ushort
    {
    #define MixedType_ENUM_ITEM(item) item,
    #define MixedType_ENUM_ITEMSTRING(item) #item,
    #define MixedType_ENUM_ITEMLIST(_)\
        _(MT_NULL)\
        _(MT_BOOLEAN)\
        _(MT_BYTE)\
        _(MT_SHORT) _(MT_USHORT)\
        _(MT_INT) _(MT_UINT)\
        _(MT_LONG) _(MT_ULONG)\
        _(MT_INT64) _(MT_UINT64)\
        _(MT_FLOAT) _(MT_DOUBLE)\
        _(MT_ANSI) _(MT_UNICODE)\
        _(MT_ARRAY)/**< 数组类型,利用STL vector&lt;Mixed&gt;存储的Mixed数组*/ \
        _(MT_COLLECTION)/**< Collection类型,利用数组存储key体现次序,然后用Map存储k/v对 */ \
        _(MT_BINARY)/**< 二进制数据类型,利用Buffer类对象存储的二进制数据 */

    MixedType_ENUM_ITEMLIST(MixedType_ENUM_ITEM)
    };

    /** \brief 输出指定类型的字符串表示 */
    static String const & TypeString( MixedType type );

    class WINUX_DLL MixedLess
    {
    public:
        bool operator () ( Mixed const & v1, Mixed const & v2 ) const;
    };

    typedef std::map< Mixed, Mixed, MixedLess > MixedMixedMap;
    typedef MixedMixedMap::value_type  MixedMixedPair;

    MixedType _type;

    union
    {
        double _dblVal;
        uint64 _ui64Val;
        int64 _i64Val;
        float _fltVal;
        ulong _ulVal;
        long _lVal;
        uint _uiVal;
        int _iVal;
        ushort _ushVal;
        short _shVal;
        byte _btVal;
        bool _boolVal;
        struct // 字符串
        {
            union
            {
                AnsiString * _pStr;
                UnicodeString * _pWStr;
            };
        };
        struct // Array or Collection
        {
            MixedArray * _pArr;
            MixedMixedMap * _pMap;
        };
        struct // 二进制数据
        {
            Buffer * _pBuf;
        };
    };

public:
    Mixed();
    Mixed( AnsiString const & str ); ///< 多字节字符串
    Mixed( UnicodeString const & str ); ///< Unicode字符串
    Mixed( char const * str, int len = -1 ); ///< 多字节字符串
    Mixed( wchar const * str, int len = -1 ); ///< Unicode字符串

    Mixed( bool boolVal );
    Mixed( byte btVal );
    Mixed( short shVal );
    Mixed( ushort ushVal );
    Mixed( int iVal );
    Mixed( uint uiVal );
    Mixed( long lVal );
    Mixed( ulong ulVal );
    Mixed( float fltVal );
    Mixed( int64 i64Val );
    Mixed( uint64 ui64Val );
    Mixed( double dblVal );

    Mixed( Buffer const & buf );
    Mixed( void * binaryData, uint size, bool isPeek = false );

    // Array构造函数 -----------------------------------------------------------------------
    /** \brief 数组构造函数 */
    Mixed( Mixed * arr, uint count );

    template < typename _Ty >
    Mixed( std::vector<_Ty> const & arr )
    {
        _zeroInit();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray( arr.size() );
        uint i;
        for ( i = 0; i < arr.size(); ++i )
        {
            this->_pArr->at(i) = arr[i];
        }
    }

#if defined(__GNUC__) || _MSC_VER > 1200
    template < typename _Ty, uint n >
    Mixed( _Ty (&arr)[n] )
    {
        _zeroInit();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray(n);
        uint i;
        for ( i = 0; i < n; ++i )
        {
            this->_pArr->at(i) = arr[i];
        }
    }
#endif
    // Collection构造函数 ------------------------------------------------------------------
    /** \brief Collection构造函数 */
    template < typename _KTy, typename _VTy, typename _Pr, typename _Alloc >
    Mixed( std::map< _KTy, _VTy, _Pr, _Alloc > const & m )
    {
        _zeroInit();
        this->assign< _KTy, _VTy, _Pr, _Alloc >(m);
    }

#if defined(__GNUC__) || _MSC_VER > 1200
    template < typename _KTy, typename _VTy, uint count >
    Mixed( std::pair< _KTy, _VTy > (&pairs)[count] )
    {
        _zeroInit();
        this->assign< _KTy, _VTy, count >(pairs);
    }

    template < typename _KTy, typename _VTy, uint count >
    Mixed( _KTy (&keys)[count], _VTy (&vals)[count] )
    {
        _zeroInit();
        this->assign< _KTy, _VTy, count >( keys, vals );
    }
#endif

    /** \brief 析构函数 */
    ~Mixed();

    /** \brief 拷贝构造函数 */
    Mixed( Mixed const & other );
    /** \brief 拷贝赋值操作 */
    Mixed & operator = ( Mixed const & other );

#ifndef MOVE_SEMANTICS_DISABLED
    /** \brief 移动构造函数 */
    Mixed( Mixed && other );
    /** \brief 移动赋值操作 */
    Mixed & operator = ( Mixed && other );

    Mixed( Buffer && buf );
    void assign( Buffer && buf );

    Mixed( GrowBuffer && buf );
    void assign( GrowBuffer && buf );

#endif

    /** \brief 释放相关资源 */
    void free();

    /** \brief 取得类型 */
    MixedType type() const { return this->_type; }
    /** \brief 取得类型串 */
    String const & typeString() const { return TypeString(this->_type); }

    // 取得相关类型的引用 --------------------------------------------------------------------
    #include "mixed_ref_specified_type.inl"

    // 类型转换 ----------------------------------------------------------------------------
    operator AnsiString() const;
    operator UnicodeString() const;
    operator Buffer() const;
    operator bool() const;
    operator byte() const;
    operator short() const;
    operator ushort() const;
    operator int() const;
    operator uint() const;
    operator long() const;
    operator ulong() const;
    operator float() const;
    operator int64() const;
    operator uint64() const;
    operator double() const;

    AnsiString toAnsi() const { return this->operator AnsiString(); }
    UnicodeString toUnicode() const { return this->operator UnicodeString(); }
    Buffer toBuffer() const { return this->operator Buffer(); }
    bool toBool() const { return this->operator bool(); }
    byte toByte() const { return this->operator winux::byte(); }
    short toShort() const { return this->operator short(); }
    ushort toUShort() const { return this->operator winux::ushort(); }
    int toInt() const { return this->operator int(); }
    uint toUInt() const { return this->operator winux::uint(); }
    long toLong() const { return this->operator long(); }
    ulong toULong() const { return this->operator winux::ulong(); }
    float toFloat() const { return this->operator float(); }
    int64 toInt64() const { return this->operator winux::int64(); }
    uint64 toUInt64() const { return this->operator winux::uint64(); }
    double toDouble() const { return this->operator double(); }

    // 比较操作符 --------------------------------------------------------------------------
    bool operator == ( Mixed const & other ) const;
    bool operator < ( Mixed const & other ) const;
    bool operator != ( Mixed const & other ) const { return !this->operator == (other); }
    bool operator > ( Mixed const & other ) const { return !this->operator <= (other); }
    bool operator >= ( Mixed const & other ) const { return !this->operator < (other); }
    bool operator <= ( Mixed const & other ) const { return this->operator < (other) || this->operator == (other); }

    // 判定特殊类型 -------------------------------------------------------------------------
    bool isNull() const { return this->_type == MT_NULL; }
    bool isArray() const { return this->_type == MT_ARRAY; }
    bool isCollection() const { return this->_type == MT_COLLECTION; }
    bool isContainer() const { return this->_type == MT_ARRAY || this->_type == MT_COLLECTION; }
    bool isBinary() const { return this->_type == MT_BINARY; }
    bool isNumeric() const { return this->_type > MT_NULL && this->_type < MT_ANSI; }
    bool isInteger() const { return this->isNumeric() && this->_type != MT_FLOAT && this->_type != MT_DOUBLE; }
    bool isAnsi() const { return this->_type == MT_ANSI; }
    bool isUnicode() const { return this->_type == MT_UNICODE; }
    bool isString() const { return this->_type == MT_ANSI || this->_type == MT_UNICODE; }

    // 创建相关类型 -------------------------------------------------------------------------
    /** \brief 创建一个Ansi字符串,并设置type为MT_STRING */
    Mixed & createString();
    /** \brief 创建一个Unicode字符串,并设置type为MT_UNICODE */
    Mixed & createUnicode();
    /** \brief 创建一个数组,自动把先前的数据清空,并设置type为MT_ARRAY */
    Mixed & createArray( uint count = 0 );
    /** \brief 创建一个集合,自动把先前的数据清空,并设置type为MT_COLLECTION */
    Mixed & createCollection();
    /** \brief 创建一个缓冲区,自动把先前的数据清空,并设置type为MT_BINARY */
    Mixed & createBuffer( uint size = 0 );

    // Array/Collection有关的操作 ----------------------------------------------------------

    /** \brief 取得数组全部元素，必须是MT_ARRAY/MT_COLLECTION类型 */
    template < typename _Ty >
    int getArray( std::vector<_Ty> * arr ) const
    {
        if ( !this->isArray() && !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getArray()" );
        MixedArray::const_iterator it;
        for ( it = this->_pArr->begin(); it != this->_pArr->end(); ++it )
            arr->push_back(*it);
        return (int)arr->size();
    }

    /** \brief 获取全部键名，必须是MT_COLLECTION类型 */
    template < typename _KTy >
    int getKeys( std::vector<_KTy> * keys ) const
    {
        if ( !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getKeys()" );
        MixedArray::const_iterator it;
        for ( it = this->_pArr->begin(); it != this->_pArr->end(); ++it )
            keys->push_back(*it);
        return (int)keys->size();
    }

    /** \brief 获取映射表，必须是MT_COLLECTION类型 */
    template < typename _KTy, typename _VTy >
    int getMap( std::map< _KTy, _VTy > * m ) const
    {
        if ( !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getMap()" );
        MixedMixedMap::const_iterator it;
        for ( it = this->_pMap->begin(); it != this->_pMap->end(); ++it )
            (*m)[(_KTy)it->first] = (_VTy)it->second;
        return (int)m->size();
    }

    /** \brief 判断容器是否为空 */
    bool isEmpty() const { return this->getCount() == 0; }

    /** \brief 获取Array/Collection元素个数
     *
     *  即使Mixed不是Array/Collection类型也不会报错，此时会返回0。 */
    int getCount() const
    {
        if ( ( this->isArray() || this->isCollection() ) && this->_pArr != NULL )
            return (int)this->_pArr->size();
        return 0;
    }

    /** \brief 下标操作 */
    Mixed & operator [] ( Mixed const & k );
    /** \brief const下标操作 */
    Mixed const & operator [] ( Mixed const & k ) const;
    /** \brief 下标操作 使兼容字符串指针 */
    template < typename _ChTy >
    Mixed & operator [] ( _ChTy const * k ) { return this->operator[]( Mixed(k) ); }
    /** \brief const下标操作 使兼容字符串指针 */
    template < typename _ChTy >
    Mixed const & operator [] ( _ChTy const * k ) const { return this->operator[]( Mixed(k) ); }

    /** \brief 当Mixed为Array或Collection类型时，get()能把指定'索引/Key'的元素按照指定类型取出来 */
    template < typename _Ty >
    _Ty get( Mixed const & k, Mixed const & defval = Mixed() ) const { return (_Ty)this->get( k, defval ); }

    /** \brief 当Mixed为Array或Collection类型时，取得指定'索引/Key'的元素，不存在则返回默认值 */
    Mixed const & get( Mixed const & k, Mixed const & defval = Mixed() ) const;

    /** \brief Collection获取'键值对'索引操作 */
    MixedMixedMap::value_type & getPair( int i );
    /** \brief Collection获取'键值对'索引操作 */
    MixedMixedMap::value_type const & getPair( int i ) const;

    class CollectionAssigner
    {
    public:
        CollectionAssigner( Mixed * mx ) : _mx(mx) { }
        CollectionAssigner & operator()( Mixed const & k, Mixed const & v )
        {
            if ( _mx->isCollection() )
            {
                _mx->_addUniqueKey(k);
                _mx->_pMap->operator[](k) = v;
            }
            return *this;
        }
        operator Mixed & () { return *_mx; }

    private:
        Mixed * _mx;
    };

    /** \brief 往Collection添加数据.
     *
     *  如果不是Collection,则自动释放之前数据,创建Collection. */
    CollectionAssigner addPair()
    {
        if ( this->_type != MT_COLLECTION ) this->createCollection();
        return CollectionAssigner(this);
    }

    /** \brief 往Collection添加一个pair. 非Collection类型调用此函数会抛异常. */
    Mixed & addPair( Mixed const & k, Mixed const & v );

    class ArrayAssigner
    {
    public:
        ArrayAssigner( Mixed * mx ) : _mx(mx) { }
        ArrayAssigner & operator()( Mixed const & v )
        {
            if ( _mx->isArray() )
            {
                _mx->_pArr->push_back(v);
            }
            return *this;
        }
        operator Mixed & () { return *_mx; }

    private:
        Mixed * _mx;
    };

    /** \brief 往Array添加数据.
     *
     *  如果不是Array,则自动释放之前数据,创建Array. */
    ArrayAssigner add()
    {
        if ( this->_type != MT_ARRAY ) this->createArray();
        return ArrayAssigner(this);
    }

    /** \brief 往数组里加一个元素,返回索引值,非Array类型调用此函数会抛异常 */
    int add( Mixed const & v );

    /** \brief 往数组里加一个唯一元素,返回索引值,非Array类型调用此函数会抛异常 */
    int addUnique( Mixed const & v );

    /** \brief 删除一个元素,操作类型可以是Array或Collection,k分别代表索引或键名 */
    void del( Mixed const & k );

    /** \brief 判断元素是否存在,Array判断值是否存在,Collection判断键名是否存在
     *
     *  即使Mixed不是Array/Collection类型也不会报错，此时会返回false。 */
    bool has( Mixed const & ek ) const;

    /** \brief 合并另一个容器或添加一个元素
     *
     *  非Array/Collection类型调用此函数会抛异常 */
    Mixed & merge( Mixed const & v );

    /** \brief 反转容器内元素顺序
     *
     *  非Array/Collection类型调用此函数会抛异常 */
    Mixed & reverse();

    // Buffer有关操作 --------------------------------------------------------------------------
    /** \brief 分配一块内存,自动释放先前数据,并设置type为MT_BINARY */
    void alloc( uint size );

    /** \brief 把窥探模式下的MT_BINARY类型变为拷贝模式，如果copyCapacity为true时连容量也一起拷贝，否则只拷贝数据。 */
    bool peekCopy( bool copyCapacity = false );

    /** \brief 取得缓冲区大小 */
    int getSize() const;

    /** \brief 暴露缓冲区指针 */
    void * getBuf() const;

    // 赋值操作 -------------------------------------------------------------------------------
    void assign( char const * str, int len = -1 );
    void assign( wchar const * str, int len = -1 );
    void assign( bool boolVal );
    void assign( byte btVal );
    void assign( short shVal );
    void assign( ushort ushVal );
    void assign( int iVal );
    void assign( uint uiVal );
    void assign( long lVal );
    void assign( ulong ulVal );
    void assign( float fltVal );
    void assign( int64 i64Val );
    void assign( uint64 ui64Val );
    void assign( double dblVal );

    void assign( Buffer const & buf );
    /** \brief 二进制数据赋值 */
    void assign( void * binaryData, uint size, bool isPeek = false );
    /** \brief 数组赋值 */
    void assign( Mixed * arr, uint count );

    template < typename _Ty >
    void assign( std::vector<_Ty> const & arr )
    {
        this->free();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray( arr.size() );
        uint i;
        for ( i = 0; i < (uint)arr.size(); ++i )
        {
            this->_pArr->at(i) = arr[i];
        }
    }

#if defined(__GNUC__) || _MSC_VER > 1200
    template < typename _Ty, uint n >
    void assign( _Ty (&arr)[n] )
    {
        this->free();
        this->_type = MT_ARRAY;
        this->_pArr = new MixedArray(n);
        uint i;
        for ( i = 0; i < n; ++i )
        {
            this->_pArr->at(i) = arr[i];
        }
    }
#endif

    /** \brief 用map给Collection赋值 */
    template < typename _KTy, typename _VTy, typename _Pr, typename _Alloc >
    void assign( std::map< _KTy, _VTy, _Pr, _Alloc > const & m )
    {
        this->free();
        this->_type = MT_COLLECTION;
        this->_pArr = new MixedArray(); // 存放keys
        this->_pMap = new MixedMixedMap();
        typename std::map< _KTy, _VTy, _Pr, _Alloc >::const_iterator it;
        for ( it = m.begin(); it != m.end(); ++it )
        {
            this->_pArr->push_back(it->first);
            (*this->_pMap)[it->first] = it->second;
        }
        //std::sort( this->_pArr->begin(), this->_pArr->end() );
    }

#if defined(__GNUC__) || _MSC_VER > 1200
    /** \brief 用pairs给Collection赋值 */
    template < typename _KTy, typename _VTy, uint count >
    void assign( std::pair< _KTy, _VTy > (&pairs)[count] )
    {
        this->free();
        this->_type = MT_COLLECTION;
        this->_pArr = new MixedArray(); // 存放keys
        this->_pMap = new MixedMixedMap();
        uint i;
        for ( i = 0; i < count; ++i )
        {
            this->_addUniqueKey(pairs[i].first);
            (*this->_pMap)[pairs[i].first] = pairs[i].second;
        }
    }

    /** \brief 用数组给Collection赋值 */
    template < typename _KTy, typename _VTy, uint count >
    void assign( _KTy (&keys)[count], _VTy (&vals)[count] )
    {
        this->free();
        this->_type = MT_COLLECTION;
        this->_pArr = new MixedArray(); // 存放keys
        this->_pMap = new MixedMixedMap();
        uint i;
        for ( i = 0; i < count; ++i )
        {
            this->_addUniqueKey(keys[i]);
            (*this->_pMap)[keys[i]] = vals[i];
        }
    }
#endif
    // JSON相关操作 ------------------------------------------------------------------------
    String myJson( bool autoKeyQuotes = true, AnsiString const & spacer = "", AnsiString const & newline = "" ) const;
    String json() const;
    Mixed & json( String const & jsonStr );

    // 类型解析功能 -------------------------------------------------------------------------
    /** \brief parse bool */
    static bool ParseBool( AnsiString const & str, bool * boolVal );
    static bool ParseBool( UnicodeString const & str, bool * boolVal );
    /** \brief parse ulong */
    static bool ParseULong( AnsiString const & str, ulong * ulVal );
    static bool ParseULong( UnicodeString const & str, ulong * ulVal );
    /** \brief parse double */
    static bool ParseDouble( AnsiString const & str, double * dblVal );
    static bool ParseDouble( UnicodeString const & str, double * dblVal );
    /** \brief parse uint64 */
    static bool ParseUInt64( AnsiString const & str, uint64 * ui64Val );
    static bool ParseUInt64( UnicodeString const & str, uint64 * ui64Val );

    /** \brief parse array or collection, it parse a json */
    static Mixed & ParseJson( AnsiString const & str, Mixed * val );

private:
    void _zeroInit();
    // MT_COLLECTION，给数组加入一个唯一键名
    void _addUniqueKey( Mixed const & k )
    {
        if ( this->_pMap->find(k) == this->_pMap->end() )
            this->_pArr->push_back(k);
    }
};

/** \brief 扩展iostream的<< */
WINUX_FUNC_DECL(std::ostream &) operator << ( std::ostream & o, Mixed const & m );
WINUX_FUNC_DECL(std::wostream &) operator << ( std::wostream & o, Mixed const & m );

//std::istream & operator >> ( std::istream & o, Mixed const & m );
//std::wistream & operator >> ( std::wistream & o, Mixed const & m );

} // namespace winux

#endif // __UTILITIES_HPP__

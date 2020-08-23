#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

// IS WINDOWS ( defined(_MSC_VER) || defined(WIN32) )
// IS LINUX ( defined(__GNUC__) && !defined(WIN32) )

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(WIN32)
#include <windows.h>
#endif

/** \brief 跨平台基础功能库 */
namespace winux
{
// macro definitions ------------------------------------------------------
/*Dll相关宏定义:
    WINUX_DLL_USE     - 此宏开关表示有DLL参与,包括生成DLL或者导入DLL,不定义此宏和一般的使用源码没区别
    WINUX_DLL_EXPORTS - 此宏开关表示是生成DLL(dllexport)还是导入DLL(dllimport),linux平台下忽略
    WINUX_DLL         - 标记函数、类、变量,用于标明其要从DLL导出还是导入,linux平台下忽略
    WINUX_API         - 标记函数调用约定,Win下Dll参与时为stdcall,否则为空白默认,linux平台下忽略
    WINUX_FUNC_DECL   - 标记函数声明
    WINUX_FUNC_IMPL   - 标记函数实现 */
#ifdef  WINUX_DLL_USE
    #if defined(_MSC_VER) || defined(WIN32)
        #pragma warning( disable: 4251 )
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
clsname & operator = ( clsname const & );\
clsname( clsname const & );

// 缓冲区转换为AnsiString二进制串
#define BufferToAnsiString( buf, size ) AnsiString( (char const *)(buf), (size_t)(size) )

// 如果指针非NULL
// IF_PTR
#define IF_PTR(ptr) if( (ptr) != NULL ) (ptr)
// ASSIGN_PTR
#define ASSIGN_PTR(ptr) if( (ptr) != NULL ) *(ptr)

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

// basic types -----------------------------------------------------------
//#ifdef UNICODE
//typedef wchar_t tchar;
//#else
typedef char tchar;
//#endif

typedef wchar_t wchar;
typedef int int32;
typedef unsigned int uint, uint32;
typedef unsigned long ulong;
typedef unsigned short ushort, uint16;
typedef char int8;
typedef short int16;
typedef unsigned char uint8;

#ifdef __GNUC__
typedef unsigned long long uint64;
typedef unsigned long long ulonglong;
typedef long long int64;
typedef long long longlong;
#else
typedef unsigned __int64 uint64;
typedef unsigned __int64 ulonglong;
typedef __int64 int64;
typedef __int64 longlong;
#endif
#ifndef byte
typedef unsigned char byte;
#endif

class Mixed;
// STL wrappers
typedef std::basic_string<char> String;
typedef std::basic_string<char> AnsiString;
typedef AnsiString LocalString;
typedef std::basic_string<wchar> UnicodeString;
typedef std::basic_string<ushort> UnicodeString16;

typedef std::vector<String> StringArray;
typedef std::map<String, String> StringStringMap;
typedef std::pair<String, String> StringStringPair;

typedef std::vector<Mixed> MixedArray;
typedef std::map<String, Mixed> StringMixedMap;
typedef std::pair<String, Mixed> StringMixedPair;
//typedef std::map<Mixed, Mixed> MixedMixedMap;
//typedef std::pair<Mixed, Mixed> MixedMixedPair;

/** \brief 检测map中是否有该键的值 */
template < typename _MAP, typename _KEY >
inline bool isset( _MAP const & m, _KEY const & k )
{
    return m.find(k) != m.end();
}

/** \brief 判断一个字符串值是否在一个字符串数组里,默认大小写敏感 */
WINUX_FUNC_DECL(bool) ValueIsInArray( StringArray const & arr, String const & val, bool caseInsensitive = false );

/** \brief 以文本模式载入文件内容作为字符串 */
WINUX_FUNC_DECL(String) FileGetContents( String const & filename );

/** \brief 把字符串以文本模式写入文件 */
WINUX_FUNC_DECL(bool) FilePutContents( String const & filename, String const & str );

/** \brief 随机数,随机产生n1~n2的数字. 包括n1,n2本身 */
WINUX_FUNC_DECL(int) Random( int n1, int n2 );

/** \brief 日志 */
WINUX_FUNC_DECL(void) WriteLog( String const & s );

/** \brief 二进制日志 */
WINUX_FUNC_DECL(void) WriteBinLog( void const * data, int size );

//#define __LOG__
#ifdef __LOG__
#define LOG(s) winux::WriteLog(s)
#define BIN_LOG(d,s) winux::WriteBinLog((d),(s))
#else
#define LOG(s)
#define BIN_LOG(d,s)
#endif

/** \brief 配置文件类 */
class WINUX_DLL Configure
{
private:
    String _configFile;
    StringStringMap _rawParams; ///< 未StripSlashes处理的数据集合

    static int _FindConfigRef( String const & str, int offset, int * length, String * name );
    String _expandVarNoStripSlashes( String const & name, StringArray * chains ) const;

    //返回加载的配置变量个数
    int _load( String const & configFile, StringStringMap * rawParams, StringArray * loadFileChains );
public:
    Configure();
    Configure( String const & configFile );

    /** \brief 载入配置文件，返回加载的配置变量个数。不会清空原数据 */
    int load( String const & configFile );

    /** \brief 判断是否含有该变量 */
    bool has( String const & name ) const { return _rawParams.find(name) != _rawParams.end(); }

    /** \brief 按指定方式获取变量值 */
    String get( String const & name, bool stripslashes = false, bool expand = false ) const;

    /** \brief 获取变量未展开的值 */
    String operator [] ( String const & name ) const;

    /** \brief 获取变量展开的值 */
    String operator () ( String const & name ) const;

    /** \brief 以RAW方式设置一个配置变量
     *
     * 必须是单行字符串值，特殊字符必须反转义 */
    void setRaw( String const & name, String const & value );

    /** \brief 设置一个配置变量
     *
     *  值会自动反转义，因此无法包含$(XXX)型的内部待展开变量，因为set()内部会自动反转义变成\\$\\(XXX\\)。
     *  需要设置$(XXX)型内部待展开变量的请使用setRaw()。 */
    void set( String const & name, String const & value );

    /** \brief 删除一个配置变量 */
    bool del( String const & name );

    /** \brief 清空所有配置变量 */
    void clear();

    /** \brief 取得内部StringStringMap引用 */
    StringStringMap const & getAll() const { return _rawParams; }
};

// 模板元编程支持 ---------------------------------------------------------------------------
/** 将C数组转换成vector */
template < typename _Ty >
std::vector<_Ty> ToArray( _Ty * arr, uint count )
{
    return std::vector<_Ty>( arr, arr + count );
}

#if defined(__GNUC__) || _MSC_VER >= 1600
/* VC2010以上支持模板取数组大小 */
template < typename _Ty, uint _N >
std::vector<_Ty> ToArray( _Ty (&arr)[_N] )
{
    return std::vector<_Ty>( arr, arr + _N );
}

#else
/* 否则使用宏定义 */
#define ToArray(a) ToArray( a, countof(a) )

#endif

/** 调用一个返回void的函数或函数对象,返回一个数字
   通常是为了在初始化语句中方便调用返回void的函数 */
template < typename FN >
int VoidReturnInt( FN fn )
{
    fn();
    return 1;
}

template < typename FN, typename ARG1 >
int VoidReturnInt( FN fn, ARG1 a1 )
{
    fn(a1);
    return 1;
}
template < typename FN, typename ARG1, typename ARG2 >
int VoidReturnInt( FN fn, ARG1 a1, ARG2 a2 )
{
    fn( a1, a2 );
    return 1;
}
template < typename FN, typename ARG1, typename ARG2, typename ARG3 >
int VoidReturnInt( FN fn, ARG1 a1, ARG2 a2, ARG3 a3 )
{
    fn( a1, a2, a3 );
    return 1;
}

/** \brief 二进制数,编译时计算, 0开头(基于8进制) */
template < uint64 n > struct Bin0
{
    enum { val = ( Bin0< n / 8 >::val  << 1 ) + n % 8 };
};

template <> struct Bin0<0>
{
    enum { val = 0 };
};

// 二进制数 macro包装
// BIN_VAL()
#define BinVal(x) winux::Bin0<0##x>::val

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

/** \brief 函数包装,用来将不同调用约定的函数统一包装成默认约定 */
template < typename _PfnType, _PfnType Fn_, typename _RetType >
struct FuncWrapper
{
#include "func_wrappers_type.inl"
};

template < typename _PfnType, _PfnType Fn_ >
struct FuncWrapper< _PfnType, Fn_, void >
{
#include "func_wrappers_void.inl"
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

    MembersWrapper( MembersWrapper const & other );
#ifndef MOVE_SEMANTICS_DISABLED
    MembersWrapper( MembersWrapper && other );
#endif
public:
    MembersWrapper() : _data(0) { }

    MembersWrapper & operator = ( MembersWrapper const & other )
    {
        if ( &other != this )
        {
            //this->create(); // 赋值不应该要这句
            *_data = *other._data;
        }
        return *this;
    }

#ifndef MOVE_SEMANTICS_DISABLED
    MembersWrapper & operator = ( MembersWrapper && other )
    {
        if ( &other != this )
        {
            this->destroy();
            _data = other._data;
            other._data = 0;
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
            _data = 0;
        }
    }
    /** \brief 必须在使用者类的构造函数里第一个调用 */
    void create()
    {
        this->destroy();
        _data = new _TargetCls;
    }

    template < typename _Arg1 >
    void create( _Arg1 a1 )
    {
        this->destroy();
        _data = new _TargetCls(a1);
    }

    template < typename _Arg1, typename _Arg2 >
    void create( _Arg1 a1, _Arg2 a2 )
    {
        this->destroy();
        _data = new _TargetCls( a1, a2 );
    }

    template < typename _Arg1, typename _Arg2, typename _Arg3 >
    void create( _Arg1 a1, _Arg2 a2, _Arg3 a3 )
    {
        this->destroy();
        _data = new _TargetCls( a1, a2, a3 );
    }

    template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4 >
    void create( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4 )
    {
        this->destroy();
        _data = new _TargetCls( a1, a2, a3, a4 );
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
        return _data != 0;
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

// -------------------------------------------------------------------------------
class GrowBuffer;
/** \brief 缓冲区,表示内存中一块2进制数据(利用malloc/realloc进行内存分配) */
class WINUX_DLL Buffer
{
public:
    Buffer();
    /** \brief 从一个缓冲区创建Buffer,可以指定是否为窥视模式 */
    Buffer( void * buf, uint size, bool isPeek = false );
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

    /** \brief 设置缓冲区 */
    void setBuf( void * buf, uint size, bool isPeek = false );

    /** \brief 分配内存 */
    void alloc( uint size );

    /** \brief 调整实际分配空间的大小,保留数据*/
    void realloc( uint newSize );

    /** \brief 把窥探模式变为拷贝模式 */
    bool peekCopy();

    /** \brief 释放缓冲区 */
    virtual void free();

    /** \brief 暴露缓冲区指针 */
    void * getBuf() const { return _buf; }

    /** \brief 获取数据大小*/
    virtual uint getSize() const;
protected:
    virtual void * _alloc( uint size );
    virtual void * _realloc( void * p, uint newSize );
    virtual void _free( void * p );

    void * _buf;
    uint _actualSize;
    bool _isPeek;

    friend class GrowBuffer;
};

/** \brief 高效的可增长缓冲区，1.33倍冗余量 */
class WINUX_DLL GrowBuffer : public Buffer
{
public:
    /** \brief 构造函数，初始化缓冲区的大小 */
    explicit GrowBuffer( uint initSize = 0 );
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

    virtual void free();
    /** \brief 获取数据大小 */
    virtual uint getSize() const;
    /** \brief 添加数据 */
    void append( void * data, uint size );
    /** \brief 擦除数据，自动紧缩 */
    void erase( uint start, uint count = (uint)-1 );

    /** \brief 获取实际分配的缓冲区大小 */
    uint getActualSize() const { return _actualSize; }
protected:
    uint _dataSize; // 数据的大小

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
    enum MixedType
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
    Mixed & createBuffer( uint size );

    // Array/Collection有关的操作 ----------------------------------------------------------

    /** \brief 取得数组全部元素，必须是MT_ARRAY/MT_COLLECTION类型 */
    template < typename _Ty >
    int getArray( std::vector<_Ty> * arr ) const
    {
        if ( !this->isArray() && !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getArray()" );
        MixedArray::const_iterator it;
        for ( it = this->_pArr->begin(); it != this->_pArr->end(); ++it )
            arr->push_back(*it);
        return arr->size();
    }

    /** \brief 获取全部键名，必须是MT_COLLECTION类型 */
    template < typename _KTy >
    int getKeys( std::vector<_KTy> * keys ) const
    {
        if ( !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getKeys()" );
        MixedArray::const_iterator it;
        for ( it = this->_pArr->begin(); it != this->_pArr->end(); ++it )
            keys->push_back(*it);
        return keys->size();
    }

    /** \brief 获取映射表，必须是MT_COLLECTION类型 */
    template < typename _KTy, typename _VTy >
    int getMap( std::map< _KTy, _VTy > * m ) const
    {
        if ( !this->isCollection() ) throw MixedError( MixedError::meUnexpectedType, TypeString(this->_type) + " can't support getMap()" );
        MixedMixedMap::const_iterator it;
        for ( it = this->_pMap->begin(); it != this->_pMap->end(); ++it )
            (*m)[(_KTy)it->first] = (_VTy)it->second;
        return m->size();
    }

    /** \brief 判断容器是否为空 */
    bool isEmpty() const { return this->getCount() == 0; }

    /** \brief 获取Array/Collection元素个数
     *
     *  即使Mixed不是Array/Collection类型也不会报错，此时会返回0。 */
    int getCount() const;

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
    _Ty get( Mixed const & k ) const { return (_Ty)this->operator [] (k); }

    /** \brief Collection获取'键值对'索引操作 */
    MixedMixedMap::value_type & getPair( int i );
    /** \brief Collection获取'键值对'索引操作 */
    MixedMixedMap::value_type const & getPair( int i ) const;

    class WINUX_DLL CollectionAssigner
    {
        Mixed * _mx;
    public:
        CollectionAssigner( Mixed * mx ) : _mx(mx) { }
        CollectionAssigner & operator()( Mixed const & k, Mixed const & v );
        operator Mixed & () { return *_mx; }
    };

    /** \brief 往Collection添加数据.
     *
     *  如果不是Collection,则自动释放之前数据,创建Collection. */
    CollectionAssigner addPair();

    /** \brief 往Collection添加一个pair. 非Collection类型调用此函数会抛异常. */
    Mixed & addPair( Mixed const & k, Mixed const & v );

    class WINUX_DLL ArrayAssigner
    {
        Mixed * _mx;
    public:
        ArrayAssigner( Mixed * mx ) : _mx(mx) { }
        ArrayAssigner & operator()( Mixed const & v );
        operator Mixed & () { return *_mx; }
    };

    /** \brief 往Array添加数据.
     *
     *  如果不是Array,则自动释放之前数据,创建Array. */
    ArrayAssigner add();

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

    // Buffer有关操作 --------------------------------------------------------------------------
    /** \brief 分配一块内存,自动释放先前数据,并设置type为MT_BINARY */
    void alloc( uint size );

    /** \brief 把窥探模式下的MT_BINARY类型变为拷贝模式 */
    bool peekCopy();

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
        for ( i = 0; i < arr.size(); ++i )
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
    String json() const;
    String myJson() const;
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

#pragma once

// 函数特征

// 其他不可调用类型
template < typename _Ty >
struct FuncTraits;

#define _NON_MEMBER_FUNCTION_TRAITS(CALL_OPT) \
/* 普通函数 */ \
template < typename _RetType, typename... _ArgTypes > \
struct FuncTraits< _RetType CALL_OPT( _ArgTypes... ) > \
{ \
    enum { Arity = sizeof...(_ArgTypes) }; /* 参数个数 */ \
    using ReturnType = _RetType; /* 返回值类型 */ \
    using FunctionType = _RetType CALL_OPT( _ArgTypes... ); /* 函数类型 */ \
    using FunctionPtr = _RetType(CALL_OPT*)( _ArgTypes... ); /* 函数指针类型 */ \
    using ParamsTuple = std::tuple<_ArgTypes...>; /* 参数元组类型 */ \
    using StdFunction = std::function<FunctionType>; /* std::function类型 */ \
    using ClassType = void; \
    /* 各参数类型 */ \
    template < size_t _Idx > \
    struct Argument \
    { \
        static_assert( _Idx < Arity, "Index is out of range, it must less than sizeof...(_ArgTypes)." ); \
        using Type = typename std::tuple_element< _Idx, ParamsTuple >::type; \
    }; \
}; \
/* 普通函数指针 */ \
template < typename _RetType, typename... _ArgTypes > \
struct FuncTraits< _RetType(CALL_OPT*)( _ArgTypes... ) > : FuncTraits< _RetType CALL_OPT( _ArgTypes... ) > { }; \
/* std::function */ \
template < typename _RetType, typename... _ArgTypes > \
struct FuncTraits< std::function< _RetType CALL_OPT( _ArgTypes... ) > > : FuncTraits< _RetType CALL_OPT( _ArgTypes... ) > { };

#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8 || defined(OS_WIN64)

#if _MSC_VER > 0
#pragma warning(push)
#pragma warning( disable: 4003 )
#endif

_NON_MEMBER_FUNCTION_TRAITS( )

#if _MSC_VER > 0
#pragma warning(pop)
#endif

#else
_NON_MEMBER_FUNCTION_TRAITS(__cdecl)
_NON_MEMBER_FUNCTION_TRAITS(__stdcall)
_NON_MEMBER_FUNCTION_TRAITS(__fastcall)
#endif

/* 类成员函数 */
#define _MEMBER_FUNCTION_TRAITS(...) \
template < class _Cls, typename _RetType, typename... _ArgTypes > \
struct FuncTraits< _RetType(_Cls::*)( _ArgTypes... ) __VA_ARGS__ > : FuncTraits< _RetType( _ArgTypes... ) > { using ClassType = _Cls; };

_MEMBER_FUNCTION_TRAITS()
_MEMBER_FUNCTION_TRAITS(const)
_MEMBER_FUNCTION_TRAITS(volatile)
_MEMBER_FUNCTION_TRAITS(const volatile)

/* 可调用对象 */
template < class _Callable >
struct FuncTraits : FuncTraits< decltype( &_Callable::operator() ) > { using ClassType = void; };

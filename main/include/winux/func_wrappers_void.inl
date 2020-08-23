// 调用模板 参数个数0~9
static void func()
{
    (*Fn_)();
}

template < typename _Arg1 >
static void func( _Arg1 a1 )
{
    (*Fn_)(a1);
}

template < typename _Arg1, typename _Arg2 >
static void func( _Arg1 a1, _Arg2 a2 )
{
    (*Fn_)( a1, a2 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3 )
{
    (*Fn_)( a1, a2, a3 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4 )
{
    (*Fn_)( a1, a2, a3, a4 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5 )
{
    (*Fn_)( a1, a2, a3, a4, a5 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6 )
{
    (*Fn_)( a1, a2, a3, a4, a5, a6 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7 )
{
    (*Fn_)( a1, a2, a3, a4, a5, a6, a7 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7, typename _Arg8 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7, _Arg8 a8 )
{
    (*Fn_)( a1, a2, a3, a4, a5, a6, a7, a8 );
}

template < typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7, typename _Arg8, typename _Arg9 >
static void func( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7, _Arg8 a8, _Arg9 a9 )
{
    (*Fn_)( a1, a2, a3, a4, a5, a6, a7, a8, a9 );
}

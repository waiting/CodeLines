// 调用模板 参数个数0~9

template < typename _RetType >
Caller<_RetType> call()
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn );
}

template < typename _RetType, typename _Arg1 >
Caller<_RetType> call( _Arg1 a1 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1 );
}

template < typename _RetType, typename _Arg1, typename _Arg2 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4, a5 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4, a5, a6 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4, a5, a6, a7 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7, typename _Arg8 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7, _Arg8 a8 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4, a5, a6, a7, a8 );
}

template < typename _RetType, typename _Arg1, typename _Arg2, typename _Arg3, typename _Arg4, typename _Arg5, typename _Arg6, typename _Arg7, typename _Arg8, typename _Arg9 >
Caller<_RetType> call( _Arg1 a1, _Arg2 a2, _Arg3 a3, _Arg4 a4, _Arg5 a5, _Arg6 a6, _Arg7 a7, _Arg8 a8, _Arg9 a9 )
{
    if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
    return Caller<_RetType>( _pfn, a1, a2, a3, a4, a5, a6, a7, a8, a9 );
}

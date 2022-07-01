#pragma once

// 调用器 ------------------------------------------------------------------------------------
// RunableT和Invoker的区别在于Tuple类型不同。
// RunableT是用decay_t<T>创建的元组，而Invoker则是直接使用FuncTraits::ParamsTuple
// 若是类成员方法的调用，则RunableT元组还保存了对象指针；而Invoker则使用单独的参数传递对象指针
template <
    typename _Fx,
    typename = typename winux::FuncTraits<_Fx>::ClassType,
    typename = typename winux::FuncTraits<_Fx>::ReturnType
>
class Invoker
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using ParamsTuple = typename FuncTraits::ParamsTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size<ParamsTuple>::value >::Type;
    using ClassType = typename FuncTraits::ClassType;
    using ReturnType = typename FuncTraits::ReturnType;

    template < typename... _ArgType >
    Invoker( _Fx func, ClassType * obj, _ArgType&&... arg ) : _func(func), _obj(obj), _params( std::forward<_ArgType>(arg)... )
    {
    }

    ReturnType invoke()
    {
        return _invoke( ParamsIndexSequence() );
    }
protected:
    template < size_t... _Index >
    ReturnType _invoke( IndexSequence<_Index...> )
    {
        return (_obj->*_func)( std::get<_Index>(_params)... );
    }

    _Fx _func;
    ClassType * _obj;
    ParamsTuple _params;
};

template < typename _Fx >
class Invoker<
    _Fx,
    typename std::enable_if< !std::is_same< typename winux::FuncTraits<_Fx>::ClassType, void >::value, typename winux::FuncTraits<_Fx>::ClassType >::type,
    void
>
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using ParamsTuple = typename FuncTraits::ParamsTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size<ParamsTuple>::value >::Type;
    using ClassType = typename FuncTraits::ClassType;

    template < typename... _ArgType >
    Invoker( _Fx func, ClassType * obj, _ArgType&&... arg ) : _func(func), _obj(obj), _params( std::forward<_ArgType>(arg)... )
    {
    }

    void invoke()
    {
        _invoke( ParamsIndexSequence() );
    }
protected:
    template < size_t... _Index >
    void _invoke( IndexSequence<_Index...> )
    {
        (_obj->*_func)( std::get<_Index>(_params)... );
    }

    _Fx _func;
    ClassType * _obj;
    ParamsTuple _params;
};

template < typename _Fx >
class Invoker<
    _Fx,
    void,
    typename std::enable_if< !std::is_same< typename winux::FuncTraits<_Fx>::ReturnType, void >::value, typename winux::FuncTraits<_Fx>::ReturnType >::type
>
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using ParamsTuple = typename FuncTraits::ParamsTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size<ParamsTuple>::value >::Type;
    using ReturnType = typename FuncTraits::ReturnType;

    template < typename... _ArgType >
    Invoker( _Fx func, _ArgType&&... arg ) : _func(func), _params( std::forward<_ArgType>(arg)... )
    {
    }

    ReturnType invoke()
    {
        return _invoke( ParamsIndexSequence() );
    }
protected:
    template < size_t... _Index >
    ReturnType _invoke( IndexSequence<_Index...> )
    {
        return _func( std::get<_Index>(_params)... );
    }

    _Fx _func;
    ParamsTuple _params;
};

template < typename _Fx >
class Invoker<
    _Fx,
    void,
    void
>
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using ParamsTuple = typename FuncTraits::ParamsTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size<ParamsTuple>::value >::Type;

    template < typename... _ArgType >
    Invoker( _Fx func, _ArgType&&... arg ) : _func(func), _params( std::forward<_ArgType>(arg)... )
    {
    }

    void invoke()
    {
        _invoke( ParamsIndexSequence() );
    }
protected:
    template < size_t... _Index >
    void _invoke( IndexSequence<_Index...> )
    {
        _func( std::get<_Index>(_params)... );
    }

    _Fx _func;
    ParamsTuple _params;
};

template < typename _Fx, typename... _ArgType >
Invoker<_Fx> * NewInvoker( _Fx fn, _ArgType&& ... arg )
{
    return new Invoker<_Fx>( fn, std::forward<_ArgType>(arg)... );
}

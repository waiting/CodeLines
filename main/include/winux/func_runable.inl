#pragma once

// 提供一个run()接口
class Runable
{
public:
    virtual ~Runable() { }
    virtual void run() = 0;
    virtual Runable * clone() = 0;
};

// 提供可返回值的接口invoke()
template < typename _RetType >
class RunableInvoker : public Runable
{
public:
    virtual _RetType invoke() = 0;
};

// RunableT和Invoker的区别在于Tuple类型不同。
// RunableT是用decay_t<T>创建的元组，而Invoker则是直接使用FuncTraits::ParamsTuple
// 若是类成员方法的调用，则RunableT元组还保存了对象指针；而Invoker则使用单独的参数传递对象指针
template < typename _Fx, typename _TargetTuple, typename = typename winux::FuncTraits<_Fx>::ClassType >
class RunableT : public RunableInvoker<typename winux::FuncTraits<_Fx>::ReturnType>
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using TargetTuple = _TargetTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size< typename FuncTraits::ParamsTuple >::value >::Type;
    using ReturnType = typename winux::FuncTraits<_Fx>::ReturnType;
    using ClassType = typename winux::FuncTraits<_Fx>::ClassType;

    template < typename... _ArgType >
    RunableT( _Fx pfn, _ArgType&&... arg ) : _pfn(pfn), _tuple( std::forward<_ArgType>(arg)... )
    {
    }

    virtual void run()
    {
        _invoke( ParamsIndexSequence() );
    }

    virtual RunableT * clone()
    {
        return new RunableT(_pfn, _tuple);
    }

    virtual ReturnType invoke()
    {
        return _invoke( ParamsIndexSequence() );
    }

    template < size_t... _Index >
    ReturnType _invoke( IndexSequence<_Index...> )
    {
        return (std::get<0>(_tuple)->*_pfn)( std::get<_Index + 1>(_tuple)... );
    }

    _Fx _pfn;
    TargetTuple _tuple;

protected:
    RunableT( _Fx pfn, TargetTuple && tuple ) : _pfn(pfn), _tuple( std::forward<TargetTuple>(tuple) )
    {
    }
};

template < typename _Fx, typename _TargetTuple >
class RunableT< _Fx, _TargetTuple, void > : public RunableInvoker<typename winux::FuncTraits<_Fx>::ReturnType>
{
public:
    using FuncTraits = winux::FuncTraits<_Fx>;
    using TargetTuple = _TargetTuple;
    using ParamsIndexSequence = typename MakeIndexSequence< std::tuple_size< typename FuncTraits::ParamsTuple >::value >::Type;
    using ReturnType = typename winux::FuncTraits<_Fx>::ReturnType;
    using ClassType = typename winux::FuncTraits<_Fx>::ClassType;

    template < typename... _ArgType >
    RunableT( _Fx fn, _ArgType&&... arg ) : _fn(fn), _tuple( std::forward<_ArgType>(arg)... )
    {
    }

    virtual void run()
    {
        _invoke( ParamsIndexSequence() );
    }

    virtual RunableT * clone()
    {
        return new RunableT(_fn, _tuple);
    }

    virtual ReturnType invoke()
    {
        return _invoke( ParamsIndexSequence() );
    }

    template < size_t... _Index >
    ReturnType _invoke( IndexSequence<_Index...> )
    {
        return _fn( std::get<_Index>(_tuple)... );
    }

    _Fx _fn;
    TargetTuple _tuple;

protected:
    RunableT( _Fx fn, TargetTuple && tuple ) : _fn(fn), _tuple( std::forward<TargetTuple>(tuple) )
    {
    }
};

/** \brief 创建一个Runable对象
 *
 *  \param fn 执行函数。可以是普通函数，成员函数，lambda表达式。
 *  \param arg... 传递的参数。内部会使用tuple保存参数的副本（会存在一个实际的同类型对象）。所以要想不进行值拷贝构造，可以使用std::ref()保存引用包装对象，或者使用std::move()触发移动构造。 */
template < typename _Fx, typename... _ArgType >
RunableT< _Fx, std::tuple< typename std::decay<_ArgType>::type... > > * NewRunable( _Fx fn, _ArgType&&... arg )
{
    return new RunableT< _Fx, std::tuple<typename std::decay<_ArgType>::type...> >( fn, std::forward<_ArgType>(arg)... );
}

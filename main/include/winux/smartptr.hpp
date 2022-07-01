#ifndef __SMARTPTR_HPP__
#define __SMARTPTR_HPP__

namespace winux
{
/** \brief 构造分配器 */
template < typename _Ty >
struct Allocator
{
    static _Ty * New() { return new _Ty; }
    static _Ty * NewArray( std::size_t count ) { return new _Ty[count]; }
};

/** \brief 析构释放器 */
template < typename _Ty >
struct Deallocator
{
    static void Delete( _Ty * p ) { delete (_Ty *)p; }
    static void DeleteArray( _Ty * p ) { delete [] (_Ty *)p; }
};

/** \brief Simple删除器场景基类 */
class WINUX_DLL SimpleDeleterContext
{
private:
    virtual void _destroy() = 0;
    virtual void _deleteThis() = 0;
protected:
    SimpleDeleterContext() { }
    virtual ~SimpleDeleterContext() { }
public:
    /** \brief 销毁资源和SimpleDeleterContext自身 */
    void release()
    {
        this->_destroy();
        this->_deleteThis();
    }

    /** \brief 显式删除detach()出来的SimpleDeleterContext对象 */
    void delThis() { this->_deleteThis(); }
};

/** \brief Simple默认删除器场景 */
template < typename _HTy >
class SimpleDefaultDeleterContext : public SimpleDeleterContext
{
public:
    SimpleDefaultDeleterContext( _HTy h ) : _h(h) { }
private:
    virtual void _destroy() { delete _h; }
    virtual void _deleteThis() { delete this; }

    _HTy _h;
};

/** \brief Simple自定义删除器场景 */
template < typename _HTy, typename _Dt >
class SimpleCustomDeleterContext : public SimpleDeleterContext
{
public:
    SimpleCustomDeleterContext( _HTy h, _Dt dt ) : _h(h), _dt(dt) { }
private:
    virtual void _destroy() { _dt(_h); }
    virtual void _deleteThis() { delete this; }

    _HTy _h;
    _Dt _dt;
};

/** \brief 简单句柄类，管理各种资源的自动释放，赋值相当于传递管理权。
 *
 *  如 ptr2 = ptr1; ptr1将管理权传递到ptr2, ptr1失效.
 *  还能指定一个failVal指示资源分配失败后的句柄值，当不等于这个值时才会创建删除器场景释放资源 */
template < typename _HTy >
class SimpleHandle
{
public:
    typedef _HTy HType;

    struct SimpleHandleData
    {
        _HTy h;
        SimpleDeleterContext * ctx;
        SimpleHandleData() : h(0), ctx(0) { }
        SimpleHandleData( _HTy h, SimpleDeleterContext * ctx ) : h(h), ctx(ctx) { }
    };

    SimpleHandle() { }

    SimpleHandle( _HTy h, _HTy failVal ) { attachNew( h, failVal ); }

    template < typename _Dt >
    SimpleHandle( _HTy h, _HTy failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    template < typename _HTy2 >
    SimpleHandle( _HTy2 h, _HTy2 failVal ) { attachNew( h, failVal ); }

    template < typename _HTy2, typename _Dt >
    SimpleHandle( _HTy2 h, _HTy2 failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    virtual ~SimpleHandle()
    {
        reset();
    }

    SimpleHandle( SimpleHandle const & other )
    {
        _reset(other);
    }

    SimpleHandle & operator = ( SimpleHandle const & other )
    {
        _reset(other);
        return *this;
    }

    template < typename _HTy2 >
    SimpleHandle( SimpleHandle<_HTy2> const & other )
    {
        _reset(other);
    }

    template < typename _HTy2 >
    SimpleHandle & operator = ( SimpleHandle<_HTy2> const & other )
    {
        _reset(other);
        return *this;
    }

    void attachNew( _HTy h, _HTy failVal )
    {
        this->_reset0( h, ( h != failVal ? new SimpleDefaultDeleterContext<_HTy>(h) : 0 ) );
    }

    template < typename _Dt >
    void attachNew( _HTy h, _HTy failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SimpleCustomDeleterContext< _HTy, _Dt >( h, dt ) : 0 ) );
    }

    template < typename _HTy2 >
    void attachNew( _HTy2 h, _HTy2 failVal )
    {
        this->_reset0( h, ( h != failVal ? new SimpleDefaultDeleterContext<_HTy2>(h) : 0 ) );
    }
    template < typename _HTy2, typename _Dt >
    void attachNew( _HTy2 h, _HTy2 failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SimpleCustomDeleterContext< _HTy2, _Dt >( h, dt ) : 0 ) );
    }

    void attach( SimpleHandleData const & data )
    {
        _reset0( data.h, data.ctx );
    }

    template < typename _HTy2 >
    void attach( typename SimpleHandle<_HTy2>::SimpleHandleData const & data )
    {
        _reset0( data.h, data.ctx );
    }

    template < typename _HTy2 >
    void attach( _HTy2 h, SimpleDeleterContext * ctx )
    {
        _reset0( h, ctx );
    }

    SimpleHandleData detach()
    {
        SimpleHandleData r = _self;
        _self.h = static_cast<_HTy>(0);
        _self.ctx = 0;
        return r;
    }

    void reset()
    {
        _reset0( static_cast<_HTy>(0), 0 );
    }

    _HTy get() const { return _self.h; }

    operator bool() const { return _self.ctx != 0; }

    _HTy operator -> ()
    {
        return _self.h;
    }

    _HTy operator -> () const
    {
        return _self.h;
    }

protected:
    /** \brief 释放自身资源，管理新资源 */
    template < typename _HTy2 >
    void _reset0( _HTy2 newH, SimpleDeleterContext * newCtx )
    {
        if ( _self.ctx )
            _self.ctx->release();
        _self.h = newH;
        _self.ctx = newCtx;
    }

    /** \brief 释放自身资源，接管另一个simple的资源，另一个simple置零 */
    template < typename _HTy2 >
    void _reset( _HTy2 & otherH, SimpleDeleterContext * & otherCtx )
    {
        _reset0( otherH, otherCtx );
        otherH = static_cast<_HTy2>(0);
        otherCtx = 0;
    }

    /** \brief 释放自身资源，接管另一个simple的资源，另一个simple置零 */
    template < typename _HTy2 >
    void _reset( SimpleHandle<_HTy2> const & other )
    {
        SimpleHandle<_HTy2> & o = const_cast< SimpleHandle<_HTy2> & >(other);
        _reset( o._self.h, o._self.ctx );
    }

    SimpleHandleData _self;
private:
    template < typename _HTy0 >
    friend class SimpleHandle;
};

/** \brief 简单指针 */
template < typename _Ty >
class SimplePointer : public SimpleHandle<_Ty*>
{
public:
    typedef SimpleHandle<_Ty*> MyBase;
    typedef _Ty Type;

    SimplePointer() { }

    explicit SimplePointer( _Ty* p ) : MyBase( p, (_Ty*)0 ) { }

    template < typename _Dt >
    SimplePointer( _Ty* p, _Dt dt ) : MyBase( p, (_Ty*)0, dt ) { }

    template < typename _Ty2 >
    explicit SimplePointer( _Ty2* p ) : MyBase( p, (_Ty2*)0 ) { }

    template < typename _Ty2, typename _Dt >
    SimplePointer( _Ty2* p, _Dt dt ) : MyBase( p, (_Ty2*)0, dt ) { }

    SimplePointer( SimplePointer const & other )
    {
        this->_reset(other);
    }

    SimplePointer & operator = ( SimplePointer const & other )
    {
        this->_reset(other);
        return *this;
    }

    template < typename _Ty2 >
    SimplePointer( SimplePointer<_Ty2> const & other )
    {
        this->_reset(other);
    }

    template < typename _Ty2 >
    SimplePointer & operator = ( SimplePointer<_Ty2> const & other )
    {
        this->_reset(other);
        return *this;
    }

    void attachNew( _Ty * p )
    {
        MyBase::attachNew( p, (_Ty*)0 );
    }

    template < typename _Dt >
    void attachNew( _Ty * p, _Dt dt )
    {
        MyBase::attachNew( p, (_Ty*)0, dt );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 * p )
    {
        MyBase::attachNew( p, (_Ty2*)0 );
    }

    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 * p, _Dt dt )
    {
        MyBase::attachNew( p, (_Ty2*)0, dt );
    }

    /** \brief 把指针由_Ty转换成_Ty2类型
     *
     *  通常_Ty为基类，_Ty2为派生类，并且_Ty指针所指的是_Ty2类型的对象。
     *  如果转换失败，返回空指针，自身改不变。如果转换成功，自身放弃所有权，置零。 */
    template < typename _Ty2 >
    SimplePointer<_Ty2> cast()
    {
        SimplePointer<_Ty2> r;
        typename SimplePointer<_Ty2>::HType p = dynamic_cast< typename SimplePointer<_Ty2>::HType >(this->_self.h);
        if ( p != 0 )
        {
            r._reset( p, this->_self.ctx );
            this->_self.h = static_cast<typename MyBase::HType>(0);
            this->_self.ctx = 0;
        }
        return r;
    }

    /** \brief 把指针由_Ty转换成_Ty2类型
     *
     *  通常_Ty为基类，_Ty2为派生类，并且_Ty指针所指的必须是_Ty2类型的对象，才能使用这个函数。
     *  你必须担保转换成功，否则将产生灾难。 */
    template < typename _Ty2 >
    SimplePointer<_Ty2> ensureCast()
    {
        SimplePointer<_Ty2> r;
        typename SimplePointer<_Ty2>::HType p = static_cast< typename SimplePointer<_Ty2>::HType >(this->_self.h);
        r._reset( p, this->_self.ctx );
        this->_self.h = static_cast<typename MyBase::HType>(0);
        this->_self.ctx = 0;
        return r;
    }

    template < typename _Ty0 >
    friend class SimplePointer;
};

///////////////////////////////////////////////////////////////////////////////////////////

/** \brief 原子化使一个Long型变量+1，返回值是+1后的*p值 */
WINUX_FUNC_DECL(long) LongAtomicIncrement( long volatile * p );
/** \brief 原子化使一个Long型变量-1，返回值是-1后的*p值 */
WINUX_FUNC_DECL(long) LongAtomicDecrement( long volatile * p );
/** \brief 原子化操作，*p若和comparand相等，就把*p赋成exchange，返回值是初始的*p值 */
WINUX_FUNC_DECL(long) LongAtomicCompareExchange( long volatile * p,  long exchange, long comparand );

/** \brief Shared删除器场景基类 */
class WINUX_DLL SharedDeleterContext
{
private:
    long volatile _uses;
    long volatile _weaks;

    /** \brief 销毁资源 */
    virtual void _destroy() = 0;
    /** \brief 删除引用计数数据场景 */
    virtual void _deleteThis() = 0;
protected:
    SharedDeleterContext() : _uses(1), _weaks(1) { }
    virtual ~SharedDeleterContext() { }

public:
    /** \brief 如果引用计数不是0，则增加引用计数。成功则返回true。
     *
     *  用于Weak*PTR*创建Shared*PTR*时检测所持资源的计数是否不为0，即还有效。*/
    bool _incRefNz()
    {
        for ( ; ; )
        {
            // loop until state is known
            long count = (long volatile &)_uses;
            if ( count == 0 ) return false;
            if ( LongAtomicCompareExchange( &_uses, count + 1, count ) == count ) return true;
        }
    }
    /** \brief 增加引用计数 */
    void incRef() { LongAtomicIncrement(&_uses); }
    /** \brief 减少引用计数.当引用计数为0时销毁资源,并且销毁资源时减少弱引用计数. */
    void decRef()
    {
        if ( LongAtomicDecrement(&_uses) == 0 )
        {
            this->_destroy();
            this->decWRef();
        }
    }

    /** \brief 增加弱引用计数 */
    void incWRef() { LongAtomicIncrement(&_weaks); }
    /** \brief 减少弱引用计数，当弱引用计数为0时销毁删除器场景对象 */
    void decWRef()
    {
        if ( LongAtomicDecrement(&_weaks) == 0 )
        {
            this->_deleteThis();
        }
    }

    /** \brief 资源引用计数 */
    long useCount() const { return (_uses); }

    /** \brief 资源是否已过期 */
    bool expired() const { return ( _uses == 0 ); }

    /** \brief 弱引用计数 */
    long weakCount() const { return (_weaks); }

    DISABLE_OBJECT_COPY(SharedDeleterContext)
};

/** \brief Shared默认删除器场景 */
template < typename _HTy >
class SharedDefaultDeleterContext : public SharedDeleterContext
{
public:
    SharedDefaultDeleterContext( _HTy h ) : _h(h) { }
private:
    virtual void _destroy() { delete _h; }
    virtual void _deleteThis() { delete this; }

    _HTy _h;
};

/** \brief Shared自定义删除器场景 */
template < typename _HTy, typename _Dt >
class SharedCustomDeleterContext : public SharedDeleterContext
{
public:
    SharedCustomDeleterContext( _HTy h, _Dt dt ) : _h(h), _dt(dt) { }
private:
    virtual void _destroy() { _dt(_h); }
    virtual void _deleteThis() { delete this; }

    _HTy _h;
    _Dt _dt;
};

/** \brief 引用计数共享句柄,管理各种资源的自动释放
 *
 *  可以赋值，还能指定一个failVal指示资源分配失败后的句柄值，当不等于这个值时才会创建删除器场景释放资源 */
template < typename _HTy >
class SharedHandle
{
public:
    typedef _HTy HType;

    struct SharedHandleData
    {
        _HTy h;
        SharedDeleterContext * ctx;
        SharedHandleData() : h(0), ctx(0) { }
        SharedHandleData( _HTy h, SharedDeleterContext * ctx ) : h(h), ctx(ctx) { }
    };

///////////////////////////////////////////////////////////////////////////////////////////
    SharedHandle() { }

    SharedHandle( _HTy h, _HTy failVal ) { attachNew( h, failVal ); }

    template < typename _Dt >
    SharedHandle( _HTy h, _HTy failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    template < typename _HTy2 >
    SharedHandle( _HTy2 h, _HTy2 failVal ) { attachNew( h, failVal ); }

    template < typename _HTy2, typename _Dt >
    SharedHandle( _HTy2 h, _HTy2 failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    virtual ~SharedHandle()
    {
        reset();
    }

    SharedHandle( SharedHandle const & other )
    {
        _reset(other);
    }

    SharedHandle & operator = ( SharedHandle const & other )
    {
        _reset(other);
        return *this;
    }

    template < typename _HTy2 >
    SharedHandle( SharedHandle<_HTy2> const & other )
    {
        _reset(other);
    }

    template < typename _HTy2 >
    SharedHandle & operator = ( SharedHandle<_HTy2> const & other )
    {
        _reset(other);
        return *this;
    }

    void attachNew( _HTy h, _HTy failVal )
    {
        this->_reset0( h, ( h != failVal ? new SharedDefaultDeleterContext<_HTy>(h) : 0 ) );
    }

    template < typename _Dt >
    void attachNew( _HTy h, _HTy failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SharedCustomDeleterContext< _HTy, _Dt >( h, dt ) : 0 ) );
    }

    template < typename _HTy2 >
    void attachNew( _HTy2 h, _HTy2 failVal )
    {
        this->_reset0( h, ( h != failVal ? new SharedDefaultDeleterContext<_HTy2>(h) : 0 ) );
    }

    template < typename _HTy2, typename _Dt >
    void attachNew( _HTy2 h, _HTy2 failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SharedCustomDeleterContext< _HTy2, _Dt >( h, dt ) : 0 ) );
    }

    /** \brief 管理一个资源
     *
     *  若资源是从另一个shared::detach()而来，则isIncRef应设为false；\n
     *  若资源是新建而来，则isIncRef应设为false；\n
     *  若资源是从另一个shared窥视而来，则isIncRef应设为true。 */
    void attach( SharedHandleData const & data, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( data.h, data.ctx );
        }
        else
        {
            _reset0( data.h, data.ctx );
        }
    }

    /** \brief 管理一个资源
     *
     *  若资源是新建而来，则isIncRef应设为false；\n
     *  若资源是从另一个shared::detach()而来，则isIncRef应设为false；\n
     *  若资源是从另一个shared窥视而来，则isIncRef应设为true。 */
    template < typename _HTy2 >
    void attach( typename SharedHandle<_HTy2>::SharedHandleData const & data, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( data.h, data.ctx );
        }
        else
        {
            _reset0( data.h, data.ctx );
        }
    }

    /** \brief 管理一个资源
     *
     *  若资源是新建而来，则isIncRef应设为false；\n
     *  若资源是从另一个shared::detach()而来，则isIncRef应设为false；\n
     *  若资源是从另一个shared窥视而来，则isIncRef应设为true。 */
    template < typename _HTy2 >
    void attach( _HTy2 h, SharedDeleterContext * ctx, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( h, ctx );
        }
        else
        {
            _reset0( h, ctx );
        }
    }

    SharedHandleData detach()
    {
        SharedHandleData r = _self;
        _self.h = static_cast<_HTy>(0);
        _self.ctx = 0;
        return r;
    }

    SharedHandleData peek() const
    {
        return _self;
    }

    void reset()
    {
        _reset0( static_cast<_HTy>(0), 0 );
    }

    _HTy get() const { return _self.h; }

    operator bool() const { return _self.ctx != 0; }

    _HTy operator -> ()
    {
        return _self.h;
    }

    _HTy operator -> () const
    {
        return _self.h;
    }

protected:
    /** \brief 减少自身引用计数，管理新资源 */
    template < typename _HTy2 >
    void _reset0( _HTy2 newH, SharedDeleterContext * newCtx )
    {
        if ( _self.ctx )
            _self.ctx->decRef();
        _self.h = newH;
        _self.ctx = newCtx;
    }

    /** \brief 增加另一个shared的资源引用计数，减少自身计数。管理另一个shared的资源 */
    template < typename _HTy2 >
    void _reset( _HTy2 otherH, SharedDeleterContext * otherCtx )
    {
        if ( otherCtx )
            otherCtx->incRef();
        _reset0( otherH, otherCtx );
    }

    /** \brief 增加另一个shared的资源引用计数，减少自身计数。管理另一个shared的资源 */
    template < typename _HTy2 >
    void _reset( SharedHandle<_HTy2> const & other )
    {
        _reset( other._self.h, other._self.ctx );
    }

    SharedHandleData _self;
private:
    template < typename _HTy0 >
    friend class SharedHandle;
    template < typename _HTy0 >
    friend class WeakHandle;
};

template < typename _Ty >
class SharedPointer : public SharedHandle<_Ty*>
{
public:
    typedef SharedHandle<_Ty*> MyBase;
    typedef _Ty Type;

    SharedPointer() { }

    explicit SharedPointer( _Ty* p ) : MyBase( p, (_Ty*)0 ) { }

    template < typename _Dt >
    SharedPointer( _Ty* p, _Dt dt ) : MyBase( p, (_Ty*)0, dt ) { }

    template < typename _Ty2 >
    explicit SharedPointer( _Ty2* p ) : MyBase( p, (_Ty2*)0 ) { }

    template < typename _Ty2, typename _Dt >
    SharedPointer( _Ty2* p, _Dt dt ) : MyBase( p, (_Ty2*)0, dt ) { }

    SharedPointer( SharedPointer const & other )
    {
        this->_reset(other);
    }

    SharedPointer & operator = ( SharedPointer const & other )
    {
        this->_reset(other);
        return *this;
    }

    template < typename _Ty2 >
    SharedPointer( SharedPointer<_Ty2> const & other )
    {
        this->_reset(other);
    }

    template < typename _Ty2 >
    SharedPointer & operator = ( SharedPointer<_Ty2> const & other )
    {
        this->_reset(other);
        return *this;
    }

    void attachNew( _Ty * p )
    {
        MyBase::attachNew( p, (_Ty*)0 );
    }

    template < typename _Dt >
    void attachNew( _Ty * p, _Dt dt )
    {
        MyBase::attachNew( p, (_Ty*)0, dt );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 * p )
    {
        MyBase::attachNew( p, (_Ty2*)0 );
    }

    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 * p, _Dt dt )
    {
        MyBase::attachNew( p, (_Ty2*)0, dt );
    }

    /** \brief 把指针由_Ty转换成_Ty2类型
     *
     *  通常_Ty为基类，_Ty2为派生类，并且_Ty指针所指的是_Ty2类型的对象。
     *  如果转换失败，返回空指针。如果转换成功，引用计数增加。 */
    template < typename _Ty2 >
    SharedPointer<_Ty2> cast()
    {
        SharedPointer<_Ty2> r;
        typename SharedPointer<_Ty2>::HType p = dynamic_cast< typename SharedPointer<_Ty2>::HType >(this->_self.h);
        if ( p != 0 )
            r._reset( p, this->_self.ctx );
        return r;
    }

    /** \brief 把指针由_Ty转换成_Ty2类型
     *
     *  通常_Ty为基类，_Ty2为派生类，并且_Ty指针所指的必须是_Ty2类型的对象，才能使用这个函数。
     *  你必须担保转换成功，否则将产生灾难。 */
    template < typename _Ty2 >
    SharedPointer<_Ty2> ensureCast()
    {
        SharedPointer<_Ty2> r;
        r._reset( static_cast< typename SharedPointer<_Ty2>::HType >(this->_self.h), this->_self.ctx );
        return r;
    }

    template < typename _Ty0 >
    friend class SharedPointer;
    template < typename _Ty0 >
    friend class WeakPointer;
};

/** \brief 弱句柄 */
template < typename _HTy >
class WeakHandle
{
public:
    typedef _HTy HType;

    typedef typename SharedHandle<_HTy>::SharedHandleData WeakHandleData;

    WeakHandle() { }

    virtual ~WeakHandle()
    {
        reset();
    }

    template < typename _HTy2 >
    WeakHandle( SharedHandle<_HTy2> const & other )
    {
        _reset( other._self.h, other._self.ctx );
    }

    template < typename _HTy2 >
    WeakHandle & operator = ( SharedHandle<_HTy2> const & other )
    {
        _reset( other._self.h, other._self.ctx );
        return *this;
    }

    WeakHandle( WeakHandle const & other )
    {
        _reset(other);
    }

    WeakHandle & operator = ( WeakHandle const & other )
    {
        _reset(other);
        return *this;
    }

    template < typename _HTy2 >
    WeakHandle( WeakHandle<_HTy2> const & other )
    {
        _reset(other);
    }

    template < typename _HTy2 >
    WeakHandle & operator = ( WeakHandle<_HTy2> const & other )
    {
        _reset(other);
        return *this;
    }

    void reset()
    {
        _reset( static_cast<_HTy>(0), 0 );
    }

    SharedHandle<_HTy> lock() const
    {
        SharedHandle<_HTy> r;
        if ( !this->_sharedReset(&r) ) { }
        return r;
    }

    /** \brief 是否过期
     *
     *  true表示已过期，false表示尚未过期 */
    bool expired() const { return ( !_self.ctx || _self.ctx->expired() ); }

    operator bool() { return _self.ctx != 0; }
    operator bool() const { return _self.ctx != 0; }

protected:
    /** \brief Shared*PTR* call reset。用于从Weak*PTR*创建Shared*PTR* */
    template < typename _HTy2 >
    bool _sharedReset( SharedHandle<_HTy2> * pSharedHandle ) const
    {
        if ( _self.ctx && _self.ctx->_incRefNz() )
        {
            // 由于之前_incRefNz()已经增加引用计数，因此这里当作新资源看待
            pSharedHandle->_reset0( _self.h, _self.ctx );
            return true;
        }
        return false;
    }

    /** \brief 增加另一个弱引用计数，减少自身弱计数。管理另一个Weak*PTR* */
    template < typename _HTy2 >
    void _reset( _HTy2 otherH, SharedDeleterContext * otherCtx )
    {
        if ( otherCtx != 0 )
            otherCtx->incWRef();
        if ( _self.ctx != 0 )
            _self.ctx->decWRef();
        _self.h = otherH;
        _self.ctx = otherCtx;
    }

    /** \brief 增加另一个弱引用计数，减少自身弱计数。管理另一个Weak*PTR* */
    template < typename _HTy2 >
    void _reset( WeakHandle<_HTy2> const & other )
    {
        _reset( other._self.h, other._self.ctx );
    }

    WeakHandleData _self;

    template < typename _HTy0 >
    friend class WeakHandle;
};

/** \brief 弱指针 */
template < typename _Ty >
class WeakPointer : public WeakHandle<_Ty*>
{
public:
    typedef WeakHandle<_Ty*> MyBase;
    typedef _Ty Type;

    WeakPointer() { }

    template < typename _Ty2 >
    WeakPointer( SharedPointer<_Ty2> const & other )
    {
        this->_reset( other._self.h, other._self.ctx );
    }

    template < typename _Ty2 >
    WeakPointer & operator = ( SharedPointer<_Ty2> const & other )
    {
        this->_reset( other._self.h, other._self.ctx );
        return *this;
    }

    WeakPointer( WeakPointer const & other )
    {
        this->_reset(other);
    }

    WeakPointer & operator = ( WeakPointer const & other )
    {
        this->_reset(other);
        return *this;
    }

    template < typename _Ty2 >
    WeakPointer( WeakPointer<_Ty2> const & other )
    {
        this->_reset(other);
    }

    template < typename _Ty2 >
    WeakPointer & operator = ( WeakPointer<_Ty2> const & other )
    {
        this->_reset(other);
        return *this;
    }

    SharedPointer<_Ty> lock() const
    {
        SharedPointer<_Ty> r;
        if ( !this->_sharedReset(&r) ) { }
        return r;
    }

    template < typename _Ty0 >
    friend class WeakPointer;
};

template < typename _Ty >
inline SimplePointer<_Ty> MakeSimple( _Ty * newObj )
{
    return SimplePointer<_Ty>(newObj);
}

template < typename _Ty, typename _Dt >
inline SimplePointer<_Ty> MakeSimple( _Ty * newObj, _Dt dt )
{
    return SimplePointer<_Ty>( newObj, dt );
}

template < typename _Ty >
inline SharedPointer<_Ty> MakeShared( _Ty * newObj )
{
    return SharedPointer<_Ty>(newObj);
}

template < typename _Ty, typename _Dt >
inline SharedPointer<_Ty> MakeShared( _Ty * newObj, _Dt dt )
{
    return SharedPointer<_Ty>( newObj, dt );
}


} // namespace winux

#endif // __SMARTPTR_HPP__

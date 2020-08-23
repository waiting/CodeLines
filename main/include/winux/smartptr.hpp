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

    void release()
    {
        this->_destroy();
        this->_deleteThis();
    }

    /** \brief 显式删除detach()出来的SimpleDeleterContext对象 */
    void delThis() { this->_deleteThis(); }
};

/** \brief Simple默认删除器场景 */
template < typename _Ty >
class SimpleDefaultDeleterContext : public SimpleDeleterContext
{
public:
    SimpleDefaultDeleterContext( _Ty h ) : _h(h) { }
private:
    virtual void _destroy() { delete _h; }
    virtual void _deleteThis() { delete this; }

    _Ty _h;
};

/** \brief Simple自定义删除器场景 */
template < typename _Ty, typename _Dt >
class SimpleCustomDeleterContext : public SimpleDeleterContext
{
public:
    SimpleCustomDeleterContext( _Ty h, _Dt dt ) : _h(h), _dt(dt) { }
private:
    virtual void _destroy() { _dt(_h); }
    virtual void _deleteThis() { delete this; }

    _Ty _h;
    _Dt _dt;
};

/** \brief 简单句柄类，管理各种资源的自动释放，赋值相当于传递管理权。
 *
 *  如 ptr2 = ptr1; ptr1将管理权传递到ptr2, ptr1失效.
 *  还能指定一个failVal指示资源分配失败后的句柄值，当不等于这个值时才会创建删除器场景释放资源 */
template < typename _Ty >
class SimpleHandle
{
public:
    //typedef SimpleHandle<_Ty> _MyT;
    struct SimpleHandleData
    {
        _Ty h;
        SimpleDeleterContext * ctx;
        SimpleHandleData() : h(0), ctx(0) { }
        SimpleHandleData( _Ty h, SimpleDeleterContext * ctx ) : h(h), ctx(ctx) { }
    };

    SimpleHandle() { }

    SimpleHandle( _Ty h, _Ty failVal ) { attachNew( h, failVal ); }

    template < typename _Dt >
    SimpleHandle( _Ty h, _Ty failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    template < typename _Ty2 >
    SimpleHandle( _Ty2 h, _Ty2 failVal ) { attachNew( h, failVal ); }

    template < typename _Ty2, typename _Dt >
    SimpleHandle( _Ty2 h, _Ty2 failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    virtual ~SimpleHandle()
    {
        if ( _self.ctx )
            _self.ctx->release();
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

    template < typename _Ty2 >
    SimpleHandle( SimpleHandle<_Ty2> const & other )
    {
        _reset(other);
    }
    template < typename _Ty2 >
    SimpleHandle & operator = ( SimpleHandle<_Ty2> const & other )
    {
        _reset(other);
        return *this;
    }

    void attachNew( _Ty h, _Ty failVal )
    {
        this->_reset0( h, ( h != failVal ? new SimpleDefaultDeleterContext<_Ty>(h) : 0 ) );
    }
    template < typename _Dt >
    void attachNew( _Ty h, _Ty failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SimpleCustomDeleterContext< _Ty, _Dt >( h, dt ) : 0 ) );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 h, _Ty2 failVal )
    {
        this->_reset0( h, ( h != failVal ? new SimpleDefaultDeleterContext<_Ty2>(h) : 0 ) );
    }
    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 h, _Ty2 failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SimpleCustomDeleterContext< _Ty2, _Dt >( h, dt ) : 0 ) );
    }

    void attach( SimpleHandleData const & data )
    {
        _reset0( data.h, data.ctx );
    }

    template < typename _Ty2 >
    void attach( typename SimpleHandle<_Ty2>::SimpleHandleData const & data )
    {
        _reset0( data.h, data.ctx );
    }

    template < typename _Ty2 >
    void attach( _Ty2 h, SimpleDeleterContext * ctx )
    {
        _reset0( h, ctx );
    }

    SimpleHandleData detach()
    {
        SimpleHandleData r = _self;
        _self.h = 0;
        _self.ctx = 0;
        return r;
    }

    _Ty get() const { return _self.h; }

    operator bool() { return _self.ctx != 0; }
    operator bool() const { return _self.ctx != 0; }

    /*operator _Ty()
    {
        return _self.h;
    }

    operator _Ty() const
    {
        return _self.h;
    }*/

    _Ty operator -> ()
    {
        return _self.h;
    }

    _Ty operator -> () const
    {
        return _self.h;
    }

protected:
    template < typename _Ty2 >
    void _reset0( _Ty2 newH, SimpleDeleterContext * newCtx )
    {
        if ( _self.ctx )
            _self.ctx->release();
        _self.h = static_cast<_Ty>(newH);
        _self.ctx = newCtx;
    }

    template < typename _Ty2 >
    void _reset( _Ty2 & h, SimpleDeleterContext * & otherCtx )
    {
        _reset0( h, otherCtx );
        h = static_cast<_Ty2>(0);
        otherCtx = 0;
    }

    template < typename _Ty2 >
    void _reset( SimpleHandle<_Ty2> const & other )
    {
        SimpleHandle<_Ty2>& o = const_cast<SimpleHandle<_Ty2>&>(other);
        _reset( o._self.h, o._self.ctx );
    }

private:
    SimpleHandleData _self;
    template < typename _Ty0 >
    friend class SimpleHandle;
};

/** \brief 简单指针 */
template < typename _Ty >
class SimplePointer : public SimpleHandle<_Ty*>
{
public:
    //typedef SimplePointer<_Ty> _MyT;
    typedef SimpleHandle<_Ty*> _MyBase;

    SimplePointer() { }

    explicit SimplePointer( _Ty* p ) : _MyBase( p, (_Ty*)0 ) { }

    template < typename _Dt >
    SimplePointer( _Ty* p, _Dt dt ) : _MyBase( p, (_Ty*)0, dt ) { }

    template < typename _Ty2 >
    explicit SimplePointer( _Ty2* p ) : _MyBase( p, (_Ty2*)0 ) { }

    template < typename _Ty2, typename _Dt >
    SimplePointer( _Ty2* p, _Dt dt ) : _MyBase( p, (_Ty2*)0, dt ) { }

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
        _MyBase::attachNew( p, (_Ty*)0 );
    }

    template < typename _Dt >
    void attachNew( _Ty * p, _Dt dt )
    {
        _MyBase::attachNew( p, (_Ty*)0, dt );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 * p )
    {
        _MyBase::attachNew( p, (_Ty2*)0 );
    }

    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 * p, _Dt dt )
    {
        _MyBase::attachNew( p, (_Ty2*)0, dt );
    }

};

///////////////////////////////////////////////////////////////////////////////////////////

/** \brief 原子化使一个Long型变量+1 */
WINUX_FUNC_DECL(long) LongAtomicIncrement( long volatile * p );
/** \brief 原子化使一个Long型变量-1 */
WINUX_FUNC_DECL(long) LongAtomicDecrement( long volatile * p );

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
    void incRef() { LongAtomicIncrement(&_uses); }
    /** \brief 减少引用计数.当引用计数为0时销毁资源,并且减少弱引用计数. */
    void decRef()
    {
        if ( LongAtomicDecrement(&_uses) == 0 )
        {
            this->_destroy();
            this->decWRef();
        }
    }

    void incWRef() { LongAtomicIncrement(&_weaks); }
    void decWRef()
    {
        if ( LongAtomicDecrement(&_weaks) == 0 )
        {
            this->_deleteThis();
        }
    }

    /** \brief 显式删除detach()出来的_RefCountBase对象 */
    void delThis() { this->_deleteThis(); }

    DISABLE_OBJECT_COPY(SharedDeleterContext)
};

/** \brief Shared默认删除器场景 */
template < typename _Ty >
class SharedDefaultDeleterContext : public SharedDeleterContext
{
public:
    SharedDefaultDeleterContext( _Ty h ) : _h(h) { }
private:
    virtual void _destroy() { delete _h; }
    virtual void _deleteThis() { delete this; }

    _Ty _h;
};

/** \brief Shared自定义删除器场景 */
template < typename _Ty, typename _Dt >
class SharedCustomDeleterContext : public SharedDeleterContext
{
public:
    SharedCustomDeleterContext( _Ty h, _Dt dt ) : _h(h), _dt(dt) { }
private:
    virtual void _destroy() { _dt(_h); }
    virtual void _deleteThis() { delete this; }

    _Ty _h;
    _Dt _dt;
};

/** \brief 引用计数共享句柄,管理各种资源的自动释放
 *
 *  可以赋值，还能指定一个failVal指示资源分配失败后的句柄值，当不等于这个值时才会创建删除器场景释放资源 */
template < typename _Ty >
class SharedHandle
{
public:
    //typedef SharedHandle<_Ty> _MyT;
    struct SharedHandleData
    {
        _Ty h;
        SharedDeleterContext * rc;
        SharedHandleData() : h(0), rc(0) { }
        SharedHandleData( _Ty h, SharedDeleterContext * rc ) : h(h), rc(rc) { }
    };

///////////////////////////////////////////////////////////////////////////////////////////
    SharedHandle() { }

    SharedHandle( _Ty h, _Ty failVal ) { attachNew( h, failVal ); }

    template < typename _Dt >
    SharedHandle( _Ty h, _Ty failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    template < typename _Ty2 >
    SharedHandle( _Ty2 h, _Ty2 failVal ) { attachNew( h, failVal ); }

    template < typename _Ty2, typename _Dt >
    SharedHandle( _Ty2 h, _Ty2 failVal, _Dt dt ) { attachNew( h, failVal, dt ); }

    virtual ~SharedHandle()
    {
        if ( _self.rc )
            _self.rc->decRef();
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

    template < typename _Ty2 >
    SharedHandle( SharedHandle<_Ty2> const & other )
    {
        _reset(other);
    }
    template < typename _Ty2 >
    SharedHandle & operator = ( SharedHandle<_Ty2> const & other )
    {
        _reset(other);
        return *this;
    }

    void attachNew( _Ty h, _Ty failVal )
    {
        this->_reset0( h, ( h != failVal ? new SharedDefaultDeleterContext<_Ty>(h) : 0 ) );
    }
    template < typename _Dt >
    void attachNew( _Ty h, _Ty failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SharedCustomDeleterContext< _Ty, _Dt >( h, dt ) : 0 ) );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 h, _Ty2 failVal )
    {
        this->_reset0( h, ( h != failVal ? new SharedDefaultDeleterContext<_Ty2>(h) : 0 ) );
    }
    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 h, _Ty2 failVal, _Dt dt )
    {
        this->_reset0( h, ( h != failVal ? new SharedCustomDeleterContext< _Ty2, _Dt >( h, dt ) : 0 ) );
    }

    void attach( SharedHandleData const & data, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( data.h, data.rc );
        }
        else
        {
            _reset0( data.h, data.rc );
        }
    }

    template < typename _Ty2 >
    void attach( typename SharedHandle<_Ty2>::SharedHandleData const & data, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( data.h, data.rc );
        }
        else
        {
            _reset0( data.h, data.rc );
        }
    }

    template < typename _Ty2 >
    void attach( _Ty2 h, SharedDeleterContext * rc, bool isIncRef )
    {
        if ( isIncRef )
        {
            _reset( h, rc );
        }
        else
        {
            _reset0( h, rc );
        }
    }

    SharedHandleData detach()
    {
        SharedHandleData r = _self;
        _self.h = 0;
        _self.rc = 0;
        return r;
    }

    _Ty get() const { return _self.h; }

    operator bool() { return _self.rc != 0; }
    operator bool() const { return _self.rc != 0; }

    /*operator _Ty()
    {
        return _self.h;
    }

    operator _Ty() const
    {
        return _self.h;
    }*/

    _Ty operator -> ()
    {
        return _self.h;
    }

    _Ty operator -> () const
    {
        return _self.h;
    }

protected:
    //减少自身引用计数,迎接管理新资源
    template < typename _Ty2 >
    void _reset0( _Ty2 newH, SharedDeleterContext * newRc )
    {
        if ( _self.rc )
            _self.rc->decRef();
        _self.h = static_cast<_Ty>(newH);
        _self.rc = newRc;
    }
    //增加另外一个shared的资源引用计数,减少自身计数. 管理另外一个shared的资源
    template < typename _Ty2 >
    void _reset( _Ty2 h, SharedDeleterContext * otherRc )
    {
        if ( otherRc )
            otherRc->incRef();
        _reset0( h, otherRc );
    }

    template < typename _Ty2 >
    void _reset( SharedHandle<_Ty2> const & other )
    {
        _reset( static_cast<_Ty>(other._self.h), other._self.rc );
    }

private:
    SharedHandleData _self;
    template < typename _Ty0 >
    friend class SharedHandle;
};

template < typename _Ty >
class SharedPointer : public SharedHandle<_Ty*>
{
public:
    //typedef SharedPointer<_Ty> _MyT;
    typedef SharedHandle<_Ty*> _MyBase;

    SharedPointer() { }

    explicit SharedPointer( _Ty* p ) : _MyBase( p, (_Ty*)0 ) { }

    template < typename _Dt >
    SharedPointer( _Ty* p, _Dt dt ) : _MyBase( p, (_Ty*)0, dt ) { }

    template < typename _Ty2 >
    explicit SharedPointer( _Ty2* p ) : _MyBase( p, (_Ty2*)0 ) { }

    template < typename _Ty2, typename _Dt >
    SharedPointer( _Ty2* p, _Dt dt ) : _MyBase( p, (_Ty2*)0, dt ) { }

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
        _MyBase::attachNew( p, (_Ty*)0 );
    }

    template < typename _Dt >
    void attachNew( _Ty * p, _Dt dt )
    {
        _MyBase::attachNew( p, (_Ty*)0, dt );
    }

    template < typename _Ty2 >
    void attachNew( _Ty2 * p )
    {
        _MyBase::attachNew( p, (_Ty2*)0 );
    }

    template < typename _Ty2, typename _Dt >
    void attachNew( _Ty2 * p, _Dt dt )
    {
        _MyBase::attachNew( p, (_Ty2*)0, dt );
    }

};

template < typename _Ty >
inline SimplePointer<_Ty> MakeSimplePointer( _Ty * newObj )
{
    return SimplePointer<_Ty>(newObj);
}

template < typename _Ty >
inline SharedPointer<_Ty> MakeSharedPointer( _Ty * newObj )
{
    return SharedPointer<_Ty>(newObj);
}


} // namespace winux

#endif // __SMARTPTR_HPP__

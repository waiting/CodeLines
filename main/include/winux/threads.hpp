#ifndef __THREADS_HPP__
#define __THREADS_HPP__

namespace winux
{

//=========================================================================================
/** \brief 线程相关错误
 *
 *  可通过getErrCode()取得pthread函数错误返回代码 */
class ThreadSysError : public Error
{
public:
    ThreadSysError( int errCode, AnsiString const & errStr ) throw() : Error( errCode, errStr ) {  }
    /** \brief 获取错误代码 */
    int getErrCode() const throw() { return this->getErrType(); }
};

//=========================================================================================
/** \brief 线程属性 */
class WINUX_DLL ThreadAttr
{
public:
    /** \brief 构造函数，isCreate=false时不创建线程属性，用户手动调用create()创建。 */
    explicit ThreadAttr( bool isCreate = true );
    ~ThreadAttr();

    /** \brief 创建并初始化一个线程属性 */
    void create();

    /** \brief 销毁一个线程属性 */
    void destroy();

public:
    //attributes:

    /** \brief 分离状态类型 */
    enum DetachStateType
    {
        threadCreateJoinable = 0,  ///< 可连接
        threadCreateDetached = 1,  ///< 分离的
    };
    /** \brief 设置分离状态 */
    void setDetachState( DetachStateType detachState = threadCreateJoinable );
    /** \brief 获取分离状态 */
    DetachStateType getDetachState() const;

    void setStackSize( size_t stackSize );
    size_t getStackSize() const;

private:
    MembersWrapper<struct ThreadAttr_Data> _self;
    friend class Thread;

    DISABLE_OBJECT_COPY(ThreadAttr)
};

/** \brief 线程 */
class WINUX_DLL Thread
{
public:
    /** \brief 线程ID */
    class WINUX_DLL ThreadId
    {
    public:
        ThreadId();
        ThreadId( ThreadId const & other );
        ~ThreadId();
        ThreadId & operator = ( ThreadId const & other );
        bool operator == ( ThreadId const & other ) const;
        bool operator != ( ThreadId const & other ) const { return !this->operator == (other); }
        operator bool() const;
    private:
        MembersWrapper<struct ThreadId_Data> _self;

        friend class Thread;
    };

    /** \brief 线程处理函数指针类型 */
    typedef void * (* PFN_ThreadFunc)( void * param );

public:
    /** \brief 默认的线程属性对象 */
    static ThreadAttr const DefaultThreadAttr;

    /** \brief 得到调用者线程的ThreadId */
    static ThreadId Self();

    /** \brief 调用者线程退出 */
    static void Exit( void * exitVal = NULL );

    /** \brief 调用者线程连接指定其他线程
     *
     *  退出状态由返回值或者otherThread.getExitVal()取得 */
    static void * Join( Thread & otherThread );

private:
    // 默认线程处理函数, param被传入Thread对象, 调用thObj->run()
    static void * _ThreadFunc( void * param );

public:
    /** \brief 构造函数1
     *
     *  isStartup=false时不立即创建线程，忽略attr参数，用户手动调用startup()函数创建线程
     *  isStartup=true时立即创建一个线程，以run()作为线程处理函数、attr为线程属性，用户需要派生Thread类重写run()函数以定义自己的处理 */
    explicit Thread( bool isStartup = false, ThreadAttr const & attr = DefaultThreadAttr );

    /** \brief 构造函数2
     *
     * 立即创建一个线程，以startRoutine作为线程处理函数、attr为线程属性 */
    explicit Thread( PFN_ThreadFunc startRoutine, void * param = NULL, ThreadAttr const & attr = DefaultThreadAttr );

    /** \brief 析构函数 */
    virtual ~Thread();

    /** \brief 实际创建一个线程，执行默认的线程处理函数，即调用虚函数run() */
    void startup( ThreadAttr const & attr = DefaultThreadAttr );

    /** \brief 实际创建一个线程，提供你自己的处理函数 */
    void startup( PFN_ThreadFunc startRoutine, void * param, ThreadAttr const & attr = DefaultThreadAttr );

    /** \brief 被调用者线程连接 */
    void * joined();

    /** \brief 使实际线程与对象分离，不再受到对象管控，自然也不能访问对象的成员
     *
     *  当使用默认过程时，由于要调用成员方法run()，所以不能调用此函数 */
    void detach();

public:
    //attributes:

    /** \brief 设置删除器场景以便默认线程函数删除Thread对象自己 */
    void setDeleter( SimpleDeleterContext * deleter = NULL );
    void setDefaultDeleter() { this->setDeleter( new SimpleDefaultDeleterContext<Thread*>(this) ); }

    template < typename _Dt >
    void setCustomDeleter( _Dt fn ) { this->setDeleter( new SimpleCustomDeleterContext<Thread*, _Dt>( this, fn ) ); }

    /** \brief 取得退出值 */
    void * getExitVal() const;
    /** \brief 在虚函数run()中设置退出值 */
    void setExitVal( void * exitVal );

    /** \brief 线程Id */
    ThreadId get() const;

protected:
    /** \brief 线程处理函数，用户需要派生重写此函数 */
    virtual void run();

private:

    MembersWrapper<struct Thread_Data> _self;
    DISABLE_OBJECT_COPY(Thread)
};

//=========================================================================================
/** \brief 互斥量属性 */
class WINUX_DLL MutexAttr
{
public:
    explicit MutexAttr( bool isCreate = true );
    ~MutexAttr();

    /** \brief 创建并初始化 */
    void create();

    /** \brief 销毁 */
    void destroy();

private:
    MembersWrapper<struct MutexAttr_Data> _self;
    friend class Mutex;

    DISABLE_OBJECT_COPY(MutexAttr)
};

/** \brief 互斥量 */
class WINUX_DLL Mutex : public winux::ILockObj
{
public:
    /** \brief 默认互斥量属性 */
    static MutexAttr const DefaultMutexAttr;
public:
    Mutex( bool isCreate = false, MutexAttr const & attr = DefaultMutexAttr );
    ~Mutex();

    /** \brief 创建并初始化 */
    void create( MutexAttr const & attr = DefaultMutexAttr );

    /** \brief 销毁 */
    void destroy();

    bool lock();
    bool tryLock();
    bool unlock();

private:
    MembersWrapper<struct Mutex_Data> _self;
    friend class Condition;

    DISABLE_OBJECT_COPY(Mutex)
};

//=========================================================================================
/** \brief 条件变量属性 */
class WINUX_DLL ConditionAttr
{
public:
    explicit ConditionAttr( bool isCreate = true );
    ~ConditionAttr();

    /** \brief 创建并初始化 */
    void create();

    /** \brief 销毁 */
    void destroy();

private:
    MembersWrapper<struct ConditionAttr_Data> _self;
    friend class Condition;

    DISABLE_OBJECT_COPY(ConditionAttr)
};

/** \brief 条件变量 */
class WINUX_DLL Condition
{
public:
    static ConditionAttr const DefaultConditionAttr;
public:
    Condition( bool isCreate = false, ConditionAttr const & attr = DefaultConditionAttr );
    ~Condition();

    /** \brief 创建并初始化 */
    void create( ConditionAttr const & attr = DefaultConditionAttr );

    /** \brief 销毁 */
    void destroy();

    /** \brief 等待事件被通知
     *
     *  在mutex锁定之间调用，超时返回false。不满足条件请循环wait()。eg: while ( !条件 ) wait(); */
    bool wait( Mutex & mutex, int timeout = -1 );

    /** \brief 等待谓词条件达成
     *
     *  在mutex锁定之间调用，给定一个谓词描述条件达成，条件不达成就循环wait()，直到达成或超时，超时返回pred()。 */
    template < typename _Predicate >
    bool waitUntil( _Predicate pred, Mutex & mutex, int timeout = -1 )
    {
        while ( !pred() )
            if ( !this->wait( mutex, timeout ) )
                return pred();
        return true;
    }

    /** \brief 通知一个正在wait()中的线程醒来 */
    void notify();

    /** \brief 通知所有正在wait()中的线程醒来 */
    void notifyAll();

private:

    MembersWrapper<struct Condition_Data> _self;
    DISABLE_OBJECT_COPY(Condition)
};

//===========================================================================================
/** \brief TLS Key */
class WINUX_DLL TlsKey
{
public:
    explicit TlsKey( void ( * destructor )( void * pv ) = NULL );
    ~TlsKey();

    /** \brief 获取Key值 */
    void * get() const;

private:
    MembersWrapper<struct TlsKey_Data> _self;

    friend class TlsVar;
    DISABLE_OBJECT_COPY(TlsKey)
};

/** \brief TLS Var */
class WINUX_DLL TlsVar
{
public:
    explicit TlsVar( TlsKey & tlsKey );
    ~TlsVar();

    void * get();
    void * get() const;
    void set( void * v );

    template < typename _Ty >
    _Ty get() { return reinterpret_cast<_Ty>( this->get() ); }
    template < typename _Ty >
    _Ty get() const { return reinterpret_cast<_Ty>( this->get() ); }

    template < typename _Ty >
    void set( _Ty v ) { this->set( reinterpret_cast<void*>(v) ); }

    template < typename _Ty >
    _Ty & ref() { return *reinterpret_cast<_Ty*>( this->get() ); }
    template < typename _Ty >
    _Ty const & ref() const { return *reinterpret_cast<_Ty*>( this->get() ); }

private:
    //MembersWrapper<struct TlsVar_Data> _self;
    TlsKey * _tlsKey;
    DISABLE_OBJECT_COPY(TlsVar)
};

}

#endif // __THREADS_HPP__

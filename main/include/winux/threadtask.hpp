#pragma once

// 任务和线程池相关

namespace winux
{

template < typename _Ty >
class Task;
class ThreadPool;

/** \brief 任务数据场景 */
struct TaskCtx
{
    enum TaskStatus
    {
        taskPending, ///< 任务未决
        taskRunning, ///< 任务执行中
        taskStop ///< 任务停止
    };

    Mutex mtxTask; ///< 互斥量保护数据
    Condition cdtTask; ///< 用于判断运行状态
    TaskStatus status; ///< 运行状态
    ThreadPool * pool; ///< 相关线程池
    bool posted; ///< 是否已被投递，只有起始任务才能被投递

    WeakPointer<TaskCtx> weakThis; ///< 自己的弱引用
    SimplePointer<Runable> routineForPool; ///< 投递到线程池的例程

    TaskCtx * prevTask; ///< 上一个任务
    SharedPointer<TaskCtx> nextTask; ///< 下一个任务，执行完本任务后应该投递到任务池中

    /** \brief 获取起始任务 */
    TaskCtx * getStartTask()
    {
        TaskCtx * start = this;
        while ( start->prevTask != nullptr ) start = start->prevTask;
        return start;
    }

    /** \brief 等待任务结束 */
    bool wait( double sec = -1 )
    {
        ScopeGuard guard(mtxTask);
        return cdtTask.waitUntil( [this] () { return this->status == TaskCtx::taskStop; }, mtxTask, sec );
    }

    /** \brief 更新运行状态 */
    void updateStatus( TaskStatus st, bool isNotifyAll = false )
    {
        ScopeGuard guard(mtxTask);
        this->status = st;
        if ( isNotifyAll ) cdtTask.notifyAll(); // 唤醒所有等待此任务的线程
    }

    /** \brief 投入线程池队列中 */
    void post();

    /** \brief 尝试投递后续任务，如果有的话 */
    void tryPostNext();
protected:
    TaskCtx() : mtxTask(true), cdtTask(true), status(taskPending), pool(nullptr), posted(false), prevTask(nullptr) { }
    virtual ~TaskCtx()
    {
        //auto start = getStartTask();
        //cout << "~TaskCtx(" << ( start == this ? "start-task" : "then-task" ) << ") " << this << "\n";
    }
};

template < typename _Ty >
struct TaskCtxT : public TaskCtx
{
    _Ty val;

    template < typename... _ArgType >
    static SharedPointer<TaskCtxT> Create( _ArgType&& ... arg )
    {
        SharedPointer<TaskCtxT> p( new TaskCtxT( std::forward<_ArgType>(arg)... ) );
        p->weakThis = p;
        return p;
    }

    _Ty get()
    {
        this->wait();
        return val;
    }

    void exec( RunableInvoker<_Ty> * ivk ) noexcept
    {
        try
        {
            this->val = ivk->invoke();
        }
        catch ( winux::Error const & e )
        {
            std::cout << e.what() << std::endl;
        }
        catch ( ... )
        {
        }
    }
protected:
    TaskCtxT( ThreadPool * pool, TaskCtx::TaskStatus status = TaskCtx::taskPending )
    {
        this->pool = pool;
        this->status = status;
    }
};

template <>
struct TaskCtxT<void> : public TaskCtx
{
    template < typename... _ArgType >
    static SharedPointer<TaskCtxT> Create( _ArgType&& ... arg )
    {
        SharedPointer<TaskCtxT> p( new TaskCtxT( std::forward<_ArgType>(arg)... ) );
        p->weakThis = p;
        return p;
    }

    void get()
    {
        this->wait();
    }

    void exec( RunableInvoker<void> * ivk ) noexcept
    {
        try
        {
            ivk->invoke();
        }
        catch ( winux::Error const & e )
        {
            std::cout << e.what() << std::endl;
        }
        catch ( ... )
        {
        }
    }
protected:
    TaskCtxT( ThreadPool * pool, TaskCtx::TaskStatus status = TaskCtx::taskPending )
    {
        this->pool = pool;
        this->status = status;
    }
};

/** \brief 线程池，创建一组线程等待着从任务队列中获取任务执行 */
class ThreadPool
{
public:
    /** \brief 构造函数0 */
    explicit ThreadPool() : _mtxPool(true), _cdtPool(true), _poolStop(false)
    {
    }

    /** \brief 构造函数1
     *
     *  \param threadCount 启动的线程数量 */
    explicit ThreadPool( int threadCount ) : _mtxPool(true), _cdtPool(true), _poolStop(false)
    {
        this->startup(threadCount);
    }

    virtual ~ThreadPool()
    {
        this->whenEmptyStopAndWait();
    }

    /** \brief 启动指定数量的线程 */
    ThreadPool & startup( int threadCount )
    {
        _group.create( threadCount, [this] () {
            while ( !_poolStop )
            {
                SharedPointer<TaskCtx> taskCtx;
                {
                    ScopeGuard guard(_mtxPool);
                    while ( _queueTask.empty() && !_poolStop )
                    {
                        _cdtPool.wait(_mtxPool);
                    }
                    if ( !_queueTask.empty() ) // 如果队列不空
                    {
                        taskCtx = _queueTask.front();
                        _queueTask.pop();

                        _cdtPool.notifyAll(); // 唤醒所有线程池线程
                    }
                }

                if ( taskCtx )
                {
                    taskCtx->updateStatus( TaskCtx::taskRunning, false );
                    taskCtx->routineForPool->run();

                    taskCtx->posted = false;
                }
            }
        } );
        _group.startup(); // 启动线程组的线程
        return *this;
    }

    /** \brief 创建一个新任务 */
    template < typename _Fx, typename... _ArgType >
    Task<typename FuncTraits<_Fx>::ReturnType> task( _Fx fn, _ArgType&&... arg )
    {
        return Task<typename FuncTraits<_Fx>::ReturnType>( this, fn, std::forward<_ArgType>(arg)... );
    }

    /** \brief 主动停止线程池运行 */
    void stop()
    {
        ScopeGuard guard(_mtxPool);
        _poolStop = true;
        _cdtPool.notifyAll(); // 唤醒所有线程池线程
    }

    /** \brief wait(sec>0)等待一定的时间长用于等待任务运行。当调用stop()后，wait(sec<0)等待线程组线程全部正常退出 */
    bool wait( double sec = -1 )
    {
        return _group.wait(sec); // 等待全部线程退出
    }

    /** \brief 当任务队列为空，就停止线程池运行，并等待线程组线程正常退出 */
    void whenEmptyStopAndWait()
    {
        if ( true )
        {
            ScopeGuard guard(_mtxPool);
            _cdtPool.waitUntil( [this] () { return _queueTask.empty(); }, _mtxPool );
        }

        this->stop();
        this->wait();
    }

    /** \brief 队列里的任务数 */
    size_t getTaskCount() const
    {
        ScopeGuard guard( const_cast<Mutex &>(_mtxPool) );
        return _queueTask.size();
    }

private:
    // 投递一个任务
    void _postTask( SharedPointer<TaskCtx> taskCtx )
    {
        ScopeGuard guard(_mtxPool);
        _queueTask.push(taskCtx);
        _cdtPool.notify(); // 通知一个等待中的线程
    }

    Mutex _mtxPool;
    Condition _cdtPool; // 用于判断队列是否有数据
    bool _poolStop; // 线程池停止
    ThreadGroup _group;
    std::queue< SharedPointer<TaskCtx> > _queueTask;

    friend struct TaskCtx;

    DISABLE_OBJECT_COPY(ThreadPool)
};

// partial TaskCtx
inline void TaskCtx::post()
{
    if ( !weakThis.expired() )
    {
        SharedPointer<TaskCtx> p = weakThis.lock();
        p->status = taskPending;
        this->pool->_postTask(p);
    }
}

inline void TaskCtx::tryPostNext()
{
    /*SharedPointer<TaskCtx> nextTask;
    {
        ScopeGuard guard(this->mtxTask);
        nextTask = this->nextTask;
    }*/

    // 检测是否有下个任务，投递到线程池
    if ( nextTask )
    {
        nextTask->post();
    }
}

/** \brief 代表投递到线程池的一个任务，用于等待执行完毕获取返回值或者接着投递下一个任务 */
template < typename _Ty >
class Task
{
public:
    using ReturnType = _Ty;

    /** \brief Ctor1 创建一个起始任务，需要提供一个线程池 */
    template < typename _Fx, typename... _ArgType >
    Task( ThreadPool * pool, _Fx fnRoutine, _ArgType&& ... argRoutine )
    {
        static_assert( std::is_same< ReturnType, typename FuncTraits<_Fx>::ReturnType >::value , "FuncTraits<_Fx>::ReturnType is not match Task<_Ty>." );
        _taskCtx = TaskCtxT<ReturnType>::Create(pool, TaskCtx::taskPending);
        //cout << "start-task: " << _taskCtx.get() << endl;

        auto routine = MakeSimple( NewRunable( fnRoutine, std::forward<_ArgType>(argRoutine)... ) );
        _taskCtx->routineForPool.attachNew( NewRunable( [routine] ( TaskCtxT<ReturnType> * taskCtx ) {
            // 执行任务例程
            taskCtx->exec( routine.get() );

            // 检测是否有下个任务，投递到线程池
            taskCtx->tryPostNext();

            // 更新运行状态并通知唤醒等待在此任务的线程
            taskCtx->updateStatus( TaskCtx::taskStop, true );

        }, _taskCtx.get() ) );
    }

    /** \brief Ctor2-1 给一个任务创建一个后续任务 */
    template < typename _Fx, typename... _ArgType >
    Task( SharedPointer< TaskCtxT<void> > prevTaskCtx, _Fx fnRoutine, _ArgType&& ... argRoutine )
    {
        static_assert( std::is_same< ReturnType, typename FuncTraits<_Fx>::ReturnType >::value , "FuncTraits<_Fx>::ReturnType is not match Task<_Ty>." );
        _taskCtx = TaskCtxT<ReturnType>::Create(prevTaskCtx->pool, TaskCtx::taskPending);
        _taskCtx->prevTask = prevTaskCtx.get();
        //cout << "then 2-1 " << endl;

        auto routine = MakeSimple( NewRunable( fnRoutine, std::forward<_ArgType>(argRoutine)... ) );
        _taskCtx->routineForPool.attachNew( NewRunable( [routine] ( SharedPointer< TaskCtxT<void> > prevTaskCtx, TaskCtxT<ReturnType> * taskCtx ) {
            // 执行任务例程
            taskCtx->exec( routine.get() );

            // 后续任务的例程执行完成后没有必要还留有上一个任务的nextTask，必须清空，否则内存泄露。
            prevTaskCtx->nextTask.reset();

            // 检测是否有下个任务，投递到线程池
            taskCtx->tryPostNext();

            // 更新运行状态并通知唤醒等待在此任务的线程
            taskCtx->updateStatus( TaskCtx::taskStop, true );

        }, prevTaskCtx, _taskCtx.get() ) );

    }

    /** \brief Ctor2-2 给一个任务创建一个后续任务 - 类方法执行 */
    template < typename _Fx, typename... _ArgType >
    Task( SharedPointer< TaskCtxT<void> > prevTaskCtx, _Fx fnRoutine, typename FuncTraits<_Fx>::ClassType * obj, _ArgType&& ... argRoutine )
    {
        static_assert( std::is_same< ReturnType, typename FuncTraits<_Fx>::ReturnType >::value , "FuncTraits<_Fx>::ReturnType is not match Task<_Ty>." );
        _taskCtx = TaskCtxT<ReturnType>::Create(prevTaskCtx->pool, TaskCtx::taskPending);
        _taskCtx->prevTask = prevTaskCtx.get();
        //cout << "then 2-2 " << endl;

        auto routine = MakeSimple( NewRunable( fnRoutine, obj, std::forward<_ArgType>(argRoutine)... ) );
        _taskCtx->routineForPool.attachNew( NewRunable( [routine] ( SharedPointer< TaskCtxT<void> > prevTaskCtx, TaskCtxT<ReturnType> * taskCtx ) {
            // 执行任务例程
            taskCtx->exec( routine.get() );

            // 后续任务的例程执行完成后没有必要还留有上一个任务的nextTask，必须清空，否则内存泄露。
            prevTaskCtx->nextTask.reset();

            // 检测是否有下个任务，投递到线程池
            taskCtx->tryPostNext();

            // 更新运行状态并通知唤醒等待在此任务的线程
            taskCtx->updateStatus( TaskCtx::taskStop, true );

        }, prevTaskCtx, _taskCtx.get() ) );

    }

    /** \brief Ctor3-1 给一个任务创建一个后续任务，并把上一个任务返回值移动给后续任务 */
    template < typename _Ty2, typename _Fx, typename... _ArgType >
    Task( SharedPointer< TaskCtxT<_Ty2> > prevTaskCtx, _Fx fnRoutine, _ArgType&& ... argRoutine )
    {
        static_assert( std::is_same< ReturnType, typename FuncTraits<_Fx>::ReturnType >::value , "FuncTraits<_Fx>::ReturnType is not match Task<_Ty>." );
        _taskCtx = TaskCtxT<ReturnType>::Create(prevTaskCtx->pool, TaskCtx::taskPending);
        _taskCtx->prevTask = prevTaskCtx.get();
        //cout << "then 3-1 " << endl;

        auto routine = MakeSimple( NewRunable( fnRoutine, _Ty2(), std::forward<_ArgType>(argRoutine)... ) );
        _taskCtx->routineForPool.attachNew( NewRunable( [routine] ( SharedPointer< TaskCtxT<_Ty2> > prevTaskCtx, TaskCtxT<ReturnType> * taskCtx ) {
            // 执行任务例程
            std::get<0>(routine->_tuple) = std::move(prevTaskCtx->val);
            taskCtx->exec( routine.get() );

            // 后续任务的例程执行完成后没有必要还留有上一个任务的nextTask，必须清空，否则内存泄露。
            prevTaskCtx->nextTask.reset();

            // 检测是否有下个任务，投递到线程池
            taskCtx->tryPostNext();

            // 更新运行状态并通知唤醒等待在此任务的线程
            taskCtx->updateStatus( TaskCtx::taskStop, true );

        }, prevTaskCtx, _taskCtx.get() ) );

    }

    /** \brief Ctor3-2 给一个任务创建一个后续任务，并把上一个任务返回值移动给后续任务 - 类方法执行 */
    template < typename _Ty2, typename _Fx, typename... _ArgType >
    Task( SharedPointer< TaskCtxT<_Ty2> > prevTaskCtx, _Fx fnRoutine, typename FuncTraits<_Fx>::ClassType * obj, _ArgType&& ... argRoutine )
    {
        static_assert( std::is_same< ReturnType, typename FuncTraits<_Fx>::ReturnType >::value , "FuncTraits<_Fx>::ReturnType is not match Task<_Ty>." );
        _taskCtx = TaskCtxT<ReturnType>::Create(prevTaskCtx->pool, TaskCtx::taskPending);
        _taskCtx->prevTask = prevTaskCtx.get();
        //cout << "then 3-2 " << endl;

        auto routine = MakeSimple( NewRunable( fnRoutine, obj, _Ty2(), std::forward<_ArgType>(argRoutine)... ) );
        _taskCtx->routineForPool.attachNew( NewRunable( [routine] ( SharedPointer< TaskCtxT<_Ty2> > prevTaskCtx, TaskCtxT<ReturnType> * taskCtx ) {
            // 执行任务例程
            std::get<1>(routine->_tuple) = std::move(prevTaskCtx->val);
            taskCtx->exec( routine.get() );

            // 后续任务的例程执行完成后没有必要还留有上一个任务的nextTask，必须清空，否则内存泄露。
            prevTaskCtx->nextTask.reset();

            // 检测是否有下个任务，投递到线程池
            taskCtx->tryPostNext();

            // 更新运行状态并通知唤醒等待在此任务的线程
            taskCtx->updateStatus( TaskCtx::taskStop, true );

        }, prevTaskCtx, _taskCtx.get() ) );

    }

    virtual ~Task()
    {
    }

    /** \brief 创建一个后续任务 */
    template < typename _Fx, typename... _ArgType >
    Task<typename FuncTraits<_Fx>::ReturnType> then( _Fx fn, _ArgType&& ... arg )
    {
        return Task<typename FuncTraits<_Fx>::ReturnType>( _taskCtx, fn, std::forward<_ArgType>(arg)... );
    }

    /** \brief 任务必须投递，否则不会被执行 */
    Task & post()
    {
        // 寻到起始任务
        TaskCtx * startTask = _taskCtx->getStartTask();
        if ( !startTask->posted )
        {
            TaskCtx * cur = nullptr;

            // 修改任务状态为taskPending
            cur = _taskCtx.get();
            while ( cur != nullptr )
            {
                ScopeGuard guard(cur->mtxTask);
                cur->status = TaskCtx::taskPending;
                cur = cur->prevTask;
            }

            // 用prev链构建next链
            cur = _taskCtx.get();
            while ( cur->prevTask != nullptr )
            {
                auto p = cur;
                cur = cur->prevTask;
                cur->nextTask = p->weakThis.lock();
            }

            // 投递起始任务
            startTask->post();

            startTask->posted = true;
        }

        return *this;
    }

    /** \brief 等待任务执行完毕 */
    void wait( double sec = -1 )
    {
        _taskCtx->wait(sec);
    }

    /** \brief 等待任务结束并获取返回值 */
    ReturnType get()
    {
        return _taskCtx->get();
    }

private:
    SharedPointer< TaskCtxT<ReturnType> > _taskCtx; ///< 任务数据场景

    template < typename _Ty0 >
    friend class Task;
};

} // namespace winux

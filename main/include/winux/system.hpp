#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

namespace winux
{
#if defined(OS_WIN)
    typedef HANDLE HPipe;
    typedef HANDLE HProcess;
#else
    typedef int HPipe;
    typedef pid_t HProcess;
#endif

#if defined(_UNICODE) || defined(UNICODE)
    #define CommandLineToArgv CommandLineToArgvW
#else
    #define CommandLineToArgv CommandLineToArgvA
#endif

/** \brief 把命令行解析成Argv数组。不支持命令行& && | ||
 *
 *  \param cmd 命令行，不支持命令行& && | ||
 *  \param argv 输出解析到的参数
 *  \return 解析到的参数个数 */
WINUX_FUNC_DECL(size_t) CommandLineToArgvA( AnsiString const & cmd, AnsiStringArray * argv );
/** \brief 把命令行解析成Argv数组。不支持命令行& && | ||
 *
 *  \param cmd 命令行，不支持命令行& && | ||
 *  \param argv 输出解析到的参数
 *  \return 解析到的参数个数 */
WINUX_FUNC_DECL(size_t) CommandLineToArgvW( UnicodeString const & cmd, UnicodeStringArray * argv );

/** \brief 新建子进程执行指定命令，并用管道重定向了标准设备
 *
 *  \param cmd winux::String const &
 *  \param hStdinWritePipe HPipe *
 *  \param hStdoutReadPipe HPipe *
 *  \param hStderrReadPipe HPipe *
 *  \param closeStdinIfStdinWritePipeIsNull bool 指示当不准备重定向标准输入时是否关闭它，这样子进程就不会等待输入而卡死
 *  \return HProcess 子进程的进程句柄 */
WINUX_FUNC_DECL(HProcess) ExecCommandEx(
    winux::String const & cmd,
    HPipe * hStdinWritePipe,
    HPipe * hStdoutReadPipe,
    HPipe * hStderrReadPipe = NULL,
    bool closeStdinIfStdinWritePipeIsNull = true
);

/** \brief 新建子进程执行指定命令，等待子进程结束，并把字符串重定向了标准设备
 *
 *  \param cmd winux::String const &
 *  \param stdinStr winux::String const &
 *  \param stdoutStr winux::String *
 *  \param stderrStr winux::String *
 *  \param closeStdinIfStdinStrEmpty bool
 *  \return int */
WINUX_FUNC_DECL(int) ExecCommand(
    winux::String const & cmd,
    winux::String const & stdinStr,
    winux::String * stdoutStr,
    winux::String * stderrStr = NULL,
    bool closeStdinIfStdinStrEmpty = true
);

/** \brief 执行命令，返回标准输出内容 */
WINUX_FUNC_DECL(winux::String) GetExec(
    winux::String const & cmd,
    winux::String const & stdinStr = "",
    winux::String * stderrStr = NULL,
    bool closeStdinIfStdinStrEmpty = true
);



///////////////////////////////////////////////////////////////////////////////////////////

/** \brief 命令行变量解析器
 *
 * 该解析器定义了四种变量\n
 *  - 参数(Params)：由‘前缀字符+名字’和‘下一个命令行值’组成。例如：/file abc.cpp\n
 *  - 选项(Options)：由‘前缀字符+名字’‘= or :’‘选项值’组成。例如： --std=c99 /Fo:abc_dir\n
 *  - 旗标(Flags)：由‘前缀字符+名字’组成。例如：--exe --cpp /DLL\n
 *  - 值：剩余未能识别的都被当成值 */
class WINUX_DLL CommandLineVars
{
public:
    /** \brief 构造函数
     *
     *  \param [in] argc 命令行参数个数，可由main()参数传入
     *  \param [in] argv 命令行参数，可由main()参数传入
     *  \param [in] desiredParams 要识别的参数名（逗号分割的String类型、或者Array类型）
     *  \param [in] desiredOptions 要识别的选项名（逗号分割的String类型、或者Array类型）
     *  \param [in] desiredFlags 要识别的旗标名（逗号分割的String类型、或者Array类型）
     *  \param [in] optionSymbols 选项赋值符号（逗号分割的String类型、或者Array类型）。默认是'='和':' */
    CommandLineVars(
        int argc,
        char const ** argv,
        Mixed const & desiredParams,
        Mixed const & desiredOptions,
        Mixed const & desiredFlags,
        Mixed const & optionSymbols = "=,:"
    );

    /** \brief 获取参数个数 */
    size_t getParamsCount() const { return _params.getCount(); }
    /** \brief 获取选项个数 */
    size_t getOptionsCount() const { return _options.getCount(); }
    /** \brief 获取旗标个数 */
    size_t getFlagsCount() const { return _flags.getCount(); }
    /** \brief 获取值个数 */
    size_t getValuesCount() const { return _values.getCount(); }

    /** \brief 是否有此参数 */
    bool hasParam( String const & name ) const { return _params.has(name); }
    /** \brief 是否有此选项 */
    bool hasOption( String const & name ) const { return _options.has(name); }
    /** \brief 是否有此旗标 */
    bool hasFlag( String const & name ) const { return _flags.has(name); }
    /** \brief 是否有此值 */
    bool hasValue( String const & value ) const { return _values.has(value); }

    /** \brief 获取指定名字的参数 */
    Mixed const & getParam( String const & name, Mixed const & defValue = "" ) const { return this->hasParam(name) ? _params[name] : defValue; }
    /** \brief 获取指定名字的选项 */
    Mixed const & getOption( String const & name, Mixed const & defValue = "" ) const { return this->hasOption(name) ? _options[name] : defValue; }
    /** \brief 获取指定索引的旗标 */
    Mixed const & getFlag( size_t i ) const { return _flags[i]; }
    /** \brief 获取指定索引的值 */
    Mixed const & getValue( size_t i ) const { return _values[i]; }

    /** \brief 获取指定参数在argv中的索引 */
    size_t getParamIndexInArgv( String const & name ) const { return _paramIndexesInArgv.find(name) != _paramIndexesInArgv.end() ? _paramIndexesInArgv.at(name) : npos; }
    /** \brief 获取指定选项在argv中的索引 */
    size_t getOptionIndexInArgv( String const & name ) const { return _optionIndexesInArgv.find(name) != _optionIndexesInArgv.end() ? _optionIndexesInArgv.at(name) : npos; }
    /** \brief 获取指定旗标在argv中的索引 */
    size_t getFlagIndexInArgv( String const & name ) const { return _flagIndexesInArgv.find(name) != _flagIndexesInArgv.end() ? _flagIndexesInArgv.at(name) : npos; }
    /** \brief 获取指定值在argv中的索引 */
    size_t getValueIndexInArgv( String const & value ) const { return _valueIndexesInArgv.find(value) != _valueIndexesInArgv.end() ? _valueIndexesInArgv.at(value) : npos; }

    /** \brief 获取全部参数 */
    Mixed & getParams() { return _params; }
    /** \brief 获取全部选项 */
    Mixed & getOptions() { return _options; }
    /** \brief 获取全部旗标 */
    Mixed & getFlags() { return _flags; }
    /** \brief 获取全部值 */
    Mixed & getValues() { return _values; }

    /** \brief 倾泻全部 */
    Mixed dump() const
    {
        CommandLineVars * p = const_cast<CommandLineVars *>(this);
        return $c{
            { "params", p->getParams() },
            { "options", p->getOptions() },
            { "flags", p->getFlags() },
            { "values", p->getValues() },
        };
    }

    /** \brief 获取argc */
    int getArgc() const { return _argc; }
    /** \brief 获取argv */
    char const ** getArgv() const { return _argv; }

private:
    int _argc; // main()命令行参数个数
    char const ** _argv; // main()命令行参数

    StringArray _desiredParams; // 要识别的参数名
    StringArray _desiredOptions; // 要识别的选项名
    StringArray _desiredFlags; // 要识别的旗标名
    StringArray _optionSymbols; // 选项赋值符号

    Mixed _params;  // 参数Collection
    std::map< String, size_t > _paramIndexesInArgv;    // 参数在argv中的索引
    Mixed _options; // 选项Collection
    std::map< String, size_t > _optionIndexesInArgv;   // 选项在argv中的索引
    Mixed _flags;   // 旗标Array
    std::map< String, size_t > _flagIndexesInArgv;     // 旗标在argv中的索引
    Mixed _values;  // 值Array
    std::map< String, size_t > _valueIndexesInArgv;    // 值在argv中的索引

    DISABLE_OBJECT_COPY(CommandLineVars)
};

/** \brief 命令行参数解析(OldVersion)
 *
 *  规则是：
 *  含有前缀的串叫作name，后跟着不含前缀的串叫作value，一对name/value叫作Param
 *  若未出现name，先出现了value，则作为noNameParam
 *  若出现了name，接着又出现另一个name，则前一个name作为noValueParam
 *  name必须包含前缀
 *  noNameParam也被叫Value参数, noValueParam也被叫Flag参数, 这样具有逻辑性的命名方便使用 */
class WINUX_DLL CommandLine
{
    StringStringMap _params;
    StringArray _values;
    StringArray _flags;
    static bool _IsName( String const & arg, StringArray const & prefixes );
public:
    CommandLine( int argc, char * argv[], String const & paramPrefix = "- -- /" );
    size_t getParamCount() const { return _params.size(); }
    size_t getValueCount() const { return _values.size(); }
    size_t getFlagCount() const { return _flags.size(); }
    bool hasParam( String const & name ) const { return isset( _params, name ); }
    bool hasValue( String const & value ) const { return std::find( _values.begin(), _values.end(), value ) != _values.end(); }
    bool hasFlag( String const & name ) const { return std::find( _flags.begin(), _flags.end(), name ) != _flags.end(); }
    String getParam( String const & name, String const & defValue = "" ) const { return isset( _params, name ) ? _params.at(name) : defValue; }
    String getValue( int i ) const { return _values[i]; }
    String getFlag( int i ) const { return _flags[i]; }

    DISABLE_OBJECT_COPY(CommandLine)
};

/** \brief 同步锁对象接口 */
class WINUX_DLL ILockObj
{
public:
    virtual ~ILockObj() { }
    virtual bool tryLock() = 0;
    virtual bool lock() = 0;
    virtual bool unlock() = 0;
};

/** \brief 作用域范围保护 */
class WINUX_DLL ScopeGuard
{
    ILockObj & _lockObj;
public:
    ScopeGuard( ILockObj & lockObj ) : _lockObj(lockObj)
    {
        _lockObj.lock();
    }
    ~ScopeGuard()
    {
        _lockObj.unlock();
    }
    DISABLE_OBJECT_COPY(ScopeGuard)
};

/** \brief 互斥锁
 *
 *  Windows平台用win32api实现，Linux用pthread实现 */
class WINUX_DLL MutexLockObj : public ILockObj
{
public:
    MutexLockObj();
    virtual ~MutexLockObj();
    virtual bool tryLock();
    virtual bool lock();
    virtual bool unlock();
private:
    MembersWrapper<struct MutexLockObj_Data> _self;

    DISABLE_OBJECT_COPY(MutexLockObj)
};

/** \brief Dll加载器错误 */
class DllLoaderError : public Error
{
public:
    enum {
        DllLoader_FuncNotFound = 0x00000100,    //!< 函数未找到
        DllLoader_ModuleNoLoaded                //!< 模块没加载
    };

    DllLoaderError( int errType, AnsiString const & errStr ) throw() : Error( errType, errStr ) { }
};

/** \brief DLL动态载入器 */
class WINUX_DLL DllLoader
{
public:

#if defined(OS_WIN)
    typedef HMODULE ModuleHandle;
#else
    typedef void * ModuleHandle;
#endif
    /** \brief 取得模块路径（含模块文件名）
     *
     *  \param funcInModule 模块中某个函数的地址 */
    static String GetModulePath( void * funcInModule );

    DllLoader();
    DllLoader( String const & dllName );

    ~DllLoader();

    operator bool() const { return _hDllModule != NULL; }
    operator ModuleHandle() const { return _hDllModule; }

    /** \brief 错误信息 */
    char const * errStr() const;

    /** \brief Dll函数动态调用 */
    template < typename _PfnType >
    class Function
    {
    public:
        typedef _PfnType PfnType;
    private:
        AnsiString _funcName;
        PfnType _pfn;
    public:
        Function() : _funcName(""), _pfn(0) { }
        Function( AnsiString const & funcName, PfnType pfn ) : _funcName(funcName), _pfn(pfn) { }

        operator bool() const { return _pfn != NULL; }

        AnsiString const & getFuncName() const { return _funcName; }
        void * get() const { return reinterpret_cast<void *>(_pfn); }

        template < typename... _ArgType >
        typename winux::FuncTraits<PfnType>::ReturnType call( _ArgType&& ... arg )
        {
            if ( !_pfn ) throw DllLoaderError( DllLoaderError::DllLoader_FuncNotFound, _funcName + " is not found" );
            return (*_pfn)( std::forward<_ArgType>(arg)... );
        }
    };

    /** \brief 获取指定名字的函数地址 */
    void (* funcAddr( AnsiString const & funcName ) )();

    /** \brief 获取指定名字的Function对象，失败抛异常 */
    template < typename _PfnType >
    Function<_PfnType> func( AnsiString const & funcName )
    {
        _PfnType pfn = (_PfnType)this->funcAddr(funcName);
        return Function<_PfnType>( funcName, pfn );
    }

    winux::String dllModuleFile; //!< DLL模块文件
private:
    ModuleHandle _hDllModule;

    DISABLE_OBJECT_COPY(DllLoader)
};


} // namespace winux

#endif // __SYSTEM_HPP__

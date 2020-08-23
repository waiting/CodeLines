#if _MSC_VER > 1 && _MSC_VER < 1201
#pragma warning( disable: 4786 )
#endif

#include "utilities.hpp"
#include "system.hpp"
#include "strings.hpp"
#include "smartptr.hpp"
#include "filesys.hpp"

#if defined(__GNUC__) || defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

#if defined(__GNUC__) && !defined(WIN32)
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include <dlfcn.h>
#endif

namespace winux
{

inline static bool IsSpace( char ch )
{
    return ch > '\0' && ch <= ' ';
}

static void __ParseCommandLineString( winux::String const & cmd, winux::String::size_type * pI, winux::String * str )
{
    winux::String::size_type & i = *pI;
    i++; // skip left "

    winux::uint slashes = 0;
    while ( i < cmd.length() )
    {
        winux::String::value_type ch = cmd[i];

        if ( ch == '\"' )
        {
            if ( slashes % 2 ) // 奇数个'\\'
            {
                if ( slashes > 1 )
                {
                    *str += winux::String( slashes / 2, '\\' );
                }
                slashes = 0;
                *str += ch;
                i++;
            }
            else // 偶数个
            {
                if ( slashes > 0 )
                {
                    *str += winux::String( slashes / 2, '\\' );
                }
                slashes = 0;
                i++; // skip right "
                break;
            }
        }
        else if ( ch == '\\' )
        {
            slashes++;
            i++;
        }
        else
        {
            if ( slashes )
            {
                *str += winux::String( slashes, '\\' );
                slashes = 0;
            }
            *str += ch;
            i++;
        }
    }
}

WINUX_FUNC_IMPL(int) CommandLineToArgv( winux::String const & cmd, winux::StringArray * argv )
{
    winux::String::size_type i;
    i = 0;
    winux::String arg;
    winux::uint slashes = 0;
    while ( i < cmd.length() )
    {
        winux::String::value_type ch = cmd[i];
        if ( IsSpace(ch) )
        {
            i++;
            while ( i < cmd.length() && IsSpace(cmd[i]) ) i++; // skip more spaces

            if ( slashes )
            {
                arg += winux::String( slashes, '\\' );
                slashes = 0;
            }
            if ( !arg.empty() ) argv->push_back(arg);
            arg.clear();
        }
        else if ( ch == '^' )
        {
            i++;
            if ( i < cmd.length() && cmd[i] == '^' )
            {
                arg += cmd[i];
                i++;
            }
        }
        else if ( ch == '\"' )
        {
            if ( slashes % 2 ) // 奇数个'\\'
            {
                if ( slashes > 1 )
                {
                    arg += winux::String( slashes / 2, '\\' );
                }
                slashes = 0;
                arg += ch;
                i++;
            }
            else // 偶数个
            {
                if ( slashes > 0 )
                {
                    arg += winux::String( slashes / 2, '\\' );
                }
                slashes = 0;
                // 执行字符串解析
                __ParseCommandLineString( cmd, &i, &arg );
            }
        }
        else if ( ch == '\\' )
        {
            slashes++;
            i++;
        }
        else
        {
            if ( slashes )
            {
                arg += winux::String( slashes, '\\' );
                slashes = 0;
            }
            arg += ch;
            i++;
        }
    }

    if ( i == cmd.length() )
    {
        if ( slashes )
        {
            arg += winux::String( slashes, '\\' );
            slashes = 0;
        }
        if ( !arg.empty() ) argv->push_back(arg);
    }

    return (int)argv->size();
}

#if defined(_MSC_VER) || defined(WIN32)

WINUX_FUNC_IMPL(HProcess) ExecCommandEx(
    winux::String const & cmd,
    HPipe * hStdinWritePipe,
    HPipe * hStdoutReadPipe,
    HPipe * hStderrReadPipe,
    bool closeStdinIfStdinWritePipeIsNull
)
{
    HPipe hStdinPipe[2];
    HPipe hStdoutPipe[2];
    HPipe hStderrPipe[2];
    bool hasStdinPipe = false;
    bool hasStdoutPipe = false;
    bool hasStderrPipe = false;

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    String cmdBuf = cmd;
    BOOL bCreateRet;
    DWORD status = 0;

    if ( hStdinWritePipe != NULL )
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        // stdin的管道不能两端都给子进程继承，只能给读端给子进程，写端不能给，否则子进程cin无法结束。
        sa.bInheritHandle = FALSE;

        HANDLE hStdinPipe0;
        if ( !CreatePipe( &hStdinPipe0, hStdinPipe + 1, &sa, 0 ) ) goto StdPipeError;
        // 把stdin读端的管道让子进程继承
        DuplicateHandle( GetCurrentProcess(), hStdinPipe0, GetCurrentProcess(), hStdinPipe + 0, 0, TRUE, DUPLICATE_SAME_ACCESS );
        // 关闭不可继承的stdin读端管道句柄
        CloseHandle(hStdinPipe0);

        hasStdinPipe = true;
    }
    if ( hStdoutReadPipe != NULL )
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        if ( !CreatePipe( hStdoutPipe + 0, hStdoutPipe + 1, &sa, 0 ) ) goto StdPipeError;
        hasStdoutPipe = true;
    }
    if ( hStderrReadPipe != NULL )
    {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        if ( !CreatePipe( hStderrPipe + 0, hStderrPipe + 1, &sa, 0 ) ) goto StdPipeError;
        hasStderrPipe = true;
    }

    si.dwFlags = STARTF_USESTDHANDLES;

    if ( hStdinWritePipe != NULL )
    {
        si.hStdInput = hStdinPipe[0];
        *hStdinWritePipe = hStdinPipe[1];
    }
    else
    {
        if ( !closeStdinIfStdinWritePipeIsNull )
        {
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }
    }

    if ( hStdoutReadPipe != NULL )
    {
        si.hStdOutput = hStdoutPipe[1];
        *hStdoutReadPipe = hStdoutPipe[0];
    }
    else
    {
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ( hStderrReadPipe != NULL )
    {
        si.hStdError = hStderrPipe[1];
        *hStderrReadPipe = hStderrPipe[0];
    }
    else
    {
        if ( hStdoutReadPipe != NULL )
        {
            si.hStdError = si.hStdOutput;
        }
        else
        {
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        }
    }

    bCreateRet = CreateProcess( NULL, ( cmdBuf.empty() ? (LPSTR)"": &cmdBuf[0] ), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );
    if ( !bCreateRet )
    {
        goto StdPipeError;
    }
    CloseHandle(pi.hThread);

    // 关闭子进程已经继承的句柄
    if ( hStdinWritePipe != NULL )
    {
        CloseHandle(hStdinPipe[0]);
    }
    if ( hStdoutReadPipe != NULL )
    {
        CloseHandle(hStdoutPipe[1]);
    }
    if ( hStderrReadPipe != NULL )
    {
        CloseHandle(hStderrPipe[1]);
    }

    return pi.hProcess;

StdPipeError:
    if ( hasStdinPipe )
    {
        CloseHandle( hStdinPipe[0] );
        CloseHandle( hStdinPipe[1] );
    }
    if ( hasStdoutPipe )
    {
        CloseHandle( hStdoutPipe[0] );
        CloseHandle( hStdoutPipe[1] );
    }
    if ( hasStderrPipe )
    {
        CloseHandle( hStderrPipe[0] );
        CloseHandle( hStderrPipe[1] );
    }

    return INVALID_HANDLE_VALUE;
}

WINUX_FUNC_IMPL(int) ExecCommand(
    winux::String const & cmd,
    winux::String const & stdinStr,
    winux::String * stdoutStr,
    winux::String * stderrStr,
    bool closeStdinIfStdinStrEmpty
)
{
    HPipe hStdinWrite = INVALID_HANDLE_VALUE, hStdoutRead = INVALID_HANDLE_VALUE, hStderrRead = INVALID_HANDLE_VALUE;
    HProcess hChildProcess;
    DWORD status = 0;
    hChildProcess = ExecCommandEx(
        cmd,
        ( stdinStr.empty() ? NULL : &hStdinWrite ),
        ( stdoutStr ? &hStdoutRead : NULL ),
        ( stderrStr ? &hStderrRead : NULL ),
        closeStdinIfStdinStrEmpty
    );
    if ( hChildProcess == INVALID_HANDLE_VALUE )
    {
        return -1;
    }

    if ( !stdinStr.empty() )
    {
        DWORD bytes;
        WriteFile( hStdinWrite, stdinStr.c_str(), stdinStr.length(), &bytes, NULL );
        CloseHandle(hStdinWrite);
    }
    if ( stdoutStr != NULL )
    {
        DWORD bytes = 0;
        int const nSize = 4096;
        char buf[nSize];
        BOOL b;
        stdoutStr->reserve(nSize);
        while ( ( b = ReadFile( hStdoutRead, buf, nSize, &bytes, NULL ) ) )
        {
            if ( bytes == 0 ) break;
            stdoutStr->append( buf, bytes );
        }
        CloseHandle(hStdoutRead);
    }
    if ( stderrStr != NULL )
    {
        DWORD bytes = 0;
        int const nSize = 4096;
        char buf[nSize];
        BOOL b;
        stderrStr->reserve(nSize);
        while ( ( b = ReadFile( hStderrRead, buf, nSize, &bytes, NULL ) ) )
        {
            if ( bytes == 0 ) break;
            stderrStr->append( buf, bytes );
        }
        CloseHandle(hStderrRead);
    }

    WaitForSingleObject( hChildProcess, INFINITE );
    GetExitCodeProcess( hChildProcess, &status );
    CloseHandle(hChildProcess);
    return status;
}

WINUX_FUNC_IMPL(winux::String) GetExec(
    winux::String const & cmd,
    winux::String const & stdinStr,
    winux::String * stderrStr,
    bool closeStdinIfStdinStrEmpty
)
{
    int rc;
    winux::String stdoutStr;
    rc = ExecCommand( cmd, stdinStr, &stdoutStr, stderrStr, closeStdinIfStdinStrEmpty );
    return stdoutStr;
}

#else

WINUX_FUNC_IMPL(HProcess) ExecCommandEx(
    winux::String const & cmd,
    HPipe * hStdinWritePipe,
    HPipe * hStdoutReadPipe,
    HPipe * hStderrReadPipe,
    bool closeStdinIfStdinWritePipeIsNull
)
{
    int stdinFds[2];
    int stdoutFds[2];
    int stderrFds[2];
    if ( hStdinWritePipe != NULL )
    {
        pipe(stdinFds);
    }
    if ( hStdoutReadPipe != NULL )
    {
        pipe(stdoutFds);
    }
    if ( hStderrReadPipe != NULL )
    {
        pipe(stderrFds);
    }

    // fork 子进程
    pid_t id = fork();
    if ( id < 0 ) // occur an error
    {
        return id;
    }
    else if ( id > 0 ) // This is the parent process. The `id` represents a child process id when `id` > 0.
    {
        if ( hStdinWritePipe != NULL )
        {
            close(stdinFds[0]); // 父进程不需要stdin读取端

            *hStdinWritePipe = stdinFds[1];
        }

        if ( hStdoutReadPipe != NULL )
        {
            close(stdoutFds[1]); // 父进程不需要stdout写入端

            *hStdoutReadPipe = stdoutFds[0];
        }

        if ( hStderrReadPipe != NULL )
        {
            close(stderrFds[1]); // 父进程不需要stderr写入端

            *hStderrReadPipe = stderrFds[0];
        }

        //int status = 0;
        //waitpid( id, &status, 0 );
        return id;
    }
    else // id == 0, 这里是子进程
    {
        if ( hStdinWritePipe != NULL )
        {
            close(0);
            dup2( stdinFds[0], 0 );
            close(stdinFds[0]);
            close(stdinFds[1]); // 子进程不需要stdin的写入端
        }
        else
        {
            if ( closeStdinIfStdinWritePipeIsNull )
                close(0);
        }

        if ( hStdoutReadPipe != NULL )
        {
            close(1);
            dup2( stdoutFds[1], 1 );
            close(stdoutFds[1]);
            close(stdoutFds[0]); // 子进程不需要stdout的读取端
        }
        if ( hStderrReadPipe != NULL )
        {
            close(2);
            dup2( stderrFds[1], 2 );
            close(stderrFds[1]);
            close(stderrFds[0]); // 子进程不需要stderr的读取端
        }
        else
        {
            close(2);
            dup2( 1, 2 );
        }
        winux::StringArray args;
        std::vector<char *> argv;
        int argc = CommandLineToArgv( cmd, &args );
        for ( int i = 0; i < argc; ++i )
        {
            argv.push_back( strdup( args[i].c_str() ) );
        }
        argv.push_back(NULL);

        // 执行命令
        int rc = execvp( argv[0], &argv[0] );

        if ( rc < 0 && errno == ENOENT )
        {
            //cerr << "The `cmd` is not found!" << endl;
        }

        for ( auto it = argv.begin(); it != argv.end(); ++it )
        {
            if ( *it ) free(*it);
        }
        exit(1);
    }

    return -1;
}

WINUX_FUNC_IMPL(int) ExecCommand(
    winux::String const & cmd,
    winux::String const & stdinStr,
    winux::String * stdoutStr,
    winux::String * stderrStr,
    bool closeStdinIfStdinStrEmpty
)
{
    HPipe hStdinWrite, hStdoutRead, hStderrRead;
    HProcess hChildProcess;
    int status = 0;
    hChildProcess = ExecCommandEx(
        cmd,
        ( stdinStr.empty() ? NULL : &hStdinWrite ),
        ( stdoutStr ? &hStdoutRead : NULL ),
        ( stderrStr ? &hStderrRead : NULL ),
        closeStdinIfStdinStrEmpty
    );
    if ( hChildProcess == (HProcess)-1 ) return -1;

    if ( !stdinStr.empty() )
    {
        int bytes = write( hStdinWrite, stdinStr.c_str(), stdinStr.length() );
        (void)bytes;

        close(hStdinWrite);
    }

    if ( stdoutStr != NULL )
    {
        stdoutStr->reserve(4096);
        char buf[4096];
        int bytes;
        while ( ( bytes = read( hStdoutRead, buf, 4096 ) ) > 0 )
        {
            stdoutStr->append( buf, bytes );
        }
        close(hStdoutRead);
    }

    if ( stderrStr != NULL )
    {
        stderrStr->reserve(4096);
        char buf[4096];
        int bytes;
        while ( ( bytes = read( hStderrRead, buf, 4096 ) ) > 0 )
        {
            stderrStr->append( buf, bytes );
        }
        close(hStderrRead);
    }

    waitpid( hChildProcess, &status, 0 );

    return (char)( ( status & 0xff00 ) >> 8 );
}

WINUX_FUNC_IMPL(winux::String) GetExec(
    winux::String const & cmd,
    winux::String const & stdinStr,
    winux::String * stderrStr,
    bool closeStdinIfStdinStrEmpty
)
{
    int rc;
    winux::String stdoutStr;
    rc = ExecCommand( cmd, stdinStr, &stdoutStr, stderrStr, closeStdinIfStdinStrEmpty );
    return stdoutStr;
}

#endif

// class CommandLineVars ------------------------------------------------------------------
void CommandLineVars::__MixedAppendToStringArray( Mixed const & mx, StringArray * arr )
{
    if ( mx.isArray() )
    {
        int n = mx.getCount();
        for ( int i = 0; i < n; i++ )
        {
            arr->push_back(mx[i]);
        }
    }
    else if ( mx.isString() )
    {
        String s = mx;
        if ( !s.empty() )
        {
            StringArray tmpArr;
            int n = StrSplit( s, ",", &tmpArr );
            for ( int i = 0; i < n; i++ )
            {
                if ( !tmpArr[i].empty() )
                {
                    arr->push_back(tmpArr[i]);
                }
            }
        }
    }
    else if ( !mx.isNull() )
    {
        arr->push_back(mx);
    }
}

CommandLineVars::CommandLineVars( int argc, char const ** argv, Mixed const & desiredParams, Mixed const & desiredOptions, Mixed const & desiredFlags, Mixed const & optionSymbols /*= "=,:" */ )
{
    _argc = argc;
    _argv = argv;
    __MixedAppendToStringArray( desiredParams, &_desiredParams );
    __MixedAppendToStringArray( desiredOptions, &_desiredOptions );
    __MixedAppendToStringArray( desiredFlags, &_desiredFlags );
    __MixedAppendToStringArray( optionSymbols, &_optionSymbols );

    _params.createCollection();
    _options.createCollection();
    _flags.createArray();
    _values.createArray();

    for ( int i = 1; i < argc; ++i )
    {
        String args = argv[i];
        bool isJudged = false;//是否已经判断属于何种变量

        // Params
        for ( int iDesiredParam = 0; !isJudged && iDesiredParam < (int)_desiredParams.size(); ++iDesiredParam )
        {
            if ( args == _desiredParams[iDesiredParam] )
            {
                isJudged = true;

                _paramIndexesInArgv[_desiredParams[iDesiredParam]] = i;
                //找到一个参数，下一个argv[i]就是参数值
                i++;
                if ( i < argc )
                {
                    _params[_desiredParams[iDesiredParam]] = argv[i];
                }
                else //已经是最后一个，参数值只好认为是空
                {
                    _params[_desiredParams[iDesiredParam]] = "";
                }
            }
        }

        // Options
        for ( int iDesiredOption = 0; !isJudged && iDesiredOption < (int)_desiredOptions.size(); ++iDesiredOption )
        {
            if ( args.length() < _desiredOptions[iDesiredOption].length() )
                continue;
            MultiMatch mmFind( _optionSymbols, NULL );
            MultiMatch::MatchResult mr = mmFind.search(args);
            String optionName, optionVal;
            if ( mr.pos != -1 )
            {
                optionName = args.substr( 0, mr.pos );
                optionVal = args.substr( mr.pos + mmFind.getMatchItem(mr.item).length() );
            }
            else
            {
                optionName = args;
                optionVal = "";
            }

            if ( optionName == _desiredOptions[iDesiredOption] )
            {
                isJudged = true;

                _optionIndexesInArgv[_desiredOptions[iDesiredOption]] = i;
                //找到一个option
                _options[optionName] = optionVal;
            }
        }

        // Flags
        for ( int iDesiredFlag = 0; !isJudged && iDesiredFlag < (int)_desiredFlags.size(); ++iDesiredFlag )
        {
            if ( args == _desiredFlags[iDesiredFlag] )
            {
                isJudged = true;

                _flagIndexesInArgv[_desiredFlags[iDesiredFlag]] = i;
                //找到一个flag
                _flags.add(_desiredFlags[iDesiredFlag]);
            }
        }

        // Values
        if ( !isJudged )
        {
            _valueIndexesInArgv[args] = i;
            _values.add(args);
        }
    }
}


// class CommandLine ----------------------------------------------------------------------
bool CommandLine::_IsName( String const & arg, StringArray const & prefixes )
{
    uint i;
    for ( i = 0; i < prefixes.size(); ++i )
    {
        if ( arg.length() < prefixes[i].length() )
        {
            continue;
        }
        if ( arg.substr( 0, prefixes[i].length() ) == prefixes[i] )
        {
            return true;
        }
    }
    return false;
}

static bool __CompareLengthS1GreaterS2( String const & s1, String const & s2 )
{
    return s1.length() > s2.length();
}

CommandLine::CommandLine( int argc, char * argv[], String const & paramPrefix )
{
    StringArray prefixes;
    StrSplit( paramPrefix, TEXT(" "), &prefixes );
    std::sort( prefixes.begin(), prefixes.end(), __CompareLengthS1GreaterS2 );
    int i;
    String prevStr;
    bool prevIsName = false;
    for ( i = 1; i < argc; ++i )
    {
        bool isName = _IsName( argv[i], prefixes );
        if ( isName ) // 这个arg是一个name
        {
            // 检查先前串是否含有数据
            if ( !prevStr.empty() ) // 如果有数据
            {
                // 则检查先前数据是否为name
                if ( prevIsName ) // 如果是,则先前串就是一个noValueParam
                {
                    _flags.push_back(prevStr);
                }
                else // 如果不是，则先前串就是一个noNameParam
                {
                    _values.push_back(prevStr);
                }
            }
            // 先前记录写入本次数据
            prevStr = argv[i];
            prevIsName = isName;
        }
        else // This arg is a value
        {
            // 检查先前串是否含有数据
            if ( !prevStr.empty() ) // 如果有数据
            {
                // 则检查先前数据是否为name
                if ( prevIsName ) // 如果是,则先前串和本次就是一个Param
                {
                    _params[prevStr] = argv[i];
                    // 先前记录写入本次数据
                    prevStr = "";
                    prevIsName = false;
                }
                else // 如果不是，则先前串就是一个noNameParam
                {
                    _values.push_back(prevStr);
                    // 先前记录写入本次数据
                    prevStr = argv[i];
                    prevIsName = isName;
                }
            }
            else
            {
                // 先前记录写入本次数据
                prevStr = argv[i];
                prevIsName = isName;
            }
        }
    }
    if ( !prevStr.empty() ) // 如果有数据
    {
        // 则检查先前数据是否为name
        if ( prevIsName ) // 如果是,则先前串就是一个noValueParam
        {
            _flags.push_back(prevStr);
        }
        else // 如果不是，则先前串就是一个noNameParam
        {
            _values.push_back(prevStr);
        }
    }
}

// struct MutexLockObj_Data ----------------------------------------------------------------------
struct MutexLockObj_Data
{
#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    pthread_mutex_t _mutex;
#else
    HANDLE _mutex;
#endif
};

// class MutexLockObj ----------------------------------------------------------------------------
MutexLockObj::MutexLockObj()
{
    _self.create(); //

#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    pthread_mutex_init( &_self->_mutex, NULL );
#else
    _self->_mutex = CreateMutex( NULL, FALSE, NULL );
#endif
}

MutexLockObj::~MutexLockObj()
{
#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    pthread_mutex_destroy(&_self->_mutex);
#else
    CloseHandle(_self->_mutex);
#endif

    _self.destroy(); //
}

bool MutexLockObj::tryLock()
{
#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    return pthread_mutex_trylock(&_self->_mutex) == 0;
#else
    return WaitForSingleObject( _self->_mutex, 0 ) == WAIT_OBJECT_0;
#endif
}

bool MutexLockObj::lock()
{
#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    return pthread_mutex_lock(&_self->_mutex) == 0;
#else
    return WaitForSingleObject( _self->_mutex, INFINITE ) == WAIT_OBJECT_0;
#endif
}

bool MutexLockObj::unlock()
{
#if defined(__GNUC__) || defined(HAVE_PTHREAD)
    return pthread_mutex_unlock(&_self->_mutex) == 0;
#else
    return ReleaseMutex(_self->_mutex) != 0;
#endif
}

// struct DllLoader_Data ------------------------------------------------------------------
struct DllLoader_Data
{

};

// class DllLoader ------------------------------------------------------------------------
String DllLoader::GetModulePath( void * funcInModule )
{
#if defined(__GNUC__) && !defined(WIN32)
    Dl_info dlinfo = { 0 };
    int ret = 0;
    ret = dladdr( funcInModule, &dlinfo );
    if ( !ret )
    {
        return GetExecutablePath();
    }
    else
    {
        return dlinfo.dli_fname;
    }
#else
    MEMORY_BASIC_INFORMATION mbi;
    HMODULE hMod = ( ( ::VirtualQuery( funcInModule, &mbi, sizeof(mbi) ) != 0 ) ? (HMODULE)mbi.AllocationBase : NULL );

    AnsiString sz;
    DWORD dwSize = MAX_PATH >> 1;
    DWORD dwGet = 0;
    do
    {
        dwSize <<= 1;
        sz.resize(dwSize);
        dwGet = GetModuleFileName( hMod, &sz[0], dwSize );
        //DWORD dwError = GetLastError();
        //ERROR_INSUFFICIENT_BUFFER;
    }
    while ( dwSize == dwGet );
    return sz.c_str();
#endif
}

DllLoader::DllLoader() : _hDllModule(NULL)
{

}

DllLoader::DllLoader( String const & dllName ) : _hDllModule(NULL), dllModuleFile(dllName)
{
#if defined(_MSC_VER) || defined(WIN32)
    _hDllModule = LoadLibrary( dllName.c_str() );
#else
    _hDllModule = dlopen( dllName.c_str(), RTLD_LAZY );
#endif
}

DllLoader::~DllLoader()
{
    if ( _hDllModule )
    {
#if defined(_MSC_VER) || defined(WIN32)
        FreeLibrary(_hDllModule);
#else
        dlclose(_hDllModule);
#endif
        _hDllModule = NULL;
    }
}

char const * DllLoader::errStr() const
{
#if defined(_MSC_VER) || defined(WIN32)
    if ( !*this )
    {
        return "The DLL is not found or hasn't access permission or the file is not a DLL.";
    }
    return "";
#else
    return dlerror();
#endif
}

int (* DllLoader::funcAddr( AnsiString const & funcName ) )()
{
    if ( !_hDllModule )
    {
        throw DllLoaderError( DllLoaderError::DllLoader_ModuleNoLoaded, "`" + dllModuleFile + "` module is not loaded" );
    }

#if defined(_MSC_VER) || defined(WIN32)
    return (int(*)())GetProcAddress( _hDllModule, funcName.c_str() );
#else
    return (int(*)())dlsym( _hDllModule, funcName.c_str() );
#endif

}

} // namespace winux

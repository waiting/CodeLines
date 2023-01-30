//
// winux的归档文件
//
#include "utilities.hpp"
#include "smartptr.hpp"
#include "filesys.hpp"
#include "strings.hpp"
#include "archives.hpp"

#include "eienexpr.hpp"

#include "zip.h"
#include "unzip.h"

#include <fstream>

namespace winux
{
using namespace eienexpr;

#include "is_x_funcs.inl"

// 判断字符串是否为数字串
inline static bool IsNumberString( winux::String const & str )
{
    if ( str.empty() ) return false;

    bool hasPoint = false;
    int n = 0;
    for ( size_t i = 0; i < str.length(); ++i )
    {
        winux::String::value_type ch = str[i];
        if ( IsDigit(ch) )
        {
            n++;
        }
        else if ( ch == '-' || ch == '+' )
        {
            // 符号不出现在开始或者e后面，则不是数字
            if ( !( i == 0 || ( i > 0 && ( str[i - 1] | 0x20 ) == 'e' ) ) )
            {
                return false;
            }
        }
        else if ( ( ch | 0x20 ) == 'e' )
        {
            if ( n == 0 || i == str.length() - 1 ) return false;
        }
        else if ( ch == '.' )
        {
            if ( hasPoint )
            {
                return false;
            }

            hasPoint = true;
        }
        else
        {
            return false;
        }
    }

    return n < 11; // 数字小于11位才算数字串
}

inline static char NumberStringToChar( const char * number, int base )
{
    char * endptr;
    return (char)strtol( number, &endptr, base );
}

inline static winux::ulong NumberStringToNumber( const char * number, int base )
{
    char * endptr;
    return (winux::ulong)strtoul( number, &endptr, base );
}

// class Configure -----------------------------------------------------------------------------
String const Configure::ConfigVarsSlashChars = "\n\r\t\v\a\\\'\"$()";

Configure::Configure()
{

}

Configure::Configure( String const & configFile )
{
    this->load(configFile);
}

int Configure::_FindConfigRef( String const & str, int offset, int * length, String * name )
{
    String ldelim = "$(";
    String rdelim = ")";
    *length = 0;
    int pos1 = (int)str.find( ldelim, offset );
    if ( pos1 == -1 ) return -1;
    int pos2 = (int)str.find( rdelim, pos1 + ldelim.length() );
    if ( pos2 == -1 ) return -1;
    *length = pos2 + (int)rdelim.length() - pos1;
    *name = str.substr( pos1 + ldelim.length(), pos2 - pos1 - ldelim.length() );
    return pos1;
}

String Configure::_expandVarNoStripSlashes( String const & name, StringArray * chains ) const
{
    if ( !this->has(name) ) return "";
    chains->push_back(name);
    String configVal = _rawParams.at(name);
    String res = "";
    int len;
    String varName;
    int offset = 0;
    int pos;
    while ( ( pos = _FindConfigRef( configVal, offset, &len, &varName ) ) != -1 )
    {
        res += configVal.substr( offset, pos - offset );
        offset = pos + len;
        res += !ValueIsInArray( *chains, varName ) ? _expandVarNoStripSlashes( varName, chains ) : "";
    }
    res += configVal.substr(offset);
    chains->pop_back();
    return res;
}

int Configure::_load( String const & configFile, StringStringMap * rawParams, StringArray * loadFileChains )
{
    loadFileChains->push_back( RealPath(configFile) );
    int lineCount = 0;
    int varsCount = 0;
    try
    {
        std::ifstream fin( configFile.c_str() );
        String line;
        while ( std::getline( fin, line ) )
        {
            lineCount++;
            String tmp = StrTrim(line);

            if ( tmp.empty() || tmp[0] == '#' ) // '#'行开头表示注释,只有在行开头才表示注释
                continue;

            if ( tmp[0] == '@' ) // '@'开头表示配置命令,目前仅支持 @import other-config-file
            {
                int pos = (int)tmp.find(' ');
                String commandName, commandParam;
                if ( pos == -1 )
                {
                    commandName = tmp.substr(1); // 偏移1是为了 skip '@'
                    commandParam = "";
                }
                else
                {
                    commandName = tmp.substr( 1, pos - 1 );
                    commandParam = tmp.substr( pos + 1 );
                }
                if ( commandName == "import" ) // 导入外部配置
                {
                    String dirPath = FilePath(configFile);
                    String confPath = commandParam;
                    confPath = IsAbsPath(confPath) ? confPath : ( dirPath.empty() ? "" : dirPath + dirSep ) + confPath;
                    if ( !ValueIsInArray( *loadFileChains, RealPath(confPath), true ) )
                    {
                        varsCount += _load( confPath, rawParams, loadFileChains );
                    }
                }
                continue;
            }

            int delimPos = (int)line.find('=');
            if ( delimPos == -1 ) // 找不到'='分隔符,就把整行当成参数名
                (*rawParams)[line] = "";
            else
                (*rawParams)[ line.substr( 0, delimPos ) ] = line.substr( delimPos + 1 );

            varsCount++;
        }
    }
    catch ( std::exception & )
    {
    }
    //loadFileChains->pop_back(); //注释掉这句，则每一个文件只载入一次
    return varsCount;
}

int Configure::load( String const & configFile )
{
    _configFile = configFile;
    StringArray loadFileChains;
    return _load( configFile, &_rawParams, &loadFileChains );
}

String Configure::get( String const & name, bool stripslashes, bool expand ) const
{
    String value;

    if ( expand )
    {
        StringArray callChains; // 递归调用链，防止无穷递归
        value = _expandVarNoStripSlashes( name, &callChains );
    }
    else
    {
        value = this->has(name) ? _rawParams.at(name) : "";
    }

    if ( stripslashes )
        value = StripSlashes( value, ConfigVarsSlashChars );

    return value;
}

String Configure::operator [] ( String const & name ) const
{
    return this->has(name) ? StripSlashes( _rawParams.at(name), ConfigVarsSlashChars ) : "";
}

String Configure::operator () ( String const & name ) const
{
    StringArray callChains; // 递归调用链，防止无穷递归
    return StripSlashes( _expandVarNoStripSlashes( name, &callChains ), ConfigVarsSlashChars );
}

void Configure::setRaw( String const & name, String const & value )
{
    _rawParams[name] = value;
}

void Configure::set( String const & name, String const & value )
{
    _rawParams[name] = AddSlashes( value, ConfigVarsSlashChars );
}

bool Configure::del( String const & name )
{
    if ( this->has(name) )
    {
        _rawParams.erase(name);
        return true;
    }
    return false;
}

void Configure::clear()
{
    _rawParams.clear();
}

// struct ConfigureSettings_Data ----------------------------------------------------------
struct ConfigureSettings_Data
{
    String settingsFile; // 设置文件
    Mixed collectionVal; // 存储值
    Mixed collectionExpr; // 存储表达式串
    ExprPackage package; // 表达式语言包，包含自定义函数和算符
    VarContext rootCtx; // Root变量场景

    ConfigureSettings_Data()
    {
        this->collectionVal.createCollection();
        this->collectionExpr.createCollection();
        this->rootCtx.setMixedCollection(&collectionVal);
    }
};

// class ConfigureSettings ----------------------------------------------------------------
ConfigureSettings::ConfigureSettings( String const & settingsFile )
{
    _self.create(); //

    if ( !settingsFile.empty() )
    {
        this->load(settingsFile);
    }
}

ConfigureSettings::~ConfigureSettings()
{


    _self.destroy(); //
}

ConfigureSettings::ConfigureSettings( ConfigureSettings const & other )
{
    _self.create(); //

    _self = other._self;
}

ConfigureSettings::ConfigureSettings( ConfigureSettings && other ) : _self( std::move(other._self) )
{
}

ConfigureSettings & ConfigureSettings::operator = ( ConfigureSettings const & other )
{
    if ( this != &other )
    {
        _self = other._self;
    }
    return *this;
}

ConfigureSettings & ConfigureSettings::operator = ( ConfigureSettings && other )
{
    if ( this != &other )
    {
        _self = std::move(other._self);
    }
    return *this;
}

// 配置解析场景标记flag
enum ConfigureSettings_ParseContext
{
    cpcName,
    cpcValue,
    cpcString,
    cpcStrAntiSlashes,
    cpcExpr,
    cpcCollection,
};

// 一个值类型标记flag
enum OneValueType
{
    ovtAuto,
    ovtString,
    ovtExpr
};

static void ConfigureSettings_ParseCollection( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, Expression * exprCtx, winux::Mixed * collAsVal,  winux::Mixed * collAsExpr );

static void ConfigureSettings_ParseStrAntiSlashes( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, winux::String * v )
{
    winux::String & result = *v;

    ++i; // skip '\\'

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        if ( ch == 'a' )
        {
            result += '\a';
            ++i;
            break;
        }
        else if ( ch == 'b' )
        {
            result += '\b';
            ++i;
            break;
        }
        else if ( ch == 't' )
        {
            result += '\t';
            ++i;
            break;
        }
        else if ( ch == 'n' )
        {
            result += '\n';
            ++i;
            break;
        }
        else if ( ch == 'v' )
        {
            result += '\v';
            ++i;
            break;
        }
        else if ( ch == 'f' )
        {
            result += '\f';
            ++i;
            break;
        }
        else if ( ch == 'r' )
        {
            result += '\r';
            ++i;
            break;
        }
        else if ( IsOct(ch) )
        {
            winux::String octStr;
            for ( ; i < (int)str.length(); ++i )
            {
                ch = str[i];
                if ( IsOct(ch) && octStr.length() < 3 )
                {
                    octStr += ch;
                }
                else
                {
                    break;
                }
            }

            result += NumberStringToChar( octStr.c_str(), 8 );

            break;
        }
        else if ( ( ch | 0x20 ) == 'x' ) // 解析\xHH
        {
            ++i; // skip 'x'
            winux::String hexStr;
            for ( ; i < (int)str.length(); ++i )
            {
                ch = str[i];
                if ( IsHex(ch) && hexStr.length() < 2 )
                {
                    hexStr += ch;
                }
                else
                {
                    break;
                }
            }

            result += NumberStringToChar( hexStr.c_str(), 16 );

            break;
        }
        else // 其余加\的字符都照原样输出
        {
            result += ch;
            ++i;
            break;
        }
    }
}

static void ConfigureSettings_ParseString( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, winux::String * v )
{
    winux::String::value_type quote = str[i];

    v->assign("");
    winux::String & result = *v;

    i++; // skip left quote

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        if ( ch == quote )
        {
            i++; // skip right quote
            break;
        }
        else if ( ch == '\\' )
        {
            cpc.push_back(cpcStrAntiSlashes);
            ConfigureSettings_ParseStrAntiSlashes( cpc, str, i, v );
            cpc.pop_back();
        }
        else
        {
            result += ch;
            i++;
        }
    }
}

static void ConfigureSettings_ParseName( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, winux::String * v )
{
    int start = i;
    v->assign("");
    winux::String & result = *v;

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];

        if ( IsSpace(ch) || ch == ';' )
        {
            break;
        }
        else if ( start == i && ( ch == '\'' || ch == '\"' ) )
        {
            cpc.push_back(cpcString);
            ConfigureSettings_ParseString( cpc, str, i, v );
            cpc.pop_back();

            break;
        }
        else if ( ch == '}' )
        {
            break;
        }
        else
        {
            result += ch;
            i++;
        }
    }
}

static void ConfigureSettings_ParseExpr( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, winux::String * v )
{
    i++; // skip '('
    int brackets = 1; // 括号配对。0时表示一个表达式结束

    v->assign("");

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        if ( ch == ';' )
        {
            //i++;
            break;
        }
        else if ( ch == '\'' || ch == '\"' )
        {
            cpc.push_back(cpcString);
            winux::String tmp;
            ConfigureSettings_ParseString( cpc, str, i, &tmp );
            cpc.pop_back();

            *v += ch + winux::AddCSlashes(tmp) + ch;
        }
        else if ( ch == '(' )
        {
            ++brackets;

            *v += ch;
            i++;

        }
        else if ( ch == ')' )
        {
            if (--brackets < 1)
            {
                i++; // skip ')'
                break;
            }
            else
            {
                *v += ch;
                i++;
            }
        }
        else
        {
            *v += ch;
            i++;
        }
    }
}

static void _StoreValue( winux::String const & oneValue, OneValueType oneValueType, Expression * exprCtx, winux::Mixed & arr, winux::Mixed & arrExpr )
{
    if ( oneValue.empty() )
    {
        arr.add("");
        arrExpr.add("\"\"");
    }
    else
    {
        switch ( oneValueType )
        {
        case ovtAuto:
            if ( IsNumberString(oneValue) )
            {
                winux::Mixed r;
                try
                {
                    exprCtx->clear();
                    ExprParser().parse( exprCtx, oneValue );
                    r = exprCtx->val();
                }
                catch ( ExprError const & /*e*/ )
                {
                    //ColorOutput( fgRed, e.what() );
                }

                arr.add(r);
                arrExpr.add(oneValue);
            }
            else
            {
                arr.add(oneValue);
                arrExpr.add("\"" + AddCSlashes(oneValue) + "\"");
            }
            break;
        case ovtString:
            {
                arr.add(oneValue);
                arrExpr.add("\"" + AddCSlashes(oneValue) + "\"");
            }
            break;
        case ovtExpr:
            {
                winux::Mixed r;
                try
                {
                    exprCtx->clear();
                    ExprParser().parse( exprCtx, oneValue );
                    r = exprCtx->val();
                }
                catch ( ExprError const & /*e*/ )
                {
                    //ColorOutput( fgRed, e.what() );
                }

                arr.add(r);
                arrExpr.add("(" + oneValue + ")");
            }
        }
    }
}

static void ConfigureSettings_ParseValue( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, Expression * exprCtx, winux::Mixed * value, winux::Mixed * valExpr )
{
    int start = i;

    winux::Mixed arr; // 数组，可能含有多个
    arr.createArray();
    winux::Mixed arrExpr; // 数组，可能含有多个
    arrExpr.createArray();

    winux::String oneValue; // 单个值串
    OneValueType oneValueType = ovtAuto; // 值串类型 0:自动判断数字或字符串  1:字符串  2:表达式串

    oneValue.assign("");

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];

        if ( ch == ';' ) // 遇到分号，说明值结束
        {
            if ( !oneValue.empty() )
            {
                _StoreValue( oneValue, oneValueType, exprCtx, arr, arrExpr );
                oneValue.assign("");
                oneValueType = ovtAuto;
            }
            else if ( oneValue.empty() && arr._pArr->size() == 0 ) // 遇到了结束分号却依旧没有读取到值，就加个空Mixed作值
            {
                _StoreValue( oneValue, oneValueType, exprCtx, arr, arrExpr );
                oneValue.assign("");
                oneValueType = ovtAuto;
            }

            i++; // skip ';'
            break;
        }
        else if ( IsSpace(ch) )
        {
            if ( !oneValue.empty() )
            {
                _StoreValue( oneValue, oneValueType, exprCtx, arr, arrExpr );
                oneValue.assign("");
                oneValueType = ovtAuto;
            }

            i++; // skip space-char
        }
        else if ( start == i && ( ch == '\'' || ch == '\"' ) )
        {
            cpc.push_back(cpcString);
            ConfigureSettings_ParseString( cpc, str, i, &oneValue );
            cpc.pop_back();

            oneValueType = ovtString;
        }
        else if ( start == i && ch == '(' ) // 表达式
        {
            cpc.push_back(cpcExpr);
            ConfigureSettings_ParseExpr( cpc, str, i, &oneValue );
            cpc.pop_back();

            oneValueType = ovtExpr;
        }
        else if ( ch == '{' ) // 又进入一个Collection
        {
            i++; // skip '{'

            winux::Mixed value;       // 存值
            winux::Mixed valExpr;     // 存值表达式串

            cpc.push_back(cpcCollection);

            value.createCollection();
            valExpr.createCollection();

            VarContext varCtx(&value);
            Expression exprSubScope( exprCtx->getPackage(), &varCtx, exprCtx, nullptr );

            ConfigureSettings_ParseCollection( cpc, str, i, &exprSubScope, &value, &valExpr );
            cpc.pop_back();

            arr.add(value);
            arrExpr.add(valExpr);

            oneValue.assign("");
            oneValueType = ovtAuto;

            // 如果是"}\n"，也说明值结束
            if ( i < (int)str.length() )
            {
                i++; // skip '}'
                if ( i < (int)str.length() )
                {
                    ch = str[i];
                    if ( ch == '\n' || ch == '\r' )
                    {
                        i++; // skip '\n' or '\r'
                        break;
                    }
                }
            }
        }
        else if ( ch == '}' )
        {
            break;
        }
        else
        {
            oneValue += ch;
            i++;
        }
    }

    if ( !oneValue.empty() )
    {
        _StoreValue( oneValue, oneValueType, exprCtx, arr, arrExpr );
        oneValue.assign("");
        oneValueType = ovtAuto;
    }

    if ( arr.getCount() == 1 )
    {
        *value = std::move(arr[0]);
    }
    else if ( arr.getCount() == 0 )
    {
        value->assign("");
    }
    else
    {
        *value = std::move(arr);
    }

    if ( arrExpr.getCount() == 1 )
    {
        *valExpr = std::move(arrExpr[0]);
    }
    else if ( arrExpr.getCount() == 0 )
    {
        valExpr->assign("");
    }
    else
    {
        *valExpr = std::move(arrExpr);
    }
}

static void ConfigureSettings_ParseCollection( std::vector<ConfigureSettings_ParseContext> & cpc, winux::String const & str, int & i, Expression * exprCtx, winux::Mixed * collAsVal,  winux::Mixed * collAsExpr )
{
    bool currIsName = true;   // 当前是否解析名字
    winux::String name;       // 存名字
    winux::Mixed value;       // 存值
    winux::Mixed valExpr;     // 存值表达式串
    bool hasName = false;     // 是否有名字
    bool hasValue = false;    // 是否有值

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        if ( IsSpace(ch) )
        {
            i++;
        }
        else if ( ch == '#' ) // 行注释
        {
            i++; // skip '#'

            while ( i < (int)str.length() && str[i] != '\n' ) i++; // 一直读取，直到'\n'

            if ( i < (int)str.length() ) i++; // skip '\n' if not end
        }
        else if ( ch == '}' )
        {
            break;
        }
        else
        {
            if ( currIsName )
            {
                cpc.push_back(cpcName);
                ConfigureSettings_ParseName( cpc, str, i, &name );
                cpc.pop_back();

                hasName = true;
                currIsName = false;
            }
            else
            {
                cpc.push_back(cpcValue);
                ConfigureSettings_ParseValue( cpc, str, i, exprCtx, &value, &valExpr );
                cpc.pop_back();

                hasValue = true;
                currIsName = true;

                if ( hasName && hasValue )
                {
                    exprCtx->getVarContext()->set( name, value );
                    (*collAsExpr)[name] = valExpr;

                    name.assign("");
                    value.free();
                    valExpr.free();
                    currIsName = true;
                    hasName = hasValue = false;
                }
                else if ( hasName )
                {
                    exprCtx->getVarContext()->set( name, winux::Mixed() );
                    (*collAsExpr)[name] = valExpr;

                    name.assign("");
                    value.free();
                    valExpr.free();
                    currIsName = true;
                    hasName = hasValue = false;
                }
            }
        }
    }

    if ( hasName && hasValue )
    {
        exprCtx->getVarContext()->set( name, value );
        (*collAsExpr)[name] = valExpr;

    }
    else if ( hasName )
    {
        exprCtx->getVarContext()->set( name, winux::Mixed() );
        (*collAsExpr)[name] = valExpr;
    }

}

size_t ConfigureSettings::_load( String const & settingsFile, winux::Mixed * collAsVal, winux::Mixed * collAsExpr, StringArray * loadFileChains )
{
    String settingsFileRealPath = RealPath(settingsFile);
    loadFileChains->push_back(settingsFileRealPath);

    String settingsContent = FileGetContents( settingsFileRealPath, true );
    {
        std::vector<ConfigureSettings_ParseContext> cpc;
        cpc.push_back(cpcCollection);

        if ( !collAsVal->isCollection() ) collAsVal->createCollection();
        if ( !collAsExpr->isCollection() ) collAsExpr->createCollection();

        VarContext varCtx(collAsVal);
        Expression exprCtx( &_self->package, &varCtx, nullptr, nullptr );

        int i = 0;
        ConfigureSettings_ParseCollection( cpc, settingsContent, i, &exprCtx, collAsVal, collAsExpr );

        cpc.pop_back();
    }

    //loadFileChains->pop_back(); //注释掉这句，则每一个文件只载入一次

    _self->rootCtx.setMixedCollection(&_self->collectionVal); // 每次载入完设置后重新设置Root变量场景

    return _self->collectionVal.getCount();
}

size_t ConfigureSettings::load( String const & settingsFile )
{
    _self->settingsFile = settingsFile;
    StringArray loadFileChains;
    return this->_load(settingsFile, &_self->collectionVal, &_self->collectionExpr, &loadFileChains );
}

static void _Update( struct ConfigureSettings_Data & sd, Expression * parent, Mixed & collVal, Mixed & collExpr, StringArray const & names, String const & updateExprStr, Mixed * * v )
{
    if ( collVal.isCollection() ) // 集合，应该创建表达式变量场景对象
    {
        if ( names.size() > 0 )
        {
            VarContext ctx(&collVal);
            Expression exprCtx( &sd.package, &ctx, parent, ( parent ? parent->getDataPtr() : nullptr ) );

            StringArray newNames( names.begin() + 1, names.end() );

            _Update( sd, &exprCtx, collVal[ names[0] ], collExpr[ names[0] ], newNames, updateExprStr, v );
        }
        else
        {
            *v = &collVal;
        }
    }
    else if ( collVal.isArray() ) // 数组
    {
        if ( names.size() > 0 )
        {
            StringArray newNames( names.begin() + 1, names.end() );

            _Update( sd, parent, collVal[ names[0] ], collExpr[ names[0] ], newNames, updateExprStr, v );
        }
        else
        {
            *v = &collVal;
        }
    }
    else // 非容器，应该通过表达式计算值
    {
        Expression expr( &sd.package, nullptr, parent, ( parent ? parent->getDataPtr() : nullptr ) );

        if ( !updateExprStr.empty() )
        {
            collExpr = updateExprStr;
        }

        ExprParser().parse( &expr, collExpr );
        collVal = expr.val();
        *v = &collVal;
    }
}

Mixed & ConfigureSettings::update( String const & multiname, String const & updateExprStr )
{
    StringArray names;

    // 这里并不是计算，而是解析层次，记下每层的名字
    Expression te( &_self->package, nullptr, nullptr, nullptr );
    ExprParser().parse( &te, multiname );

    for ( ExprAtom * atom : te._suffixAtoms )
    {
        if ( atom->getAtomType() == ExprAtom::eatOperand )
        {
            ExprOperand * opd = (ExprOperand *)atom;

            if ( opd->getOperandType() == ExprOperand::eotLiteral )
            {
                names.push_back( ((ExprLiteral *)opd)->getValue() );
            }
            else
            {
                names.push_back( atom->toString() );
            }
        }
    }

    Mixed * v = nullptr;
    _Update( _self, nullptr, _self->collectionVal, _self->collectionExpr, names, updateExprStr, &v );
    return *v;
}

Mixed & ConfigureSettings::execRef( String const & exprStr ) const
{
    thread_local Mixed inner;
    try
    {
        Expression expr( const_cast<ExprPackage*>(&_self->package), const_cast<VarContext*>(&_self->rootCtx), nullptr, nullptr );
        ExprParser().parse( &expr, exprStr );
        Mixed * v = nullptr;
        if ( expr.evaluateMixedPtr(&v) )
        {
            return *v;
        }
        inner.free();
        return inner;
    }
    catch ( winux::ExprError const & /*e*/ )
    {
    }
    inner.free();
    return inner;
}

Mixed ConfigureSettings::execVal( String const & exprStr, Mixed const & defval ) const
{
    try
    {
        Expression expr( const_cast<ExprPackage*>(&_self->package), const_cast<VarContext*>(&_self->rootCtx), nullptr, nullptr );
        ExprParser().parse( &expr, exprStr );
        return expr.val();
    }
    catch ( winux::ExprError const & /*e*/ )
    {
    }
    return defval;
}

Mixed const & ConfigureSettings::operator [] ( String const & name ) const
{
    return this->get(name);
}

Mixed & ConfigureSettings::operator [] ( String const & name )
{
    return _self->collectionVal[name];
}

bool ConfigureSettings::has( String const & name ) const
{
    return _self->collectionVal.has(name);
}

winux::Mixed const & ConfigureSettings::get( String const & name ) const
{
    thread_local Mixed const inner;
    return _self->collectionVal.has(name) ? _self->collectionVal[name] : inner;
}

ConfigureSettings & ConfigureSettings::set( String const & name, Mixed const & value )
{
    _self->collectionVal[name] = value;
    return *this;
}

Mixed const & ConfigureSettings::val() const
{
    return _self->collectionVal;
}

Mixed & ConfigureSettings::val()
{
    return _self->collectionVal;
}

Mixed const & ConfigureSettings::expr() const
{
    return _self->collectionExpr;
}

Mixed & ConfigureSettings::expr()
{
    return _self->collectionExpr;
}

// class CsvWriter ------------------------------------------------------------------------
inline static String __JudgeAddQuotes( String const & valStr )
{
    String::const_iterator it = valStr.begin();
    for ( ; it != valStr.end(); ++it )
    {
        if ( *it == ',' || *it == '\"' ||  *it == '\'' || *it == '\n' )
        {
            break;
        }
    }
    if ( it != valStr.end() )
    {
        return "\"" + AddQuotes(valStr) + "\"";
    }
    else
    {
        return valStr;
    }
}

CsvWriter::CsvWriter( IFile * outputFile ) : _outputFile(outputFile)
{
}

void CsvWriter::write( Mixed const & records, Mixed const & columnHeaders /*= Mixed() */ )
{
    if ( columnHeaders.isArray() )
    {
        this->writeRecord(columnHeaders);
    }
    if ( records.isArray() ) //多条记录
    {
        size_t i, n = records.getCount();
        for ( i = 0; i < n; i++ )
        {
            this->writeRecord(records[i]);
        }
    }
    else // 只有一条记录
    {
        this->writeRecord(records);
    }
}

void CsvWriter::writeRecord( Mixed const & record )
{
    if ( record.isArray() ) // 多个列
    {
        size_t i, n = record.getCount();
        String strRecord = "";
        for ( i = 0; i < n; i++ )
        {
            if ( i != 0 ) strRecord += ",";
            strRecord += __JudgeAddQuotes(record[i]);
        }
        _outputFile->puts( strRecord + "\n" );
    }
    else if ( record.isCollection() )
    {
        size_t i, n = record.getCount();
        String strRecord = "";
        for ( i = 0; i < n; i++ )
        {
            if ( i != 0 ) strRecord += ",";
            strRecord += __JudgeAddQuotes( record.getPair(i).second );
        }
        _outputFile->puts( strRecord + "\n" );
    }
    else // 只有1列
    {
        _outputFile->puts( (String)__JudgeAddQuotes(record) + "\n" );
    }
}

// class CsvReader ------------------------------------------------------------------------
CsvReader::CsvReader( IFile * inputFile, bool hasColumnHeaders /*= false */ )
{
    if ( inputFile ) this->read( inputFile->buffer(), hasColumnHeaders );
}

CsvReader::CsvReader( String const & content, bool hasColumnHeaders /*= false */ )
{
    if ( !content.empty() ) this->read( content, hasColumnHeaders );
}

void CsvReader::read( String const & content, bool hasColumnHeaders /*= false */ )
{
    int i = 0;
    if ( hasColumnHeaders )
    {
        Mixed hdrs;
        _readRecord( content, i, hdrs );
        _columns.createCollection();
        for ( size_t i = 0, n = hdrs.getCount(); i < n; ++i )
        {
            _columns[ hdrs[i] ] = i;
        }
    }

    _records.createArray();
    while ( i < (int)content.length() )
    {
        Mixed & record = _records[ _records.add( Mixed() ) ];
        _readRecord( content, i, record );
    }
}

void CsvReader::_readRecord( String const & str, int & i, Mixed & record )
{
    record.createArray();
    String valStr = "";
    while ( i < (int)str.length() )
    {
        String::value_type ch = str[i];
        if ( ch == '\n' ) //结束一条记录
        {
            record.add(valStr);
            i++; // skip '\n'
            break;
        }
        else if ( ch == ',' ) //结束一个值
        {
            record.add(valStr);
            valStr.clear();
            i++; // skip ','
        }
        else if ( ch == '\"' )
        {
            //去除之前可能的空白
            if ( StrTrim(valStr).empty() )
            {
                valStr.clear(); //去除之前可能获得的空白字符
                _readString( str, i, valStr );
                //record.add(valStr);
                //valStr.clear();
                //跳过','以避免再次加入这个值,或者跳过换行符
                //while ( i < (int)str.length() && str[i] != ',' && str[i] != '\n' ) i++;
            }
            else
            {
                valStr += ch;
                i++;
            }
        }
        else
        {
            valStr += ch;
            i++;
        }
    }

    if ( str.length() > 0 && '\n' != str[i - 1] ) // 最后一个字符不是换行符
    {
        record.add(valStr);
    }
}

void CsvReader::_readString( String const & str, int & i, String & valStr )
{
    ++i; // skip '\"'
    while ( i < (int)str.length() )
    {
        String::value_type ch = str[i];
        if ( ch == '\"' )
        {
            if ( i + 1 < (int)str.length() && str[i+1] == '\"' ) // double '\"' 解析成一个'\"'
            {
                valStr += '\"';
                i++; // skip "
                i++; // skip "
            }
            else
            {
                i++; // skip 作为字符串结束的尾"
                break;
            }
        }
        else
        {
            valStr += ch;
            i++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

template < typename _Ty >
inline static _Ty _ByteOrder( _Ty v, bool b )
{
    if ( b )
    {
        return InvertByteOrder(v);
    }
    else
    {
        return v;
    }
}

template < typename _ChTy >
winux::XString<_ChTy> _NewlineFromFile( winux::XString<_ChTy> const & content, bool b )
{
#if defined(OS_WIN)
    winux::XString<_ChTy> r2;
    for ( size_t i = 0; i < content.length(); i++ )
    {
        if ( content[i] == _ByteOrder( winux::Literal<_ChTy>::crChar, b ) )
        {
            i++;
            if ( i < content.length() )
            {
                if ( content[i] == _ByteOrder( winux::Literal<_ChTy>::lfChar, b ) )
                {
                    r2 += _ByteOrder( winux::Literal<_ChTy>::lfChar, b );
                }
                else
                {
                    r2 += _ByteOrder( winux::Literal<_ChTy>::crChar, b );
                    r2 += content[i];
                }
            }
            else
            {
                r2 += _ByteOrder( winux::Literal<_ChTy>::crChar, b );
            }
        }
        else
        {
            r2 += content[i];
        }
    }
    return r2;
#elif defined(OS_DARWIN)
    winux::XString<_ChTy> r2;
    for ( size_t i = 0; i < content.length(); i++ )
    {
        if ( content[i] == _ByteOrder( winux::Literal<_ChTy>::crChar, b ) )
        {
            r2 += _ByteOrder( winux::Literal<_ChTy>::lfChar, b );
        }
        else
        {
            r2 += content[i];
        }
    }
    return r2;
#else
    return content;
#endif
}

template < typename _ChTy >
winux::XString<_ChTy> _NewlineToFile( winux::XString<_ChTy> const & content, bool b )
{
#if defined(OS_WIN)
    winux::XString<_ChTy> r2;
    for ( size_t i = 0; i < content.length(); i++ )
    {
        if ( content[i] == _ByteOrder( winux::Literal<_ChTy>::lfChar, b ) )
        {
            r2 += _ByteOrder( winux::Literal<_ChTy>::crChar, b );
            r2 += _ByteOrder( winux::Literal<_ChTy>::lfChar, b );
        }
        else
        {
            r2 += content[i];
        }
    }
    return r2;
#elif defined(OS_DARWIN)
    winux::XString<_ChTy> r2;
    for ( size_t i = 0; i < content.length(); i++ )
    {
        if ( content[i] == _ByteOrder( winux::Literal<_ChTy>::lfChar, b ) )
        {
            r2 += _ByteOrder( winux::Literal<_ChTy>::crChar, b );
        }
        else
        {
            r2 += content[i];
        }
    }
    return r2;
#else
    return content;
#endif
}

// class TextArchive ----------------------------------------------------------------------
void TextArchive::saveEx( winux::Buffer const & content, winux::AnsiString const & encoding, winux::GrowBuffer * output, FileEncoding fileEncoding )
{
    switch ( fileEncoding )
    {
    case MultiByte:
        {
            winux::Conv conv{ encoding.c_str(), this->_mbsEncoding.c_str() };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::AnsiString res( buf, n );
            free(buf);
            output->append( _NewlineToFile( res, false ) );
        }
        break;
    case Utf8:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-8" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::AnsiString res( buf, n );
            free(buf);
            output->append( _NewlineToFile( res, false ) );
        }
        break;
    case Utf8Bom:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-8" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::AnsiString res( buf, n );
            free(buf);
            // write BOM
            output->append( { '\xef', '\xbb', '\xbf' } );
            output->append( _NewlineToFile( res, false ) );
        }
        break;
    case Utf16Le:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-16LE" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::Utf16String res( (winux::Utf16String::value_type *)buf, n / sizeof(winux::Utf16String::value_type) );
            free(buf);
            // write BOM
            output->append( { '\xff', '\xfe' } );
            winux::Utf16String res2 = _NewlineToFile( res, IsBigEndian() );
            output->append( res2.c_str(), res2.length() * sizeof(winux::Utf16String::value_type) );
        }
        break;
    case Utf16Be:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-16BE" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::Utf16String res( (winux::Utf16String::value_type *)buf, n / sizeof(winux::Utf16String::value_type) );
            free(buf);
            // write BOM
            output->append( { '\xfe', '\xff' } );
            winux::Utf16String res2 = _NewlineToFile( res, IsLittleEndian() );
            output->append( res2.c_str(), res2.length() * sizeof(winux::Utf16String::value_type) );
        }
        break;
    case Utf32Le:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-32LE" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::Utf32String res( (winux::Utf32String::value_type *)buf, n / sizeof(winux::Utf32String::value_type) );
            free(buf);
            // write BOM
            output->append( { '\xff', '\xfe', '\0', '\0' } );
            winux::Utf32String res2 = _NewlineToFile( res, IsBigEndian() );
            output->append( res2.c_str(), res2.length() * sizeof(winux::Utf32String::value_type) );
        }
        break;
    case Utf32Be:
        {
            winux::Conv conv{ encoding.c_str(), "UTF-32BE" };
            char * buf;
            size_t n = conv.convert( content.getBuf<char>(), content.getSize(), &buf );
            winux::Utf32String res( (winux::Utf32String::value_type *)buf, n / sizeof(winux::Utf32String::value_type) );
            free(buf);
            // write BOM
            output->append( { '\0', '\0', '\xfe', '\xff' } );
            winux::Utf32String res2 = _NewlineToFile( res, IsLittleEndian() );
            output->append( res2.c_str(), res2.length() * sizeof(winux::Utf32String::value_type) );
        }
        break;
    }
}

void TextArchive::_processContent( winux::Buffer const & content, bool isConvert, winux::AnsiString const & encoding )
{
    switch ( _fileEncoding )
    {
    case MultiByte:
        {
            winux::XString<char> strContent = _NewlineFromFile( content.toAnsi(), false );
            if ( isConvert )
            {
                winux::Conv conv{ this->_mbsEncoding.c_str(), encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char), false );
                this->setContentEncoding(this->_mbsEncoding);
            }
        }
        break;
    case Utf8:
        {
            winux::XString<char> strContent = _NewlineFromFile( content.toAnsi(), false );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-8", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char), false );
                this->setContentEncoding("UTF-8");
            }
        }
        break;
    case Utf8Bom:
        {
            winux::XString<char> strContent = _NewlineFromFile( content.toAnsi(), false );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-8", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char), false );
                this->setContentEncoding("UTF-8");
            }
        }
        break;
    case Utf16Le:
        {
            winux::XString<char16> strContent = _NewlineFromFile( content.toString<char16>(), IsBigEndian() );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-16LE", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char16), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char16), false );
                this->setContentEncoding("UTF-16LE");
            }
        }
        break;
    case Utf16Be:
        {
            winux::XString<char16> strContent = _NewlineFromFile( content.toString<char16>(), IsLittleEndian() );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-16BE", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char16), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char16), false );
                this->setContentEncoding("UTF-16BE");
            }
        }
        break;
    case Utf32Le:
        {
            winux::XString<char32> strContent = _NewlineFromFile( content.toString<char32>(), IsBigEndian() );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-32LE", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char32), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char32), false );
                this->setContentEncoding("UTF-32LE");
            }
        }
        break;
    case Utf32Be:
        {
            winux::XString<char32> strContent = _NewlineFromFile( content.toString<char32>(), IsLittleEndian() );
            if ( isConvert )
            {
                winux::Conv conv{ "UTF-32BE", encoding.c_str() };
                char * buf;
                size_t n = conv.convert( (char const *)strContent.c_str(), strContent.length() * sizeof(char32), &buf );
                this->_pureContent.setBuf( buf, n, false );
                free(buf);
                this->setContentEncoding(encoding);
            }
            else
            {
                this->_pureContent.setBuf( (void *)strContent.c_str(), strContent.length() * sizeof(char32), false );
                this->setContentEncoding("UTF-32BE");
            }
        }
        break;
    }
}

void TextArchive::_recognizeEncode( winux::Buffer const & content, size_t * pI )
{
    this->_fileEncoding = MultiByte;
    size_t & i = *pI;
    size_t k;
    if ( i < content.getSize() )
    {
        if ( content[i] == 0xEF )
        {
            i++;
            if ( i < content.getSize() )
            {
                if ( content[i] == 0xBB )
                {
                    i++;
                    if ( i < content.getSize() )
                    {
                        if ( content[i] == 0xBF )
                        {
                            i++;
                            this->_fileEncoding = Utf8Bom;
                        }
                    }
                }
            }
        }
        else if ( content[i] == 0xFF )
        {
            i++;
            if ( i < content.getSize() )
            {
                if ( content[i] == 0xFE )
                {
                    i++;
                    this->_fileEncoding = Utf16Le;
                    if ( i < content.getSize() )
                    {
                        if ( content[i] == 0x00 )
                        {
                            i++;
                            if ( i < content.getSize() )
                            {
                                if ( content[i] == 0x00 )
                                {
                                    i++;
                                    this->_fileEncoding = Utf32Le;
                                }
                            }
                        }
                    }
                }
            }
        }
        else if ( content[i] == 0xFE )
        {
            i++;
            if ( i < content.getSize() )
            {
                if ( content[i] == 0xFF )
                {
                    i++;
                    this->_fileEncoding = Utf16Be;
                }
            }
        }
        else if ( content[i] == 0x00 )
        {
            i++;
            if ( i < content.getSize() )
            {
                if ( content[i] == 0x00 )
                {
                    i++;
                    if ( i < content.getSize() )
                    {
                        if ( content[i] == 0xFE )
                        {
                            i++;
                            if ( i < content.getSize() )
                            {
                                if ( content[i] == 0xFF )
                                {
                                    i++;
                                    this->_fileEncoding = Utf32Be;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if ( this->_fileEncoding == MultiByte )
    {
        int bytesOfHighestBit1 = 0; // 最高位是1的字节数
        for ( k = 0; k < content.getSize(); k++ )
        {
            if ( ( content[k] & 0x80 ) == 0x80 )
            {
                bytesOfHighestBit1++;

                if ( bytesOfHighestBit1 == 1 )
                {
                    if ( ( content[k] & 0xF0 ) == 0xF0 )
                    {
                        k++;
                        if ( k < content.getSize() )
                        {
                            if ( ( content[k] & 0xC0 ) == 0x80 )
                            {
                                k++;
                                if ( k < content.getSize() )
                                {
                                    if ( ( content[k] & 0xC0 ) == 0x80 )
                                    {
                                        k++;
                                        if ( k < content.getSize() )
                                        {
                                            if ( ( content[k] & 0xC0 ) == 0x80 )
                                            {
                                                this->_fileEncoding = Utf8;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if ( ( content[k] & 0xE0 ) == 0xE0 )
                    {
                        k++;
                        if ( k < content.getSize() )
                        {
                            if ( ( content[k] & 0xC0 ) == 0x80 )
                            {
                                k++;
                                if ( k < content.getSize() )
                                {
                                    if ( ( content[k] & 0xC0 ) == 0x80 )
                                    {
                                        this->_fileEncoding = Utf8;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if ( ( content[k] & 0xC0 ) == 0xC0 )
                    {
                        k++;
                        if ( k < content.getSize() )
                        {
                            if ( ( content[k] & 0xC0 ) == 0x80 )
                            {
                                this->_fileEncoding = Utf8;
                                break;
                            }
                        }
                    }
                }
                else if ( bytesOfHighestBit1 == 2 )
                {
                    bytesOfHighestBit1 = 0;
                }
            }
            else
            {
                bytesOfHighestBit1 = 0;
            }
        }
    }
}

// struct Zip_Data ------------------------------------------------------------------------
struct Zip_Data
{
    HZIP hzip;

    Zip_Data() : hzip(0)
    {
    }
};

// class Zip ------------------------------------------------------------------------------
String Zip::Message( ZRESULT code )
{
    String s;
    s.resize(128);
    FormatZipMessageZ( code, &s[0], 128 );
    return s.c_str();
}

Zip::Zip()
{
    _self.create(); //

}

Zip::Zip( String const & filename, char const * password /*= NULL */ )
{
    _self.create(); //

    this->create( filename, password );
}

Zip::Zip( void * buf, uint32 size, char const * password /*= NULL */ )
{
    _self.create(); //

    this->create( buf, size, password );
}

Zip::~Zip()
{
    this->close();

    _self.destroy(); //
}

bool Zip::create( String const & filename, char const * password /*= NULL */ )
{
    this->close();

    _self->hzip = CreateZip( filename.c_str(), password );

#if defined(__linux__) || ( defined(__GNUC__) && !defined(_WIN32) )
    return _self->hzip != NULL;
#else
    return _self->hzip != INVALID_HANDLE_VALUE;
#endif
}

bool Zip::create( void * buf, uint32 size, char const * password /*= NULL */ )
{
    this->close();

    _self->hzip = CreateZip( buf, size, password );

#if defined(__linux__) || ( defined(__GNUC__) && !defined(_WIN32) )
    return _self->hzip != NULL;
#else
    return _self->hzip != INVALID_HANDLE_VALUE;
#endif
}

ZRESULT Zip::close()
{
    ZRESULT zr = CloseZipZ(_self->hzip);
    _self->hzip = 0;
    return zr;
}

ZRESULT Zip::addFile( String const & dstPathInZip, String const & srcFilename )
{
    return ZipAdd( _self->hzip, dstPathInZip.c_str(), srcFilename.c_str() );
}

ZRESULT Zip::addFile( String const & dstPathInZip, void * src, uint32 size )
{
    return ZipAdd( _self->hzip, dstPathInZip.c_str(), src, size );
}

ZRESULT Zip::addFolder( String const & dstPathInZip )
{
    return ZipAddFolder( _self->hzip, dstPathInZip.c_str() );
}

static void __ZipAll( Zip * z, String const & dirPath, String const & dstPath )
{

    DirIterator di(dirPath);
    while ( di.next() )
    {
        if ( di.getName() == "." || di.getName() == ".." )
        {
            continue;
        }
        if ( di.isDir() )
        {
            //printf( "subdir. %s | %s\n", di.getFullPath().c_str(), (dstPath + DirSep + di.getName()).c_str() );
            String dst = dstPath + DirSep + di.getName();
            z->addFolder(dst);
            __ZipAll( z, di.getFullPath(), dst );
        }
        else
        {
            String dst = dstPath + DirSep + di.getName();
            z->addFile( dst, di.getFullPath() );
            //printf( "file. %s | %s\n", di.getFullPath().c_str(), (dstPath + DirSep + di.getName()).c_str() );
            //printf( "%s\n", di.getFullPath().c_str() );
        }
    }
}

void Zip::zipAll( String const & dirPath )
{
    String realPath = RealPath(dirPath);
    String dstDir;
    FilePath( realPath, &dstDir );
    //printf( "%s\n", dstDir.c_str() );
    this->addFolder(dstDir);

    __ZipAll( this, dirPath, dstDir );
}

ZRESULT Zip::getMemory( void * * buf, unsigned long * size )
{
    return ZipGetMemory( _self->hzip, buf, size );
}

// struct Unzip_Data ----------------------------------------------------------------------
struct Unzip_Data
{
    HZIP hzip;
    Unzip_Data() : hzip(0)
    {
    }
};

// class Unzip ----------------------------------------------------------------------------
String Unzip::Message( ZRESULT code /*= ZR_RECENT */ )
{
    String s;
    s.resize(128);
    FormatZipMessageU( code, &s[0], 128 );
    return s.c_str();

}

Unzip::Unzip()
{
    _self.create(); //

}

Unzip::Unzip( String const & filename, char const * password /*= NULL */ )
{
    _self.create(); //

    this->open( filename, password );
}

Unzip::Unzip( void * zipbuf, uint32 size, char const * password /*= NULL */ )
{
    _self.create(); //

    this->open( zipbuf, size, password );
}

Unzip::~Unzip()
{
    this->close();

    _self.destroy(); //
}

bool Unzip::open( String const & filename, char const * password /*= NULL */ )
{
    this->close();

    _self->hzip = OpenZip( filename.c_str(), password );

#if defined(__linux__) || ( defined(__GNUC__) && !defined(_WIN32) )
    return _self->hzip != NULL;
#else
    return _self->hzip != INVALID_HANDLE_VALUE;
#endif
}

bool Unzip::open( void * zipbuf, uint32 size, char const * password /*= NULL */ )
{
    this->close();

    _self->hzip = OpenZip( zipbuf, size, password );

#if defined(__linux__) || ( defined(__GNUC__) && !defined(_WIN32) )
    return _self->hzip != NULL;
#else
    return _self->hzip != INVALID_HANDLE_VALUE;
#endif

}

ZRESULT Unzip::close()
{
    ZRESULT zr = CloseZipU(_self->hzip);
    _self->hzip = 0;
    return zr;
}

int Unzip::getEntriesCount() const
{
    ZIPENTRY ze;
    GetZipItem( _self->hzip, -1, &ze );
    return ze.index;
}

ZRESULT Unzip::getEntry( int index, ZipEntry * entry )
{
    ZIPENTRY ze;
    ZRESULT zr;

    zr = GetZipItem( _self->hzip, index, &ze );
    if ( entry && zr == ZR_OK )
    {
        entry->index = ze.index;
        entry->name = ze.name;
        entry->attr = ze.attr;
        entry->atime = ze.atime;
        entry->ctime = ze.ctime;
        entry->mtime = ze.mtime;
        entry->comp_size = ze.comp_size;
        entry->unc_size = ze.unc_size;
    }

    return zr;
}

ZRESULT Unzip::findEntry( String const & name, bool caseInsensitive, int * index, ZipEntry * entry )
{
    ZIPENTRY ze;
    ZRESULT zr;
    zr = FindZipItem( _self->hzip, name.c_str(), caseInsensitive, index, &ze );
    if ( entry && zr == ZR_OK )
    {
        entry->index = ze.index;
        entry->name = ze.name;
        entry->attr = ze.attr;
        entry->atime = ze.atime;
        entry->ctime = ze.ctime;
        entry->mtime = ze.mtime;
        entry->comp_size = ze.comp_size;
        entry->unc_size = ze.unc_size;
    }
    return zr;
}

ZRESULT Unzip::unzipEntry( int index, String const & outFilename )
{
    return UnzipItem( _self->hzip, index, outFilename.c_str() );
}

ZRESULT Unzip::unzipEntry( int index, void * buf, uint32 size )
{
    return UnzipItem( _self->hzip, index, buf, size );
}

void Unzip::unzipAll( String const & dirPath )
{
    if ( !dirPath.empty() )
    {
        this->setUnzipBaseDir(dirPath);
    }

    int nEntries = this->getEntriesCount();
    for ( int i = 0; i < nEntries; ++i )
    {
        ZipEntry ze;
        this->getEntry( i, &ze );
        this->unzipEntry( i, ze.name );
    }
}

ZRESULT Unzip::setUnzipBaseDir( String const & dirPath )
{
    return SetUnzipBaseDir( _self->hzip, dirPath.c_str() );
}

} // namespace winux

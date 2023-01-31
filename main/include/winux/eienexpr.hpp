#ifndef __EIENEXPR_HPP__
#define __EIENEXPR_HPP__

#include "winux.hpp"

/** \brief 表达式引擎，提供一种简单的动态解释执行的功能 */
namespace eienexpr
{
#ifdef EIENEXPR_DLL_USE
    #if defined(_MSC_VER) || defined(WIN32)
        #pragma warning( disable: 4251 )
        #ifdef EIENEXPR_DLL_EXPORTS
            #define EIENEXPR_DLL  __declspec(dllexport)
        #else
            #define EIENEXPR_DLL  __declspec(dllimport)
        #endif

        #define EIENEXPR_API __stdcall
    #else
        #define EIENEXPR_DLL
        #define EIENEXPR_API
    #endif
#else
    #define EIENEXPR_DLL
    #define EIENEXPR_API
#endif

#define EIENEXPR_FUNC_DECL(ret) EIENEXPR_DLL ret EIENEXPR_API
#define EIENEXPR_FUNC_IMPL(ret) ret EIENEXPR_API

/** \brief 表达式库错误 */
class EIENEXPR_DLL ExprError : public winux::Error
{
public:
    enum
    {
        eeNone,                ///< 无错误
        eeExprParseError,      ///< 表达式解析错误
        eeStringParseError,    ///< 字符串解析出错
        eeOperandTypeError,    ///< 操作数类型出错
        eeValueTypeError,      ///< 值类型出错
        eeFuncParamTypeError,  ///< 函数参数类型错误
        eeFuncParamCountError, ///< 函数参数个数错误
        eeVarCtxNotFound,      ///< 未关联变量场景
        eeVarNotFound,         ///< 变量未定义
        eeFuncNotFound,        ///< 函数未定义
        eeEvaluateFailed,      ///< 计算失败
        eeOutOfArrayBound      ///< 超出数组边界
    };

    ExprError( int errNo, winux::AnsiString const & err ) throw() : winux::Error( errNo, err ) { }
};

/** \brief 表达式原子 */
class EIENEXPR_DLL ExprAtom
{
public:
    enum ExprAtomType
    {
        eatOperator, ///< 操作符
        eatOperand   ///< 操作数
    };

    ExprAtom();
    virtual ~ExprAtom();

    /** \brief 原子类型 */
    ExprAtomType getAtomType() const { return this->_atomType; }

    /** \brief 克隆一个atom */
    virtual ExprAtom * clone() const = 0;

    /** \brief 字符串形式输出 */
    virtual winux::String toString() const = 0;

protected:
    ExprAtomType _atomType; // 表达式原子类型

    friend class ExprParser;
};

class ExprOperand;
class VarContext;
class ExprPackage;
class Expression;

/** \brief 表达式操作符 */
class EIENEXPR_DLL ExprOperator : public ExprAtom
{
public:
    // 操作符函数类型
    typedef bool (* OperatorFunction)( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data );

    /** \brief 仅是文本上判断是否有解析为操作符的可能性 */
    static bool Possibility( ExprPackage * package, winux::String const & str );

    ExprOperator( winux::String const & oprStr = "", bool isUnary = false, bool isRight = false, short level = 0, OperatorFunction oprFn = NULL );
    virtual ~ExprOperator();

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;

    /** \brief 操作符优先关系(低于:-1, 高于:1, 错误:-2)
     *
     *  看优先级别,如果本算符级别大于opr算符级别,则返回1,小于则返回-1
     *  (`以opr算符在本算符左边为依据: a opr b this_opr c`)若相同，则看opr的结合性，左结合返回-1，右结合返回1 */
    int nexus( ExprOperator const & opr ) const;

    bool isUnary() const { return _isUnary; }
    bool isRight() const { return _isRight; }
    winux::String const & getStr() const { return _oprStr; }

    /** \brief 判断两个算符是否相同 */
    bool operator == ( ExprOperator const & opr ) const
    {
        return (
            this->_oprStr == opr._oprStr &&
            this->_isUnary == opr._isUnary &&
            this->_level == opr._level &&
            this->_isRight == opr._isRight &&
            this->_oprFn == opr._oprFn
        );
    }

    /** \brief 判断两个算符是否结合性和优先级都相同 */
    bool isSameLevel( ExprOperator const & opr ) const { return _level == opr._level && _isRight == opr._isRight; }

protected:
    winux::String _oprStr; // 算符文本
    bool _isUnary; // 是否一元
    short _level;  // 优先级别
    bool _isRight;  // 是否右结合
    OperatorFunction _oprFn; // 对应函数

    friend class Expression;
    friend class ExprPackage;
    friend class ExprParser;
};

/** \brief 表达式操作数 */
class EIENEXPR_DLL ExprOperand : public ExprAtom
{
public:
    enum ExprOperandType
    {
        eotLiteral,    ///< 普通的字面值
        eotIdentifier, ///< 标识符（变量）
        eotReference,  ///< 内部引用
        eotFunction,   ///< 函数
        eotExpression  ///< 子表达式
    };

    ExprOperand();
    virtual ~ExprOperand();
    /** \brief 计算，结果不一定是值，还可以是其他操作数 */
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result ) = 0;
    /** \brief 把操作数计算成可用的值，如果不能算，则抛出异常 */
    winux::Mixed val();

    /** \brief 计算，直到结果是指定的操作数或Literal。
     *
     *  计算到遇到Literal为止。
     *  如果types[]不指定类型，则计算到Literal就返回true。
     *  如果types[]指定类型，则计算到指定类型就返回true，否则返回false。
     *  计算失败返回false。*/
    //bool evaluateUntil( ExprOperandType const * types, int n, winux::SimplePointer<ExprOperand> * result );

    /** \brief 求值取得Reference或Identifier，获取内部的Mixed指针 */
    bool evaluateMixedPtr( winux::Mixed ** ppv );

    /** \brief 取得操作数类型 */
    ExprOperandType getOperandType() const;

protected:
    ExprOperandType _operandType; // 操作数类型

    friend class ExprParser;
};

/** \brief 字面值操作数，无需计算 */
class EIENEXPR_DLL ExprLiteral : public ExprOperand
{
public:
    ExprLiteral();
    ExprLiteral( winux::Mixed const & val );
    virtual ~ExprLiteral();

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result );

    winux::Mixed::MixedType getValueType() const;
    winux::Mixed const & getValue() const;
    winux::Mixed & getValue();
    void setValue( winux::Mixed const & val );

    /** \brief 判断是否有解析为数字的可能性 */
    static bool NumberPossibility( winux::String const & str, bool * isFloat = NULL, bool * isExp = NULL/*, bool * isMinus = NULL*/ );
protected:
    winux::Mixed _val;

    friend class ExprParser;
};

/** \brief 标识符（变量）操作数 */
class EIENEXPR_DLL ExprIdentifier : public ExprOperand
{
public:
    ExprIdentifier( Expression * exprObj, winux::String const & name );
    virtual ~ExprIdentifier();

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result );

    winux::String const & getName() const { return _name; }

    /** \brief 设置变量值 */
    void setVar( winux::Mixed const & val );

    /** \brief 获取变量值 */
    winux::Mixed const & getVar() const;

    /** \brief 获取变量场景 */
    VarContext * getVarContext() const;

    /** \brief 获取关联的表达式对象 */
    Expression * getExprObj() const { return _exprObj; }

    static bool Possibility( winux::String const & str );

protected:
    Expression * _exprObj;  // 所属表达式对象
    winux::String _name; // 名称

    friend class ExprParser;
};

/** \brief 临时引用操作数 */
class EIENEXPR_DLL ExprReference : public ExprOperand
{
public:
    ExprReference( winux::Mixed & ref, winux::String const & syntax );
    virtual ~ExprReference();

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result );

    winux::Mixed & getRef() const { return *this->_ref; }
protected:

    winux::Mixed * _ref;
    winux::String _syntax;
};

/** \brief 函数操作数 */
class EIENEXPR_DLL ExprFunc : public ExprOperand
{
public:
    // 函数类型
    typedef bool (* FuncFunction)( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data );
    // 字符串=>函数映射
    typedef std::map< winux::String, FuncFunction > StringFuncMap;

    ExprFunc( Expression * exprObj, winux::String const & funcName );
    virtual ~ExprFunc();
    ExprFunc( ExprFunc const & other );
    ExprFunc & operator = ( ExprFunc const & other );

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result );

//protected:
    Expression * _exprObj;  ///< 所属表达式对象
    winux::String _funcName; ///< 函数名
    std::vector<Expression *> _params; ///< 参数，也是表达式

    /** \brief 释放参数内存 */
    void _clear();
    /** \brief 添加一个参数，param必须是new的或者clone()的 */
    void _addParam( Expression * param );


    friend class ExprParser;
};

/** \brief 表达式 */
class EIENEXPR_DLL Expression : public ExprOperand
{
public:
    Expression( ExprPackage * package, VarContext * ctx, Expression * parent, void * data );
    virtual ~Expression();
    Expression( Expression const & other );
    Expression & operator = ( Expression const & other );

    virtual ExprAtom * clone() const;
    virtual winux::String toString() const;
    virtual bool evaluate( winux::SimplePointer<ExprOperand> * result );

    winux::String toSuffixString() const;

    bool isEmpty() const { return this->_suffixAtoms.empty(); }

    /** \brief 获取表达式包 */
    ExprPackage * getPackage() const { return this->_package; }

    /** \brief 获取变量场景 */
    VarContext * getVarContext() const;

    /** \brief 取得外部数据对象指针 */
    void * getDataPtr() const;

    /** \brief 取得变量 */
    bool getVar( winux::String const & name, winux::Mixed * * outVarPtr, VarContext * * outVarCtx = nullptr ) const;
    /** \brief 取得变量值 */
    winux::Mixed const & getVar( winux::String const & name, VarContext * * outVarCtx = nullptr ) const;

    /** \brief 设置变量值 */
    void setVar( winux::String const & name, winux::Mixed const & val );

    /** \brief 清空所有表达式原子，可以再次参与解析 */
    void clear();

//protected:
    std::vector<ExprAtom *> _suffixAtoms; ///< 后缀式原子
    ExprPackage * _package; ///< 表达式包
    VarContext * _varCtx; ///< 变量场景
    Expression * _parent; ///< 父表达式
    void * _data; ///< 外部数据

    /** \brief 添加一个原子，atom必须是new的或者clone()的 */
    void _addAtom( ExprAtom * atom );

    friend class ExprIdentifier;
    friend class ExprParser;
};

/** \brief 变量场景类 */
class EIENEXPR_DLL VarContext
{
public:
    struct VariableStruct
    {
        winux::Mixed * p;
        bool isNewAlloc; ///< 是否为新分配的Mixed变量

        VariableStruct() : p(NULL), isNewAlloc(false)
        {
        }
    };

    VarContext( winux::Mixed * collection = nullptr );

    virtual ~VarContext();

    VarContext( VarContext const & other );

    VarContext & operator = ( VarContext const & other );

    /** \brief 设置一个Mixed(Collection)作为变量场景 */
    void setMixedCollection( winux::Mixed * collection );

    /** \brief 设置一个变量，并直接给值
     *
     *  如果不是设置Mixed(Collection)场景，内部会new一个Mixed。如果name表示的变量已存在，则直接引用其进行赋值操作
     *  如果是设置了Mixed(Collection)场景，内部会创建一对K/V。如果name表示的变量已存在，则直接引用其进行赋值操作 */
    void set( winux::String const & name, winux::Mixed const & v );

    /** \brief 设置一个变量，并返回其引用，方便自由修改
     *
     *  如果不是设置Mixed(Collection)场景，内部会new一个Mixed并返回其引用。如果name表示的变量已存在，则直接返回其引用
     *  如果是设置了Mixed(Collection)场景，内部会创建一对K/V并返回Value的引用。如果name表示的变量已存在，则直接返回其引用 */
    winux::Mixed & set( winux::String const & name );

    /** \brief 外部引用一个Mixed指针，要确保在表达式运行期间指针所指的内容存在
     *
     *  如果name表示的变量已存在，则删除后重新建立
     *  无论设不设置Mixed(Collection)场景都会删除后重新建立 */
    void setPtr( winux::String const & name, winux::Mixed * v );

    /** \brief 删除已经存在的名称，然后设置一个名称，并返回其指针引用以便赋值为其他外部引用的指针
     *
     *  如果name表示的变量已存在，则删除后重新建立
     *  无论设不设置Mixed(Collection)场景都会删除后重新建立 */
    winux::Mixed * & setPtr( winux::String const & name );

    bool unset( winux::String const & name );

    bool has( winux::String const & name ) const;

    bool get( winux::String const & name, winux::Mixed * * outVarPtr ) const;

    winux::Mixed const & get( winux::String const & name ) const;

    /** \brief 清空所有变量 */
    void clear();

    /** \brief 倾泻出所有变量 */
    winux::Mixed dump() const;

protected:
    winux::Mixed * _collection;
    std::map< winux::String, VariableStruct > _vars;

    friend class ExprParser;
};

/** \brief 表达式包（包含支持的算符和函数） */
class EIENEXPR_DLL ExprPackage
{
public:
    ExprPackage();

    /** \brief 仅从文本上判断是否有解析为操作符的可能性 */
    bool oprPossibility( winux::String const & str ) const;

    /** \brief 动态添加一个操作符 */
    void addOpr( winux::String const & oprStr, bool isUnary, bool isRight, short level, ExprOperator::OperatorFunction oprFn );
    /** \brief 动态移除一个操作符 */
    bool delOpr( winux::String const & oprStr, bool isUnary, bool isRight );
    /** \brief 动态修改一个操作符 */
    bool modifyOpr( int i, winux::String const & oprStr, bool isUnary, bool isRight, short level, ExprOperator::OperatorFunction oprFn );
    /** \brief 查找预定义的字符 */
    int findOpr( ExprOperator * opr, winux::String const & oprStr, bool isUnary = false, bool isRight = false ) const;

    /** \brief 取得指定字符串的所有操作符，返回取得的个数，当n==0，则只返回个数 */
    int getOpr( winux::String const & oprStr, ExprOperator * oprArr, int n ) const;
    /** \brief 取得所有操作符，返回取得的个数，当n==0，则只返回个数 */
    int getAllOprs( ExprOperator * oprArr, int n ) const;


    /** \brief 动态添加一个func */
    void setFunc( winux::String const & funcName, ExprFunc::FuncFunction fn );
    /** \brief 动态移除一个func */
    bool delFunc( winux::String const & funcName );
    /** \brief 动态修改一个func */
    bool modifyFunc( winux::String const & funcName, winux::String const & newFuncName, ExprFunc::FuncFunction newFn );
    /** \brief 查找预定义的func */
    bool findFunc( winux::String const & funcName, ExprFunc::FuncFunction * fn ) const;
    /** \brief 取得所有funcs */
    int getAllFuncs( std::vector< std::pair< winux::String, ExprFunc::FuncFunction > > * funcs ) const;

private:
    std::vector<ExprOperator> _operators; // 支持的运算符
    ExprFunc::StringFuncMap _funcsMap; // 支持的函数

    friend class ExprFunc;
    friend class Expression;
    friend class ExprParser;
};

/** \brief 解析器 */
class EIENEXPR_DLL ExprParser
{
public:
    ExprParser();
    virtual ~ExprParser();

    void parse( Expression * e, winux::String const & str );

    //winux::String const & error() const { return _errStr; }
    //int errNo() const { return _errNo; }
private:
    //int _errNo;
    //winux::String _errStr;
    enum ExprParseContext
    {
        epcExpr, epcFuncParams, epcString, epcStrAntiSlashes
    };
    void _parseExpr( Expression * e, winux::String const & str, int & i, std::vector<ExprParseContext> & epc );
    void _parseString( winux::String * v, winux::String const & str, int & i, std::vector<ExprParseContext> & epc );
    void _parseStrAntiSlashes( winux::String * v, winux::String const & str, int & i, std::vector<ExprParseContext> & epc );
    void _parseFuncParams( Expression * exprOwner, ExprFunc * funcAtom, winux::String const & str, int & i, std::vector<ExprParseContext> & epc );

};

}

#endif //__EIENEXPR_HPP__

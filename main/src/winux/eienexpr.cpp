#include "eienexpr.hpp"
#include "eienexpr_errstrs.inl"
#include <stack>
#include <cmath>

#include <iostream> // usage std::cout in FuncEcho()

#if defined(_MSC_VER)

#pragma warning( disable: 4146 )

#endif

namespace eienexpr
{
// macros definitions ---------------------------------------------------------------------
#define IS_OPERATOR(atom) ((atom)->getAtomType() == ExprAtom::eatOperator)

#define IS_OPERAND(atom) ((atom)->getAtomType() == ExprAtom::eatOperand)

#define IS_LITERAL(opd) ((opd)->getOperandType() == ExprOperand::eotLiteral)
#define IS_NOT_LITERAL(opd) ((opd)->getOperandType() != ExprOperand::eotLiteral)

#define IS_IDENTIFIER(opd) ((opd)->getOperandType() == ExprOperand::eotIdentifier)
#define IS_NOT_IDENTIFIER(opd) ((opd)->getOperandType() != ExprOperand::eotIdentifier)

#define IS_REFERENCE(opd) ((opd)->getOperandType() == ExprOperand::eotReference)
#define IS_NOT_REFERENCE(opd) ((opd)->getOperandType() != ExprOperand::eotReference)

#define IS_FUNCTION(opd) ((opd)->getOperandType() == ExprOperand::eotFunction)
#define IS_NOT_FUNCTION(opd) ((opd)->getOperandType() != ExprOperand::eotFunction)

#define IS_EXPRESSION(opd) ((opd)->getOperandType() == ExprOperand::eotExpression)
#define IS_NOT_EXPRESSION(opd) ((opd)->getOperandType() != ExprOperand::eotExpression)

#define GET_OPERATOR(atom) (static_cast<ExprOperator *>(atom))
#define GET_OPERAND(atom) (static_cast<ExprOperand *>(atom))
#define GET_LITERAL(opd) (static_cast<ExprLiteral *>(opd))
#define GET_IDENTIFIER(opd) (static_cast<ExprIdentifier *>(opd))
#define GET_REFERENCE(opd) (static_cast<ExprReference *>(opd))
#define GET_FUNCTION(opd) (static_cast<ExprFunc *>(opd))
#define GET_EXPRESSION(opd) (static_cast<Expression *>(opd))

// helper functions -----------------------------------------------------------------------
#include "is_x_funcs.inl"

inline static char StringToChar( char const * number, int base )
{
    char * endptr;
    return (char)strtol( number, &endptr, base );
}

// class ExprAtom -------------------------------------------------------------------------
ExprAtom::ExprAtom() : _atomType(eatOperand)
{
}

ExprAtom::~ExprAtom()
{
}

// class ExprOperator ---------------------------------------------------------------------
bool ExprOperator::Possibility( ExprPackage * package, winux::String const & str )
{
    return package->oprPossibility(str);
}

ExprOperator::ExprOperator( winux::String const & oprStr, bool isUnary, bool isRight, short level, OperatorFunction oprFn )
: _oprStr(oprStr), _isUnary(isUnary), _level(level), _isRight(isRight), _oprFn(oprFn)
{
    this->_atomType = ExprAtom::eatOperator;
}

ExprOperator::~ExprOperator()
{

}

ExprAtom * ExprOperator::clone() const
{
    return new ExprOperator(*this);
}

winux::String ExprOperator::toString() const
{
    if ( this->isUnary() )
    {
        if ( this->isRight() )
            return this->getStr();
        else
            return this->getStr();
    }
    else
    {
        if ( this->isRight() )
            return this->getStr();
        else
            return this->getStr();
    }
}

int ExprOperator::nexus( ExprOperator const & opr ) const
{
    if ( this->_level > 0 )
    {
        if ( opr._level > 0 )
        {
            // 看优先级别,如果本算符级别大于目标算符级别,则返回1,小于则返回-1
            // (`以opr算符在本算符左边为依据`)若相同，都是单目则看this结合性：左结合返回-1，右结合返回1；否则看opr结合性，左结合返回-1，右结合返回1。
            // 由于单目不一样,所以要单独判断
            if ( this->_isUnary && opr._isUnary )
            {
                if ( this->_level > opr._level )
                {
                    return 1;
                }
                else if ( this->_level < opr._level )
                {
                    return -1;
                }
                else // =
                {
                    if ( this->_isRight )//当前是右结合算符 ,如 -1
                    {//右结合单目算符左边不可能邻着一个左结合单目算符，因此返回-1没有意义
                        //if ( opr._isRight )
                            return 1;
                        //else
                        //    return -1;
                    }
                    else //当前是左结合算符 ，如阶乘 3!
                    {//左结合单目算符左边虽然可以邻着右结合单目算符，如-100#（#代表阶乘）,但显然应该从左往右先算-号,所以还是返回-1.
                        //if ( opr._isRight )
                        //    return -1;
                        //else
                            return -1;

                    }
                }
            }
            else
            {
                if ( this->_level > opr._level )
                {
                    return 1;
                }
                else if ( this->_level < opr._level )
                {
                    return -1;
                }
                else // =
                {
                    if ( opr._isRight )
                        return  1;
                    else
                        return  -1;
                }
            }
        }
    }
    return -2;
}

// class ExprOperand ----------------------------------------------------------------------
ExprOperand::ExprOperand()
{
    this->_atomType = ExprAtom::eatOperand;
    this->_operandType = eotLiteral;
}

ExprOperand::~ExprOperand()
{

}

ExprOperand::ExprOperandType ExprOperand::getOperandType() const
{
    return this->_operandType;
}

// 把操作数计算成可用的Mixed值，如果不能算，则抛出异常
winux::Mixed ExprOperand::val()
{
    winux::SimplePointer<ExprOperand> resOpd, tmpOpd;
    ExprOperand * opd = this;
    bool evalSuccess = true;
    // 一直evaluate()，直到得出Literal类型的结果。
    while ( IS_NOT_LITERAL(opd) && ( evalSuccess = opd->evaluate(&resOpd) ) )
    {
        tmpOpd = resOpd;
        opd = tmpOpd.get();
    }

    if ( !evalSuccess )
    {
        throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd->toString().c_str() ) );
    }

    return GET_LITERAL(opd)->getValue();
}

/*
bool ExprOperand::evaluateUntil( ExprOperandType const * types, int n, winux::SimplePointer<ExprOperand> * result )
{
    winux::SimplePointer<ExprOperand> & resOpd = *result; // 接受计算结果操作数
    winux::SimplePointer<ExprOperand> tmpOpd; // 记录上次结果

    ExprOperand * opd = this;
    bool evalSuccess = true; // 是否计算失败
    ExprOperandType resType; // 计算完的类型

    bool resTypeOk = false; // 结果类型符合

    // 一直evaluate()，直到得出指定类型的结果
    while ( true )
    {
        resType = opd->getOperandType();

        // 判断是否结果类型符合给定的目标类型
        for ( int i = 0; i < n; i++ )
        {
            if ( resType == types[i] )
            {
                resTypeOk = true;
                break;
            }
        }

        if (
            resTypeOk ||
            resType == eotLiteral ||
            !( evalSuccess = opd->evaluate(&resOpd) )
        ) break;

        tmpOpd = resOpd; // 记下结果，以免resOpd再次evaluate()赋值时被释放掉
        opd = tmpOpd.get();
    }

    return ( ( types == nullptr || n == 0 ) ? resType == eotLiteral : resTypeOk );
}*/

bool ExprOperand::evaluateMixedPtr( winux::Mixed ** ppv )
{
    winux::SimplePointer<eienexpr::ExprOperand> resOpd; // 接受计算结果操作数
    winux::SimplePointer<eienexpr::ExprOperand> tmpOpd; // 记录上次结果

    ExprOperand * opd = this;
    bool evalSuccess = true;
    eienexpr::ExprOperand::ExprOperandType resOpdType;

    // 一直evaluate()，直到得出指定类型的结果
    while ( true )
    {
        resOpdType = opd->getOperandType();

        if (
            resOpdType == eienexpr::ExprOperand::eotReference ||
            resOpdType == eienexpr::ExprOperand::eotIdentifier ||
            resOpdType == eienexpr::ExprOperand::eotLiteral ||
            !( evalSuccess = opd->evaluate(&resOpd) )
        ) break;

        tmpOpd = resOpd; // 记下结果，以免resOpd再次evaluate()赋值时被释放掉
        opd = tmpOpd.get();
    }

    switch ( resOpdType )
    {
    case eienexpr::ExprOperand::eotReference:
        {
            eienexpr::ExprReference * opdRef = static_cast<eienexpr::ExprReference *>(opd);
            *ppv = &opdRef->getRef();
            return true;
        }
        break;
    case eienexpr::ExprOperand::eotIdentifier:
        {
            eienexpr::ExprIdentifier * opdVar = static_cast<eienexpr::ExprIdentifier *>(opd);
            eienexpr::VarContext * ctx = opdVar->getVarContext();
            if ( ctx != NULL )
                return ctx->get( opdVar->getName(), ppv );
        }
        break;
    }

    return false;
}

// class ExprLiteral ----------------------------------------------------------------------
ExprLiteral::ExprLiteral()
{
    this->_operandType = ExprOperand::eotLiteral;
}
ExprLiteral::ExprLiteral( winux::Mixed const & val )
{
    this->_operandType = ExprOperand::eotLiteral;
    this->_val = val;
}

ExprLiteral::~ExprLiteral()
{

}

winux::Mixed::MixedType ExprLiteral::getValueType() const
{
    return this->_val._type;
}

winux::Mixed const & ExprLiteral::getValue() const
{
    return this->_val;
}

winux::Mixed & ExprLiteral::getValue()
{
    return this->_val;
}

void ExprLiteral::setValue( winux::Mixed const & val )
{
    this->_val = val;
}

ExprAtom * ExprLiteral::clone() const
{
    return new ExprLiteral(*this);
}

winux::String ExprLiteral::toString() const
{
    if ( this->getValue().isString() )
    {
        return "\"" + winux::AddCSlashes(this->getValue()) + "\"";
    }
    else
    {
        return this->getValue().toAnsi();
    }
}

bool ExprLiteral::evaluate( winux::SimplePointer<ExprOperand> * result )
{
    result->attachNew( new ExprLiteral(*this) );
    return true;
}

bool ExprLiteral::NumberPossibility( winux::String const & str, bool * isFloat, bool * isExp/*, bool * isMinus*/ )
{
    bool havePoint = false;
    bool haveExp = false;
    //bool haveMinus = false;
    int k;
    for ( k = 0; k < (int)str.length(); k++ )
    {
        if ( !IsDigit(str[k]) ) // 如果不是数字,则看看有没有其他属于数字的字符
        {
            if ( str[k] == '.' ) // 小数点
            {
                // 不能出现在第一个
                if ( k == 0 ) return false;
                // 若还没有小数点,则标记为已有
                if ( havePoint == false )
                {
                    havePoint = true;
                }
                else
                {
                    // 不可能有2个以上小数点
                    return false;
                }
            }
            else if ( str[k] == 'e' || str[k] == 'E' ) // 指数符号
            {
                // 不能出现在第一个
                if ( k == 0 ) return false;
                // 若还没有指数符号,则标记为已有
                if ( haveExp == false )
                {
                    haveExp = true;
                }
                else
                {
                    // 不可能有2个以上指数符号
                    return false;
                }
            }
            else if ( str[k] == '-' ) // 负号
            {
                /*if ( k == 0 ) // 第一个字符是负号
                {
                    if ( haveMinus == false )
                    {
                        haveMinus = true;
                    }
                }
                else // k > 0
                {
                    // 前一个必须是指数符号
                    if ( str[k - 1] != 'e' && str[k - 1] != 'E' ) return false;
                }*/

                // 不能出现在第一个
                if ( k == 0 ) return false;
                // 前一个必须是指数符号
                if ( str[k - 1] != 'e' && str[k - 1] != 'E' ) return false;
            }
            else // 说明含有其他非数字字符
            {
                return false;
            }
        }
    }
    ASSIGN_PTR(isFloat) = havePoint;
    ASSIGN_PTR(isExp) = haveExp;
    //ASSIGN_PTR(isMinus) = haveMinus;
    return str.length() > 0;
}

// class ExprIdentifier --------------------------------------------------------------------------
// 标识符可能性
bool ExprIdentifier::Possibility( winux::String const & str )
{
    int i = 0;
    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        bool isDigit = IsDigit(ch);
        if ( IsWord(ch) || isDigit )
        {
            if ( isDigit && i == 0 )
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        ++i;
    }

    return true;
}

ExprIdentifier::ExprIdentifier( Expression * exprObj, winux::String const & name )
{
    this->_exprObj = exprObj;
    this->_name = name;
    this->_operandType = ExprOperand::eotIdentifier;
}

ExprIdentifier::~ExprIdentifier()
{

}

ExprAtom * ExprIdentifier::clone() const
{
    return new ExprIdentifier(*this);
}

winux::String ExprIdentifier::toString() const
{
    return this->getName();
}

bool ExprIdentifier::evaluate( winux::SimplePointer<ExprOperand> * result )
{
    result->attachNew(NULL);

    winux::Mixed * v = nullptr;
    if ( this->_exprObj->getVar( this->_name, &v ) )
    {
        //result->attachNew( new ExprLiteral(*v) );
        result->attachNew( new ExprReference( *v, this->_name ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeVarNotFound, EXPRERRSTR_IDENTIFIER_NOT_DEFINED( this->_name.c_str() ) );
    }
    return false;
}

void ExprIdentifier::setVar( winux::Mixed const & val )
{
    this->_exprObj->setVar( this->_name, val );
}

winux::Mixed const & ExprIdentifier::getVar() const
{
    return this->_exprObj->getVar(this->_name);
}

VarContext * ExprIdentifier::getVarContext() const
{
    return this->_exprObj->getVarContext();
}

// class ExprReference --------------------------------------------------------------------
ExprReference::ExprReference( winux::Mixed & ref, winux::String const & syntax ) : _ref(&ref), _syntax(syntax)
{
    this->_operandType = ExprOperand::eotReference;
}

ExprReference::~ExprReference()
{

}

ExprAtom * ExprReference::clone() const
{
    return new ExprReference(*this);
}

winux::String ExprReference::toString() const
{
    return _syntax;
}

bool ExprReference::evaluate( winux::SimplePointer<ExprOperand> * result )
{
    result->attachNew(NULL);
    if ( this->_ref )
    {
        result->attachNew( new ExprLiteral(*this->_ref) );
        return true;
    }
    return false;
}

// class ExprFunc -------------------------------------------------------------------------
ExprFunc::ExprFunc( Expression * exprObj, winux::String const & funcName ) : _exprObj(exprObj), _funcName(funcName)
{
    this->_operandType = ExprOperand::eotFunction;
}

ExprFunc::~ExprFunc()
{
    this->_clear();
}

ExprFunc::ExprFunc( ExprFunc const & other )
{
    this->operator = (other);
}

ExprFunc & ExprFunc::operator = ( ExprFunc const & other )
{
    if ( this != &other )
    {
        this->_clear(); // 清空自身

        this->_atomType = other._atomType;
        this->_operandType = other._operandType;
        this->_exprObj = other._exprObj;
        this->_funcName = other._funcName;

        std::vector<Expression *>::const_iterator it;
        for ( it = other._params.begin(); it != other._params.end(); ++it )
        {
            this->_addParam( (Expression *)(*it)->clone() );
        }

    }
    return *this;
}

void ExprFunc::_clear()
{
    std::vector<Expression *>::iterator it;
    for ( it = this->_params.begin(); it != this->_params.end(); ++it )
    {
        if ( *it )
            delete *it;
    }
    this->_params.clear();
}

void ExprFunc::_addParam( Expression * param )
{
    this->_params.push_back(param);
}

ExprAtom * ExprFunc::clone() const
{
    return new ExprFunc(*this);
}

winux::String ExprFunc::toString() const
{
    winux::String str;
    str += this->_funcName + "( ";
    std::vector<Expression *>::const_iterator it = this->_params.begin();
    std::vector<Expression *>::const_iterator it0 = it;
    for ( ; it != this->_params.end(); ++it )
    {
        if ( it != it0 ) // when is not first
            str += ", ";
        str += (*it)->toString();
    }
    str += " )";
    return str;
}

bool ExprFunc::evaluate( winux::SimplePointer<ExprOperand> * result )
{
    result->attachNew(NULL);
    StringFuncMap & funcsMap = this->_exprObj->_package->_funcsMap;
    if ( winux::isset( funcsMap, this->_funcName ) )
        return funcsMap[this->_funcName]( this->_exprObj, this->_params, result, this->_exprObj->getDataPtr() );
    else
        throw ExprError( ExprError::eeFuncNotFound, EXPRERRSTR_FUNCTION_NOT_DEFINED( this->_funcName.c_str() ) );
    return false;
}

// class Expression -----------------------------------------------------------------------
Expression::Expression( ExprPackage * package, VarContext * ctx, Expression * parent, void * data )
{
    this->_operandType = ExprOperand::eotExpression;
    this->_package = package;
    this->_varCtx = ctx;
    this->_parent = parent;
    this->_data = data;
}

Expression::~Expression()
{
    this->clear();
}

Expression::Expression( Expression const & other )
{
    this->operator = (other);
}

Expression & Expression::operator = ( Expression const & other )
{
    if ( this != &other )
    {
        this->clear(); // 清空自身

        this->_atomType = other._atomType;
        this->_operandType = other._operandType;
        this->_varCtx = other._varCtx;
        this->_parent = other._parent;
        this->_data = other._data;

        std::vector<ExprAtom *>::const_iterator it;
        for ( it = other._suffixAtoms.begin(); it != other._suffixAtoms.end(); ++it )
        {
            this->_addAtom( (*it)->clone() );
        }

    }
    return *this;
}

void Expression::clear()
{
    std::vector<ExprAtom *>::iterator it;
    for ( it = this->_suffixAtoms.begin(); it != this->_suffixAtoms.end(); ++it )
    {
        if ( *it )
            delete *it;
    }
    this->_suffixAtoms.clear();
}

void Expression::_addAtom( ExprAtom * atom )
{
    this->_suffixAtoms.push_back(atom);
}

ExprAtom * Expression::clone() const
{
    return new Expression(*this);
}

static void __ExpandSuffixAtoms( Expression const & e, std::vector<ExprAtom *> * suffixAtoms )
{
    std::vector<ExprAtom *>::const_iterator it = e._suffixAtoms.begin();
    for ( ; it != e._suffixAtoms.end(); ++it )
    {
        ExprAtom * atom = *it;
        ExprOperand * opd;
        if ( atom->getAtomType() == ExprAtom::eatOperand && (opd = (ExprOperand *)atom)->getOperandType() == ExprOperand::eotExpression )
        {
            __ExpandSuffixAtoms( *(Expression *)opd, suffixAtoms );
        }
        else
        {
            suffixAtoms->push_back(*it);
        }
    }
}

struct EvalNode
{
    winux::String sub; // 子表达式串
    ExprOperator * opr; // 子表达式的算符

    explicit EvalNode( winux::String const & sub = "", ExprOperator * opr = nullptr ) :
        sub(sub),
        opr(opr)
    {
    }

    // left 表示操作数在左边还是右边
    winux::String toString( bool left, ExprOperator * curOpr )
    {
        winux::String r;
        if ( opr && !opr->isUnary() )
        {
            if ( opr->isSameLevel(*curOpr) )
            {
                // 在左边，左结合的算符不需要加括号，右结合的算符需要加括号 
                // 在右边，右结合的算符不需要加括号，左结合的算符需要加括号 
                if ( left )
                {
                    if ( !opr->isRight() )
                        r = sub;
                    else
                        r = "(" + sub + ")";
                }
                else
                {
                    if ( opr->isRight() )
                        r = sub;
                    else
                        r = "(" + sub + ")";
                }
            }
            else
            {
                if ( opr->nexus(*curOpr) < 0 )
                {
                    r = "(" + sub + ")";
                }
                else
                {
                    r = sub;
                }
            }
        }
        else
        {
            r = sub;
        }

        return r;
    }
};

winux::String __ExpressionToString( Expression const & e )
{
    std::vector<ExprAtom *> suffixAtoms;
    // 展开子表达式内的表达式原子
    __ExpandSuffixAtoms( e, &suffixAtoms );

    std::stack<EvalNode> rStack; // 结果栈
    EvalNode res;

    for ( std::vector<ExprAtom *>::const_iterator it = suffixAtoms.begin(); it != suffixAtoms.end(); ++it )
    {
        ExprAtom * atom = *it;
        if ( atom->getAtomType() == ExprAtom::eatOperand ) // 是操作数
        {
            ExprOperand * opd = static_cast<ExprOperand *>(atom);
            rStack.push( EvalNode( opd->toString(), nullptr ) );
        }
        else // 是操作符
        {
            ExprOperator * curOpr = static_cast<ExprOperator *>(atom);

            if ( curOpr->isUnary() ) // 一元
            {
                EvalNode opd1;
                if ( rStack.empty() )
                    break;

                opd1 = rStack.top();
                rStack.pop();

                if ( curOpr->isRight() )
                {
                    EvalNode eNode( curOpr->toString() + opd1.toString( false, curOpr ), curOpr );
                    rStack.push(eNode);
                }
                else
                {
                    EvalNode eNode( opd1.toString( true, curOpr ) + curOpr->toString(), curOpr );
                    rStack.push(eNode);
                }
            }
            else // 二元
            {
                EvalNode opd1, opd2;
                if ( rStack.empty() )
                    break;

                opd2 = rStack.top();
                rStack.pop();

                if ( rStack.empty() ) // 操作数不够，这可能是少输入操作数导致的
                {
                    rStack.push(opd2); // 把唯一的结果放回去，不然可能表达式没有结果了
                    break;
                }
                opd1 = rStack.top();
                rStack.pop();


                EvalNode eNode( opd1.toString( true, curOpr ) + " " + curOpr->toString() + " " + opd2.toString( false, curOpr ), curOpr );
                rStack.push(eNode);
            }
        }
    }

    if ( !rStack.empty() ) // 结果栈不空，顶元素即运算结果
    {
        res = rStack.top();
        rStack.pop();
    }

    return res.sub;
}

winux::String Expression::toString() const
{
    return __ExpressionToString(*this);
}

winux::String Expression::toSuffixString() const
{
    winux::String str;
    std::vector<ExprAtom *>::const_iterator it = this->_suffixAtoms.begin();
    std::vector<ExprAtom *>::const_iterator itBegin = it;
    for ( ; it != this->_suffixAtoms.end(); ++it )
    {
        if ( it != itBegin )
            str += " ";

        ExprAtom * atom = *it;
        if (
            atom->getAtomType() == ExprAtom::eatOperand &&
            GET_OPERAND(atom)->getOperandType() == ExprOperand::eotExpression
        )
        {
            str += GET_EXPRESSION(GET_OPERAND(atom))->toSuffixString();
        }
        else
        {
            str += atom->toString();
        }
    }
    return str;
}

// 计算表达式结果
bool Expression::evaluate( winux::SimplePointer<ExprOperand> * result )
{
    result->attachNew(NULL);
    std::stack< winux::SimplePointer<ExprOperand> > resStack; // 结果栈
    std::vector<ExprAtom *>::const_iterator it = this->_suffixAtoms.begin();
    for ( ; it != this->_suffixAtoms.end(); ++it )
    {
        ExprAtom * atom = *it;
        if ( atom->getAtomType() == ExprAtom::eatOperand ) // 操作数，存入结果栈中
        {
            resStack.push( winux::SimplePointer<ExprOperand>( static_cast<ExprOperand *>( atom->clone() ) ) );
        }
        else // 操作符
        {
            ExprOperator * opr = static_cast<ExprOperator *>(atom);
            if ( opr->isUnary() ) // 一元
            {
                winux::SimplePointer<ExprOperand> opd0;
                if ( resStack.empty() )
                    break;

                opd0 = resStack.top();
                resStack.pop();

                if ( !opr->_oprFn )
                    break;

                winux::SimplePointer<ExprOperand> opdResult; // 操作结果
                ExprOperand * arOperands[] = { opd0.get() };
                if ( !opr->_oprFn( this, arOperands, 1, &opdResult, this->getDataPtr() ) )
                    break;

                resStack.push(opdResult);
            }
            else // 二元
            {
                winux::SimplePointer<ExprOperand> opd0, opd1;
                if ( resStack.empty() )
                    break;
                opd1 = resStack.top();
                resStack.pop();
                if ( resStack.empty() ) // 操作数不够，这可能是少输入操作数导致的
                {
                    resStack.push(opd1); // 把唯一的结果放回去，不然可能表达式没有结果了
                    break;
                }
                opd0 = resStack.top();
                resStack.pop();

                if ( !opr->_oprFn )
                    break;

                winux::SimplePointer<ExprOperand> opdResult; // 操作结果
                ExprOperand * arOperands[] = { opd0.get(), opd1.get() };
                if ( !opr->_oprFn( this, arOperands, 2, &opdResult, this->getDataPtr() ) )
                    break;

                resStack.push(opdResult);
            }
        }
    }

    if ( !resStack.empty() ) // 结果栈不空，顶元素即运算结果
    {
        *result = resStack.top();
        resStack.pop();
    }
    else // 如果结果栈空，表达式也必须有一个结果
    {
        result->attachNew( new ExprLiteral() );
    }
    return true;
}

VarContext * Expression::getVarContext() const
{
    for ( Expression const * e = this; e != nullptr; e = e->_parent )
        if ( e->_varCtx != nullptr ) return e->_varCtx;

    return nullptr;
}

void * Expression::getDataPtr() const
{
    for ( Expression const * e = this; e != nullptr; e = e->_parent )
        if ( e->_data != nullptr ) return e->_data;

    return nullptr;
}

bool Expression::getVar( winux::String const & name, winux::Mixed * * outVarPtr, VarContext * * outVarCtx ) const
{
    bool b = false;
    
    for ( Expression const * e = this; e != nullptr; e = e->_parent )
    {
        ASSIGN_PTR(outVarCtx) = e->_varCtx;
        if ( e->_varCtx != nullptr )
            if ( b = e->_varCtx->get( name, outVarPtr ) ) break;
    }

    return b;
}

winux::Mixed const & Expression::getVar( winux::String const & name, VarContext * * outVarCtx ) const
{
    thread_local winux::Mixed inner;
    winux::Mixed * v = nullptr;
    return this->getVar( name, &v, outVarCtx ) ? *v : inner;
}

void Expression::setVar( winux::String const & name, winux::Mixed const & val )
{
    winux::Mixed * v = nullptr;
    if ( this->getVar( name, &v ) )
    {
        *v = val;
    }
    else
    {
        VarContext * varCtx = this->getVarContext();
        if ( varCtx )
        {
            varCtx->set( name, val );
        }
        else
        {
            throw ExprError( ExprError::eeVarCtxNotFound, EXPRERRSTR_NOT_ASSOC_VARCTX( this->toString().c_str() ) );
        }
    }
}

// class VarContext -----------------------------------------------------------------------
VarContext::VarContext( winux::Mixed * collection ) : _collection(nullptr)
{
    this->setMixedCollection(collection);
}

VarContext::~VarContext()
{
    this->clear();
}

VarContext::VarContext( VarContext const & other )
{
    this->operator = (other);
}

VarContext & VarContext::operator = ( VarContext const & other )
{
    if ( this != &other )
    {
        this->clear();

        std::map< winux::String, VariableStruct >::const_iterator it;
        for ( it = other._vars.begin(); it != other._vars.end(); ++it )
        {
            winux::String const & k = it->first;
            VariableStruct const & v = it->second;
            if ( v.isNewAlloc )
            {
                VariableStruct vNew;
                vNew.p = new winux::Mixed(*v.p);
                vNew.isNewAlloc = true;
                this->_vars[k] = vNew;
            }
            else
            {
                this->_vars[k] = v;
            }
        }
    }
    return *this;
}

void VarContext::setMixedCollection( winux::Mixed * collection )
{
    if ( collection && collection->isCollection() )
    {
        this->_collection = collection;

        size_t n = this->_collection->getCount();
        for ( size_t i = 0; i < n; ++i )
        {
            auto & kv = this->_collection->getPair(i);
            this->setPtr( kv.first, &kv.second );
        }
    }
    else
    {
        this->_collection = nullptr;
    }
}

void VarContext::set( winux::String const & name, winux::Mixed const & v )
{
    if ( this->has(name) )
    {
        *this->_vars[name].p = v;
    }
    else
    {
        if ( this->_collection )
        {
            VariableStruct var;
            var.p = &this->_collection->operator [] (name);
            *var.p = v;
            var.isNewAlloc = false;
            this->_vars[name] = var;
        }
        else
        {
            VariableStruct var;
            var.p = new winux::Mixed(v);
            var.isNewAlloc = true;
            this->_vars[name] = var;
        }
    }
}

winux::Mixed & VarContext::set( winux::String const & name )
{
    if ( this->has(name) )
    {
        return *this->_vars[name].p;
    }
    else
    {
        if ( this->_collection )
        {
            VariableStruct var;
            var.p = &this->_collection->operator [] (name);
            var.isNewAlloc = false;
            this->_vars[name] = var;
            return *this->_vars[name].p;
        }
        else
        {
            VariableStruct var;
            var.p = new winux::Mixed();
            var.isNewAlloc = true;
            this->_vars[name] = var;
            return *this->_vars[name].p;
        }
    }
}

void VarContext::setPtr( winux::String const & name, winux::Mixed * v )
{
    this->unset(name);

    VariableStruct var;
    var.p = v;
    var.isNewAlloc = false;
    this->_vars[name] = var;
}

winux::Mixed * & VarContext::setPtr( winux::String const & name )
{
    this->unset(name);

    VariableStruct var;
    var.p = NULL;
    var.isNewAlloc = false;
    this->_vars[name] = var;
    return this->_vars[name].p;
}

bool VarContext::unset( winux::String const & name )
{
    if ( this->has(name) )
    {
        VariableStruct & v = this->_vars.at(name);

        if ( v.isNewAlloc )
            delete v.p;

        this->_vars.erase(name);
        return true;
    }
    return false;
}

bool VarContext::has( winux::String const & name ) const
{
    return winux::isset( this->_vars, name );
}

bool VarContext::get( winux::String const & name, winux::Mixed * * outVarPtr ) const
{
    if ( this->has(name) )
    {
        *outVarPtr = this->_vars.at(name).p;
        return true;
    }
    return false;
}

winux::Mixed const & VarContext::get( winux::String const & name ) const
{
    thread_local winux::Mixed inner;
    if ( this->has(name) )
    {
        return *this->_vars.at(name).p;
    }
    return inner;
}

void VarContext::clear()
{
    std::map< winux::String, VariableStruct >::iterator it;
    for ( it = this->_vars.begin(); it != this->_vars.end(); ++it )
    {
        if ( it->second.isNewAlloc )
        {
            delete it->second.p;
        }
    }
    this->_vars.clear();
}

winux::Mixed VarContext::dump() const
{
    winux::Mixed result;
    result.createCollection();

    for ( auto it = _vars.begin(); it != _vars.end(); ++it )
    {
        result[it->first] = *it->second.p;
    }

    return result;
}

// built-in operators ---------------------------------------------------------------------

// 句点，用于Array/Collection类型访问容器元素 `arr.0` `coll.elem` `coll."key-name"`
static bool __Period( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) && IS_NOT_REFERENCE(opd0) ) // 如果第一操作数不是可操作的Identifier,Reference，就尝试计算是否能得到Identifier,Reference，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier,Reference
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) || IS_REFERENCE(evalOpd0) ) ) // 如果不能得到Identifier或Reference就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_INDEXABLE_OBJECT( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        if ( IS_IDENTIFIER(opd0) )
        {
            ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);

            winux::Mixed * target = NULL;
            if ( !e->getVar( v0->getName(), &target ) ) // 变量未定义
            {
                throw ExprError( ExprError::eeVarNotFound, EXPRERRSTR_IDENTIFIER_NOT_DEFINED( v0->toString().c_str() ) );
                return false;
            }
            if ( !target->isCollection() && !target->isArray() ) // 变量值不是collect/array类型
            {
                throw ExprError( ExprError::eeValueTypeError, EXPRERRSTR_ERROR_VALUE_TYPE( v0->toString().c_str(), "Collection/Array" ) );
                return false;
            }

            ExprOperand * opd1 = arOperands[1]; // 句点号右边的操作数
            winux::Mixed key;
            if ( IS_IDENTIFIER(opd1) ) // 是标识符，只用标识符名称做键值索引
            {
                ExprIdentifier * v1 = static_cast<ExprIdentifier *>(opd1);
                key = v1->getName();
            }
            else if ( IS_LITERAL(opd1) ) // 是字面量，用其值做键值索引
            {
                ExprLiteral * v1 = static_cast<ExprLiteral *>(opd1);
                key = v1->getValue();
            }
            else // 若是其他，则直接求其值做键值索引
            {
                key = opd1->val();
            }

            if ( target->isArray() && key.toInt() >= (int)target->_pArr->size() )
            {
                throw ExprError( ExprError::eeOutOfArrayBound, EXPRERRSTR_OUT_OF_ARRAY_BOUND( v0->toString().c_str(), (int)target->_pArr->size(), key ) );
                return false;
            }

            winux::Mixed & tmp = (winux::Mixed &)target->operator [] (key);
            outRetValue->attachNew( new ExprReference( tmp, v0->getName() + "." + key.json() ) );
            return true;

        }
        else if ( IS_REFERENCE(opd0) )
        {
            ExprReference * v0 = static_cast<ExprReference *>(opd0);
            winux::Mixed * target = &v0->getRef();

            if ( !target->isCollection() && !target->isArray() ) // 变量值不是collect/array类型
            {
                throw ExprError( ExprError::eeValueTypeError, EXPRERRSTR_ERROR_VALUE_TYPE( v0->toString().c_str(), "Collection/Array" ) );
                return false;
            }

            ExprOperand * opd1 = arOperands[1];
            winux::Mixed key;
            if ( IS_IDENTIFIER(opd1) )
            {
                ExprIdentifier * v1 = static_cast<ExprIdentifier *>(opd1);
                key = v1->getName();
            }
            else if ( IS_LITERAL(opd1) ) //是字面量，用其值做键值索引
            {
                ExprLiteral * v1 = static_cast<ExprLiteral *>(opd1);
                key = v1->getValue();
            }
            else
            {
                key = opd1->val();
            }

            if ( target->isArray() && key.toInt() >= (int)target->_pArr->size() )
            {
                throw ExprError( ExprError::eeOutOfArrayBound, EXPRERRSTR_OUT_OF_ARRAY_BOUND( v0->toString().c_str(), (int)target->_pArr->size(), key ) );
                return false;
            }

            winux::Mixed & tmp = (winux::Mixed &)target->operator [] (key);
            outRetValue->attachNew( new ExprReference( tmp, v0->toString() + "." + key.json() ) );
            return true;
        }
    }

    return false;
}

// 正号
static bool __Plus( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        winux::Mixed v = arOperands[0]->val();

        if ( v.isNumeric() )
        {
            outRetValue->attachNew( new ExprLiteral(v) );
            return true;
        }
    }
    return false;
}

// 负号
static bool __Minus( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        winux::Mixed v = arOperands[0]->val();

        if ( v.isNumeric() )
        {
            if ( v.isInteger() )
            {
                outRetValue->attachNew( new ExprLiteral( (winux::int64)-(winux::int64)v ) );
            }
            else
            {
                outRetValue->attachNew( new ExprLiteral( (double)-(double)v ) );
            }
            return true;
        }
    }
    return false;
}

// 加法
static bool __Add( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 + (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 + (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() + v1.toAnsi() ) );
            return true;
        }
        else if ( v0.isArray() || v0.isCollection() ) // 数组或集合
        {
            outRetValue->attachNew( new ExprLiteral( v0.merge(v1) ) );
            return true;
        }
    }
    return false;
}

// 字符串连结
static bool __Concat( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        outRetValue->attachNew( new ExprLiteral( v0.toAnsi() + v1.toAnsi() ) );
        return true;
    }
    return false;
}

// 减法
static bool __Subtract( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 - (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 - (double)v1 ) );

            return true;
        }
    }
    return false;
}

// 乘法
static bool __Multiply( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 * (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 * (double)v1 ) );

            return true;
        }
    }
    return false;
}

// 除法
static bool __Divide( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 / (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 / (double)v1 ) );

            return true;
        }
    }
    return false;
}

// 赋值
static bool __Assign( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) && IS_NOT_REFERENCE(opd0) ) // 如果赋值号左边不是可赋值的Identifier,Reference，就尝试计算是否能得到Identifier,Reference，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier,Reference
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) || IS_REFERENCE(evalOpd0) ) ) // 如果不能得到Identifier或Reference就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_EXPR_NOT_WRITEABLE( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        if ( IS_IDENTIFIER(opd0) )
        {
            ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);
            winux::Mixed v1 = arOperands[1]->val();
            v0->setVar(v1);
            outRetValue->attachNew( static_cast<ExprOperand *>( v0->clone() ) ); // 赋值操作符返回一个变量操作数
            return true;
        }
        else if ( IS_REFERENCE(opd0) )
        {
            ExprReference * v0 = static_cast<ExprReference *>(opd0);
            v0->getRef() = arOperands[1]->val();
            outRetValue->attachNew( static_cast<ExprOperand *>( v0->clone() ) ); // 赋值操作符返回一个变量操作数
            return true;
        }
    }

    return false;
}

// 逗号
static bool __Comma( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::SimplePointer<ExprOperand> v0;
        arOperands[0]->evaluate(&v0);
        arOperands[1]->evaluate(outRetValue);
        return true;
    }
    return false;
}

// 大于
static bool __Greater( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 > (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 > (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() > v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 小于
static bool __Less( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 < (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 < (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() < v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 大于等于
static bool __GreaterEqual( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 >= (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 >= (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() >= v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 小于等于
static bool __LessEqual( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 <= (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 <= (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() <= v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 不等于
static bool __NotEqual( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 != (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 != (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() != v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 等于
static bool __Equal( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            if ( v0.isInteger() && v1.isInteger() )
                outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 == (winux::int64)v1 ) );
            else
                outRetValue->attachNew( new ExprLiteral( (double)v0 == (double)v1 ) );

            return true;
        }
        else if ( v0.isString() ) // 字符串
        {
            outRetValue->attachNew( new ExprLiteral( v0.toAnsi() == v1.toAnsi() ) );
            return true;
        }
    }
    return false;
}

// 非
static bool __Not( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        winux::Mixed v = arOperands[0]->val();
        outRetValue->attachNew( new ExprLiteral( !(bool)v ) );
        return true;
    }
    return false;
}

// 且
static bool __And( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        // 逻辑与的断路写法，当v0为false，整个肯定为false，v1不用计算
        if ( (bool)v0 )
        {
            winux::Mixed v1 = arOperands[1]->val();
            outRetValue->attachNew( new ExprLiteral( (bool)v1 ) );
        }
        else
        {
            outRetValue->attachNew( new ExprLiteral(false) );
        }
        return true;
    }
    return false;

}

// 或
static bool __Or( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        // 逻辑或的断路写法，当v0为true，整个肯定为true，v1不用计算
        if ( (bool)v0 )
        {
            outRetValue->attachNew( new ExprLiteral(true) );
        }
        else
        {
            winux::Mixed v1 = arOperands[1]->val();
            outRetValue->attachNew( new ExprLiteral( (bool)v1 ) );
        }

        return true;
    }
    return false;
}

// 乘方
static bool __Power( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            outRetValue->attachNew( new ExprLiteral( pow( (double)v0, (double)v1 ) ) );
            return true;
        }
    }
    return false;

}

// 取模
static bool __Mod( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 % (winux::int64)v1 ) );
            return true;
        }
    }
    return false;

}

// 整除
static bool __IntDivide( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 2 )
    {
        winux::Mixed v0 = arOperands[0]->val();
        winux::Mixed v1 = arOperands[1]->val();

        if ( v0.isNumeric() ) // 数值
        {
            outRetValue->attachNew( new ExprLiteral( (winux::int64)v0 / (winux::int64)v1 ) );
            return true;
        }
    }
    return false;

}

// 右结合自增 ++i
static bool __Increase( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) ) // 如果操作数不是可赋值的Identifier，就尝试计算是否能得到Identifier，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) ) ) // 如果不能得到Identifier就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_EXPR_NOT_WRITEABLE( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);
        winux::int64 v = (winux::int64)v0->val();
        ++v;
        v0->setVar(v);
        outRetValue->attachNew( static_cast<ExprOperand *>( v0->clone() ) ); // 赋值操作符返回一个变量操作数
        return true;
    }
    return false;
}

// 右结合自减 --i
static bool __Decrease( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) ) // 如果操作数不是可赋值的Identifier，就尝试计算是否能得到Identifier，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) ) ) // 如果不能得到Identifier就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_EXPR_NOT_WRITEABLE( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);
        winux::int64 v = (winux::int64)v0->val();
        --v;
        v0->setVar(v);
        outRetValue->attachNew( static_cast<ExprOperand *>( v0->clone() ) ); // 赋值操作符返回一个变量操作数
        return true;
    }
    return false;
}

// 左结合自增 i++
static bool __Increase2( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) ) // 如果操作数不是可赋值的Identifier，就尝试计算是否能得到Identifier，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) ) ) // 如果不能得到Identifier就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_EXPR_NOT_WRITEABLE( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);
        winux::int64 v = (winux::int64)v0->val();
        outRetValue->attachNew( new ExprLiteral(v) ); // 赋值操作符返回一个变量操作数
        v++;
        v0->setVar(v);
        return true;
    }
    return false;
}

// 左结合自减 i--
static bool __Decrease2( Expression * e, ExprOperand * arOperands[], short n, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( n == 1 )
    {
        ExprOperand * opd0 = arOperands[0];
        winux::SimplePointer<ExprOperand> evalOpd0;

        if ( IS_NOT_IDENTIFIER(opd0) ) // 如果操作数不是可赋值的Identifier，就尝试计算是否能得到Identifier，如果不能就抛异常
        {
            if ( IS_LITERAL(opd0) ) // 如果是不能计算的字面值，抛出异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_NOT_VARIABLE( opd0->toString().c_str() ) );

            if ( !opd0->evaluate(&evalOpd0) ) // 尝试计算是否能得到Identifier
                throw ExprError( ExprError::eeEvaluateFailed, EXPRERRSTR_EVALUATE_FAILED( opd0->toString().c_str() ) );

            if ( !( IS_IDENTIFIER(evalOpd0) ) ) // 如果不能得到Identifier就抛异常
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_EXPR_NOT_WRITEABLE( opd0->toString().c_str() ) );

            opd0 = evalOpd0.get();
        }

        ExprIdentifier * v0 = static_cast<ExprIdentifier *>(opd0);
        winux::int64 v = (winux::int64)v0->val();
        outRetValue->attachNew( new ExprLiteral(v) ); // 赋值操作符返回一个变量操作数
        v--;
        v0->setVar(v);
        return true;
    }
    return false;
}


/*
    算符左右结合的区别：

    大多数二元算符都是左结合，即相同优先级从左往右计算。如 1+2-3+4，先算1+2，然后R1-3，最后R2+4
    例外，譬如赋值=是右结合，即相同优先级从右往左算。如 a=b=c=3，先算c=3，然后b=R1，最后a=R2
    大多数一元算符都是右结合，即相同优先级从右往左算。如~!-1，先算-1，然后!R1，最后~R2
    例外，譬如阶乘!是左结合，即相同优先级从左往右算。如3!!!，先算3!，然后R1!，最后R2!
 */

static ExprOperator __Operators[] = {
    ExprOperator( ".", false, false, 2000, __Period ),  // 句点
    ExprOperator( "+", true, true, 1000, __Plus ),  // 正号
    ExprOperator( "-", true, true, 1000, __Minus ), // 负号

    ExprOperator( "!", true, true, 1000, __Not ), // 逻辑取反
    //ExprOperator( "!"/*"#"*/, true, false, 1000, NULL ), // 阶乘，左结合

    ExprOperator( "++", true, true, 1000, __Increase ),  // 左自增 右结合
    ExprOperator( "++", true, false, 1000, __Increase2 ),  // 右自增 左结合
    ExprOperator( "--", true, true, 1000, __Decrease ),  // 左自减 右结合
    ExprOperator( "--", true, false, 1000, __Decrease2 ),  // 右自减 左结合

    ExprOperator( "**", false, false, 400, __Power ), // 乘方

    ExprOperator( "*", false, false, 300, __Multiply ), // 乘
    ExprOperator( "/", false, false, 300, __Divide ), // 除

    ExprOperator( "%", false, false, 300, __Mod ), // 取余
    ExprOperator( "\\", false, false, 300, __IntDivide ), // 整除

    ExprOperator( "+", false, false, 200, __Add ), // 加
    ExprOperator( "-", false, false, 200, __Subtract ), // 减

    ExprOperator( "&", false, false, 200, __Concat ), // 连结

    ExprOperator( ">", false, false, 180, __Greater ), // 大于
    ExprOperator( "<", false, false, 180, __Less ), // 小于
    ExprOperator( ">=", false, false, 180, __GreaterEqual ), // 大于等于
    ExprOperator( "<=", false, false, 180, __LessEqual ), // 小于等于
    ExprOperator( "!=", false, false, 180, __NotEqual ), // 不等于
    ExprOperator( "==", false, false, 180, __Equal ), // 等于

    ExprOperator( "&&", false, false, 175, __And ), // 逻辑且
    ExprOperator( "||", false, false, 170, __Or ), // 逻辑或

    ExprOperator( "=", false, true, 100, __Assign ), // 赋值号，右结合

    ExprOperator( ",", false, false, 1, __Comma ), // 逗号
};

// built-in functions ---------------------------------------------------------------------

// if( condition, true_expr[, false_expr] )，condition条件达成就执行true_expr，否则执行false_expr
static bool __FuncIf( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 1 )
    {
        winux::Mixed cdt = params[0]->val();

        if ( (bool)cdt )
        {
            return params[1]->evaluate(outRetValue);
        }
        else
        {
            if ( params.size() > 2 )
            {
                return params[2]->evaluate(outRetValue);
            }
            else
            {
                outRetValue->attachNew( new ExprLiteral() );
                return true;
            }
        }
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("if") );
    }
    return false;
}

// while( condition, expr, change_condition )，condition条件达成就执行expr，change_condition改变条件
static bool __FuncWhile( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 )
    {
        ExprLiteral * ret = new ExprLiteral();

        winux::Mixed & retMx = ret->getValue();
        retMx.createString();

        while ( (bool)params[0]->val() )
        {
            if ( params.size() > 1 ) // 有第二个参数
            {
                (*retMx._pStr) += params[1]->val().toAnsi();
            }
            if ( params.size() > 2 ) // 有第三个参数
            {
                params[2]->val();
            }
        }

        outRetValue->attachNew(ret);
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("while") );
    }
    return false;
}

// echo(str)，标准输出
static bool __FuncEcho( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral() );

        std::cout << params[0]->val();
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("echo") );
    }
    return false;
}

// 表达式里有且只有一个字面量值，则直接返回它，不用计算
static inline bool ___OnlyOneLiteralInExpr( Expression * expr, ExprLiteral * * literal )
{
    ExprOperand * opd = NULL;
    *literal = NULL;
    if (
        expr->_suffixAtoms.size() == 1 &&
        IS_OPERAND( opd = GET_OPERAND(expr->_suffixAtoms[0]) ) &&
        IS_LITERAL(opd)
    )
    {
        *literal = GET_LITERAL(opd);
        return true;
    }
    return false;
}

// json(str)，解析JSON字符串
static bool __FuncJson( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 ) // 至少需要1个参数
    {
        ExprLiteral * retVal = new ExprLiteral();
        ExprLiteral * opdLiteral = NULL;
        if ( ___OnlyOneLiteralInExpr( params[0], &opdLiteral ) )
        {
            retVal->getValue().json( opdLiteral->getValue().toAnsi() );
        }
        else
        {
            retVal->getValue().json( params[0]->val().toAnsi() );
        }

        outRetValue->attachNew(retVal);

        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("json") );
    }
    return false;
}

// tojson(mx)，将值转换成JSON字符串
static bool __FuncToJson( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 ) // 至少需要1个参数
    {
        ExprLiteral * retVal = new ExprLiteral();
        ExprLiteral * opdLiteral = NULL;
        if ( ___OnlyOneLiteralInExpr( params[0], &opdLiteral ) )
        {
            retVal->setValue( opdLiteral->getValue().json() );
        }
        else
        {
            retVal->setValue( params[0]->val().json() );
        }

        outRetValue->attachNew(retVal);
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("tojson") );
    }
    return false;
}

// arr(...)，构建一个数组
static bool __FuncArr( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    ExprLiteral * retVal = new ExprLiteral();
    winux::Mixed & mxRetVal = retVal->getValue();
    mxRetVal.createArray();

    int i, n = (int)params.size();
    for ( i = 0; i < n; ++i )
    {
        Expression * elemExpr = params[i];
        ExprLiteral * opdLiteral = NULL;
        if ( ___OnlyOneLiteralInExpr( elemExpr, &opdLiteral ) )
        {
            mxRetVal.add( opdLiteral->getValue() );
        }
        else
        {
            mxRetVal.add( elemExpr->val() );
        }
    }

    outRetValue->attachNew(retVal);
    return true;
}

// coll( arr( k, v ) ... )，构建一个collection
static bool __FuncColl( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    ExprLiteral * retVal = new ExprLiteral();
    winux::Mixed & mxRetVal = retVal->getValue();
    mxRetVal.createCollection();

    int i, n = (int)params.size();
    for ( i = 0; i < n; ++i )
    {
        Expression * pairExpr = params[i];
        winux::Mixed pairElem = pairExpr->val();
        if ( pairElem.isArray() )
        {
            if ( pairElem.getCount() > 1 )
            {
                mxRetVal[ pairElem[0] ] = pairElem[1];
            }
            else
            {
                mxRetVal[ pairElem[0] ] = winux::Mixed();
            }
        }
        else
        {
            mxRetVal[ pairElem ] = winux::Mixed();
        }
    }

    outRetValue->attachNew(retVal);
    return true;
}

// upper(s)
static bool __FuncStrUpper( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::StrUpper( params[0]->val() ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("upper") );
    }
    return false;
}

// lower(s)
static bool __FuncStrLower( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::StrLower( params[0]->val() ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("lower") );
    }
    return false;
}

// addslashes( str[, charlist = "\n\r\t\v\a\\\'\""] )
static bool __FuncAddSlashes( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 1 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::AddSlashes( params[0]->val(), params[1]->val() ) ) );
        return true;
    }
    else if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::AddSlashes( params[0]->val(), "\n\r\t\v\a\\\'\"" ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("addslashes") );
    }
    return false;
}

// stripslashes( str[, charlist = "\n\r\t\v\a\\\'\""] )
static bool __FuncStripSlashes( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 1 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::StripSlashes( params[0]->val(), params[1]->val() ) ) );
        return true;
    }
    else if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral( winux::StripSlashes( params[0]->val(), "\n\r\t\v\a\\\'\"" ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("stripslashes") );
    }
    return false;
}

// substr( str[, start[, count]] )
static bool __FuncSubStr( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 2 )
    {
        winux::String::size_type start = params[1]->val();
        outRetValue->attachNew( new ExprLiteral( ( start != winux::String::npos ? params[0]->val().toAnsi().substr( start,  params[2]->val() ) : "" ) ) );
        return true;
    }
    else if ( params.size() > 1 )
    {
        winux::String::size_type start = params[1]->val();
        outRetValue->attachNew( new ExprLiteral( ( start != winux::String::npos ? params[0]->val().toAnsi().substr(start) : "" ) ) );
        return true;
    }
    else if ( params.size() > 0 )
    {
        outRetValue->attachNew( new ExprLiteral( params[0]->val().toAnsi() ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("substr") );
    }
    return false;
}

// find( str, findstr[, start] )
static bool __FuncFind( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 2 )
    {
        outRetValue->attachNew( new ExprLiteral( (int)params[0]->val().toAnsi().find( params[1]->val().toAnsi(), params[2]->val().toInt() ) ) );
        return true;
    }
    else if ( params.size() > 1 )
    {
        outRetValue->attachNew( new ExprLiteral( (int)params[0]->val().toAnsi().find( params[1]->val().toAnsi() ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("find") );
    }
    return false;
}

// split( str, delimList[, always = false] )
static bool __FuncSplit( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 2 )
    {
        winux::StringArray arr;
        winux::StrSplit( params[0]->val(), params[1]->val(), &arr, params[2]->val().toBool() );
        outRetValue->attachNew( new ExprLiteral(arr) );
        return true;
    }
    else if ( params.size() > 1 )
    {
        winux::StringArray arr;
        winux::StrSplit( params[0]->val(), params[1]->val(), &arr );
        outRetValue->attachNew( new ExprLiteral(arr) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("split") );
    }
    return false;
}

// split2( str, delim[, always = false] )
static bool __FuncSplit2( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 2 )
    {
        winux::StringArray arr;
        winux::StrSplit2( params[0]->val(), params[1]->val(), &arr, params[2]->val().toBool() );
        outRetValue->attachNew( new ExprLiteral(arr) );
        return true;
    }
    else if ( params.size() > 1 )
    {
        winux::StringArray arr;
        winux::StrSplit2( params[0]->val(), params[1]->val(), &arr );
        outRetValue->attachNew( new ExprLiteral(arr) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("split") );
    }
    return false;
}

// join( arr[, delim = ""] )
static bool __FuncJoin( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 1 )
    {
        winux::StringArray arr;
        params[0]->val().getArray(&arr);
        outRetValue->attachNew( new ExprLiteral( winux::StrJoin( params[1]->val().toAnsi(), arr ) ) );
        return true;
    }
    else if ( params.size() > 0 )
    {
        winux::StringArray arr;
        params[0]->val().getArray(&arr);
        outRetValue->attachNew( new ExprLiteral( winux::StrJoin( "", arr ) ) );
        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("join") );
    }
    return false;
}

// var(varname) 取得变量引用
static bool __FuncVar( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);

    if ( params.size() > 0 ) // 至少需要1个参数
    {
        winux::String varName;
        if ( params[0]->_suffixAtoms.size() == 1 ) // 表达式只有一个ATOM
        {
            ExprAtom * atom = params[0]->_suffixAtoms[0];
            if ( IS_OPERAND(atom) ) // 是一个操作数
            {
                ExprOperand * opd = GET_OPERAND(atom);

                if ( IS_IDENTIFIER(opd) ) //是一个标识符
                {
                    ExprIdentifier * identifier = GET_IDENTIFIER(opd);
                    varName = identifier->getName();
                }
                else if ( IS_LITERAL(opd) ) //是一个字面值
                {
                    ExprLiteral * lit = GET_LITERAL(opd);
                    varName = lit->getValue().toAnsi();
                }
                else //是其他，如Expression, ExprFunc
                {
                    varName = params[0]->val().toAnsi();
                }
            }
            else // 是操作符
            {
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_OPERATOR_WRONG_PLACE_IN( "var", atom->toString().c_str() ) );
            }
        }
        else
        {
            varName = params[0]->val().toAnsi();
        }

        if ( !varName.empty() )
        {
            winux::Mixed * v = nullptr;
            if ( e->getVar( varName, &v ) )
            {
                outRetValue->attachNew( new ExprReference( *v, "var(" + varName + ")" ) );
            }
            else
            {
                VarContext * varCtx = e->getVarContext();
                if ( varCtx )
                {
                    outRetValue->attachNew( new ExprReference( varCtx->set(varName), "var(" + varName + ")" ) );
                }
                else
                {
                    throw ExprError( ExprError::eeVarCtxNotFound, EXPRERRSTR_NOT_ASSOC_VARCTX( e->toString().c_str() ) );
                }
            }
        }
        else
        {
            throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("var") );
        }

        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("var") );
    }

    return false;
}

// def( varname[, value] )
static bool __FuncDef( Expression * e, std::vector<Expression *> & params, winux::SimplePointer<ExprOperand> * outRetValue, void * data )
{
    outRetValue->attachNew(NULL);
    if ( params.size() > 0 ) // 至少需要1个参数
    {
        outRetValue->attachNew( new ExprLiteral() );

        winux::String varName;
        Expression * expr1 = params[0]; //第一个参数

        if ( expr1->_suffixAtoms.size() == 1 ) //表达式只有一个ATOM
        {
            ExprAtom * atom = expr1->_suffixAtoms[0];
            if ( IS_OPERAND(atom) )//是一个操作数
            {
                ExprOperand * opd = GET_OPERAND(atom);

                if ( IS_IDENTIFIER(opd) ) //是一个标识符
                {
                    ExprIdentifier * identifier = GET_IDENTIFIER(opd);
                    varName = identifier->getName();
                }
                else if ( IS_LITERAL(opd) ) //是一个字面值
                {
                    ExprLiteral * lit = GET_LITERAL(opd);
                    varName = lit->getValue().toAnsi();
                }
                else //是其他，如Expression, ExprFunc
                {
                    varName = params[0]->val().toAnsi();
                }
            }
            else
            {
                throw ExprError( ExprError::eeOperandTypeError, EXPRERRSTR_OPERATOR_WRONG_PLACE_IN( "def", atom->toString().c_str() ) );
            }
        }
        else
        {
            varName = params[0]->val().toAnsi();
        }


        if ( !varName.empty() )
        {
            winux::Mixed * v = nullptr;
            if ( e->getVar( varName, &v ) )
            {
                //outRetValue->attachNew( new ExprReference( *v, "var(" + varName + ")" ) );
                *v = params.size() > 1 ? params[1]->val() : winux::Mixed();
            }
            else
            {
                VarContext * varCtx = e->getVarContext();
                if ( varCtx )
                {
                    //outRetValue->attachNew( new ExprReference( varCtx->set(varName), "var(" + varName + ")" ) );
                    varCtx->set(varName) = params.size() > 1 ? params[1]->val() : winux::Mixed();
                }
                else
                {
                    throw ExprError( ExprError::eeVarCtxNotFound, EXPRERRSTR_NOT_ASSOC_VARCTX( e->toString().c_str() ) );
                }
            }
        }
        else
        {
            throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("def") );
        }

        return true;
    }
    else
    {
        throw ExprError( ExprError::eeFuncParamCountError, EXPRERRSTR_NOT_ENOUGH_PARAMETERS("def") );
    }
    return false;
}

ExprFunc::StringFuncMap::value_type __Funcs[] = {
    ExprFunc::StringFuncMap::value_type( "if", __FuncIf ),
    ExprFunc::StringFuncMap::value_type( "while", __FuncWhile ),
    ExprFunc::StringFuncMap::value_type( "echo", __FuncEcho ),
    ExprFunc::StringFuncMap::value_type( "json", __FuncJson ),
    ExprFunc::StringFuncMap::value_type( "tojson", __FuncToJson ),
    ExprFunc::StringFuncMap::value_type( "arr", __FuncArr ),
    ExprFunc::StringFuncMap::value_type( "coll", __FuncColl ),
    ExprFunc::StringFuncMap::value_type( "upper", __FuncStrUpper ),
    ExprFunc::StringFuncMap::value_type( "lower", __FuncStrLower ),
    ExprFunc::StringFuncMap::value_type( "addslashes", __FuncAddSlashes ),
    ExprFunc::StringFuncMap::value_type( "stripslashes", __FuncStripSlashes ),
    ExprFunc::StringFuncMap::value_type( "substr", __FuncSubStr ),
    ExprFunc::StringFuncMap::value_type( "find", __FuncFind ),
    ExprFunc::StringFuncMap::value_type( "split", __FuncSplit ),
    ExprFunc::StringFuncMap::value_type( "split2", __FuncSplit2 ),
    ExprFunc::StringFuncMap::value_type( "join", __FuncJoin ),
    ExprFunc::StringFuncMap::value_type( "def", __FuncDef ),
    ExprFunc::StringFuncMap::value_type( "var", __FuncVar ),
};

// class ExprPackage ----------------------------------------------------------------------
ExprPackage::ExprPackage() :
    _operators( __Operators, __Operators + countof(__Operators) ),
    _funcsMap( __Funcs, __Funcs + countof(__Funcs) )
{
    // 初始化内建算符和内建函数


}

bool ExprPackage::oprPossibility( winux::String const & str ) const
{
    int i;
    for ( i = 0; i < (int)_operators.size(); i++ )
    {
        if ( _operators[i]._oprStr.find(str) != winux::String::npos )
        {
            return true;
        }
    }

    return false;
}

void ExprPackage::addOpr( winux::String const & oprStr, bool isUnary, bool isRight, short level, ExprOperator::OperatorFunction oprFn )
{
    _operators.push_back( ExprOperator( oprStr, isUnary, isRight, level, oprFn ) );
}

bool ExprPackage::delOpr( winux::String const & oprStr, bool isUnary, bool isRight )
{
    int i = this->findOpr( NULL, oprStr, isUnary, isRight );
    if ( i == -1 ) return false;
    _operators.erase( _operators.begin() + i );
    return true;
}

bool ExprPackage::modifyOpr( int i, winux::String const & oprStr, bool isUnary, bool isRight, short level, ExprOperator::OperatorFunction oprFn )
{
    if ( i < 0 || i > (int)_operators.size() - 1 ) return false;
    ExprOperator & opr = _operators[i];
    opr._oprStr = oprStr;
    opr._isUnary = isUnary;
    opr._isRight = isRight;
    opr._level = level;
    opr._oprFn = oprFn;
    return true;
}

int ExprPackage::findOpr( ExprOperator * opr, winux::String const & oprStr, bool isUnary, bool isRight ) const
{
    int i;
    for ( i = 0; i < (int)_operators.size(); ++i )
    {
        ExprOperator const & o = _operators[i];
        if ( oprStr == o._oprStr && isUnary == o._isUnary && isRight == o._isRight )
        {
            ASSIGN_PTR(opr) = o;
            return i;
        }
    }
    return -1;
}

int ExprPackage::getOpr( winux::String const & oprStr, ExprOperator * oprArr, int n ) const
{
    int i;
    int count = 0;
    for ( i = 0; i < (int)_operators.size(); ++i )
    {
        ExprOperator const & o = _operators[i];
        if ( oprStr == o._oprStr )
        {
            if ( count < n )
            {
                if ( oprArr ) oprArr[count] = o;
            }
            ++count;
        }
    }
    return count;
}

int ExprPackage::getAllOprs( ExprOperator * oprArr, int n ) const
{
    int i;
    int count = 0;
    for ( i = 0; i < (int)_operators.size(); ++i )
    {
        if ( count < n )
        {
            if ( oprArr ) oprArr[count] = _operators[i];
        }
        ++count;
    }
    return count;
}

void ExprPackage::setFunc( winux::String const & funcName, ExprFunc::FuncFunction fn )
{
    _funcsMap[funcName] = fn;
}

bool ExprPackage::delFunc( winux::String const & funcName )
{
    if ( winux::isset( _funcsMap, funcName ) )
    {
        _funcsMap.erase(funcName);
        return true;
    }
    return false;
}

bool ExprPackage::modifyFunc( winux::String const & funcName, winux::String const & newFuncName, ExprFunc::FuncFunction newFn )
{
    if ( winux::isset( _funcsMap, funcName ) )
    {
        _funcsMap.erase(funcName);
        this->setFunc( newFuncName, newFn );
        return true;
    }
    return false;
}

bool ExprPackage::findFunc( winux::String const & funcName, ExprFunc::FuncFunction * fn ) const
{
    if ( winux::isset( _funcsMap, funcName ) )
    {
        ASSIGN_PTR(fn) = _funcsMap.at(funcName);
        return true;
    }
    return false;

}

int ExprPackage::getAllFuncs( std::vector< std::pair< winux::String, ExprFunc::FuncFunction > > * funcs ) const
{
    ExprFunc::StringFuncMap::const_iterator it;
    int count = 0;
    for ( it = _funcsMap.begin(); it != _funcsMap.end(); ++it, ++count )
    {
        IF_PTR(funcs)->push_back(*it);
    }
    return count;
}

// class ExprParser -----------------------------------------------------------------------
ExprParser::ExprParser()
{

}

ExprParser::~ExprParser()
{

}

// 判断ATOM类型所需的上下文变量
struct __JudgeContext
{
    bool isOprPrevPossibility; //前一次是否可能是操作符
    bool isNumPrevPossibility; //前一次是否可能是数字
    bool isNumFloatPrev; // 数字是否小数点
    bool isNumExpPrev; //数字是否含指数符
    bool isIdPrevPossibility;  //前一次是否可能是标识符

    winux::String strTmp; //临时存下的内容

    ExprAtom * prevAtom; // 指向上一个原子
    std::stack<ExprOperator *> oprStack; // 算符栈

    __JudgeContext()
    {
        isOprPrevPossibility = false;
        isNumPrevPossibility = false;
        isNumFloatPrev = false;
        isNumExpPrev = false;
        isIdPrevPossibility = false;

        prevAtom = NULL;
    }

    void reset( ExprAtom * prevAtom = NULL )
    {
        this->isOprPrevPossibility = false;
        this->isNumPrevPossibility = false;
        this->isNumFloatPrev = false;
        this->isNumExpPrev = false;
        this->isIdPrevPossibility = false;

        this->strTmp.clear();
        this->prevAtom = prevAtom;
        while( !this->oprStack.empty() ) this->oprStack.pop();
    }
};

// 添加Atom到后缀式
static void __AddAtomToSuffix( Expression * e, __JudgeContext * ctx, ExprAtom * atom )
{
    switch ( atom->getAtomType() )
    {
    case ExprAtom::eatOperand:
        e->_addAtom(atom);
        break;
    case ExprAtom::eatOperator:
        {
            ExprOperator * curOpr = static_cast<ExprOperator *>(atom);
            while ( !ctx->oprStack.empty() && curOpr->nexus( *ctx->oprStack.top() ) < 0 )
            {
                e->_addAtom( ctx->oprStack.top() );
                ctx->oprStack.pop();
            }
            ctx->oprStack.push(curOpr);
        }
        break;
    }

    ctx->prevAtom = atom;
}

// 把算符栈里的算符发到后缀式
static void __PopStackOprToSuffix( Expression * e, __JudgeContext * ctx )
{
    while ( !ctx->oprStack.empty() )
    {
        e->_addAtom( ctx->oprStack.top() );
        ctx->oprStack.pop();
    }
}

// 判断能不能是操作数
static void __IsOkAsOperand( __JudgeContext * ctx, winux::String const & tmp, int pos )
{
    if ( ctx->prevAtom )
    {
        if ( ctx->prevAtom->getAtomType() == ExprAtom::eatOperand )
        {
            throw ExprError( ExprError::eeExprParseError, EXPRERRSTR_OPERAND_WRONG_PLACE_IMPOSSIBLE_CONTINUOUS_OPERAND( tmp.c_str(), pos ) );
        }
        else if ( ctx->prevAtom->getAtomType() == ExprAtom::eatOperator )
        {
            ExprOperator * prevOpr = static_cast<ExprOperator *>(ctx->prevAtom);
            if ( prevOpr->isUnary() && !prevOpr->isRight() )
            {
                //单目左结合算符右边不可能是操作数
                throw ExprError( ExprError::eeExprParseError, EXPRERRSTR_OPERAND_WRONG_PLACE_IMPOSSIBLE_OPERAND_ATRIGHTOF_UNARYLEFTASSOC_OPR( tmp.c_str(), pos ) );
            }
        }
    }
}

// 判断是操作符，数字，还是标识符
static void __JudgeOprOrNumOrId( Expression * e, __JudgeContext * ctx, int pos )
{
    ExprPackage * package = e->getPackage();
    if ( !ctx->strTmp.empty() )
    {
        if ( ctx->isOprPrevPossibility )
        {
            //如果prevAtom是空
            //  则这个操作符可能是单目右结合算符，搜索它，找到则添加到Atom数组中，找不到，抛出eeExprParseError异常；
            //prevAtom不空,如果前一个atom是操作数
            //  则当前操作符可能是双目算符或者单目左结合算符，搜索他们，找到则添加到Atom数组中，找不到，抛出eeExprParseError异常；
            //前一个atom是操作符
            //  若是单目右结合算符或者双目算符，则当前操作符只可能是单目右结合，搜索他，找到则添加到Atom数组中，找不到，抛出eeExprParseError异常；
            //  若是单目左结合算符，则当前操作符可能是双目算符或者单目左结合算符，搜索他们，找到则添加到Atom数组中，找不到，抛出eeExprParseError异常；

            ExprAtom * curOpr = NULL;
            ExprOperator opr;

            if ( ctx->prevAtom == NULL )
            {
                if ( package->findOpr( &opr, ctx->strTmp, true, true ) != -1 )
                {
                    curOpr = opr.clone();
                }
            }
            else
            {
                if ( ctx->prevAtom->getAtomType() == ExprAtom::eatOperand )
                {
                    if (
                        package->findOpr( &opr, ctx->strTmp, true, false ) != -1 ||
                        package->findOpr( &opr, ctx->strTmp, false, false ) != -1 ||
                        package->findOpr( &opr, ctx->strTmp, false, true ) != -1
                    )
                    {
                        curOpr = opr.clone();
                    }
                }
                else if ( ctx->prevAtom->getAtomType() == ExprAtom::eatOperator )
                {
                    ExprOperator * prevOpr = static_cast<ExprOperator *>(ctx->prevAtom);

                    if ( ( prevOpr->isUnary() && prevOpr->isRight() ) || !prevOpr->isUnary() )//若上一个是单目右结合算符或者双目算符
                    {
                        if ( package->findOpr( &opr, ctx->strTmp, true, true ) != -1 )
                        {
                            curOpr = opr.clone();
                        }
                    }
                    else if ( prevOpr->isUnary() && !prevOpr->isRight() ) //上一次是单目左结合算符
                    {
                        if (
                            package->findOpr( &opr, ctx->strTmp, true, false ) != -1 ||
                            package->findOpr( &opr, ctx->strTmp, false, false ) != -1 ||
                            package->findOpr( &opr, ctx->strTmp, false, true ) != -1
                        )
                        {
                            curOpr = opr.clone();
                        }
                    }
                }
            }

            if ( curOpr != NULL )
            {
                __AddAtomToSuffix( e, ctx, curOpr );
            }
            else
            {
                throw ExprError( ExprError::eeExprParseError, EXPRERRSTR_OPERATOR_WRONG_PLACE( ctx->strTmp.c_str(), pos ) );
            }
        }
        else if ( ctx->isNumPrevPossibility )
        {
            __IsOkAsOperand( ctx, ctx->strTmp, pos );

            ExprLiteral * numAtom = NULL;
            if ( ctx->isNumFloatPrev || ctx->isNumExpPrev ) // 浮点数
            {
                double dbl = 0.0;
                winux::Mixed::ParseDouble( ctx->strTmp, &dbl );
                numAtom = new ExprLiteral(dbl);
            }
            else // 整数
            {
                winux::uint64 ui64 = 0;
                winux::Mixed::ParseUInt64( ctx->strTmp, &ui64 );

                winux::Mixed val;
                if ( ctx->strTmp.length() < 10 ) // int
                {
                    val.assign( (int)ui64 );
                }
                else // int64
                {
                    val.assign( (winux::int64)ui64 );
                }

                numAtom = new ExprLiteral(val);
            }

            __AddAtomToSuffix( e, ctx, numAtom );
        }
        else // if ctx->isIdPrevPossibility
        {
            __IsOkAsOperand( ctx, ctx->strTmp, pos );

            ExprIdentifier * idAtom = new ExprIdentifier( e, ctx->strTmp );

            __AddAtomToSuffix( e, ctx, idAtom );
        }

        ctx->strTmp.clear();
    }
}

void ExprParser::parse( Expression * e, winux::String const & str )
{
    std::vector<ExprParseContext> epc; // 表达式解析场景
    int i = 0;
    epc.push_back(epcExpr);
    _parseExpr( e, str, i, epc );
    epc.pop_back();
}

void ExprParser::_parseExpr( Expression * e, winux::String const & str, int & i, std::vector<ExprParseContext> & epc )
{
    __JudgeContext jctx; // 判断Atom需要的上下文变量

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];

        if ( IsSpace(ch) ) // 跳过空白字符
        {
            __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );

            ++i;
        }
        else if ( ch == '\"' || ch == '\'' ) // 字符串
        {
            __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );

            int iStart = i;
            winux::String v;
            epc.push_back(epcString);
            _parseString( &v, str, i, epc );

            __IsOkAsOperand( &jctx, str.substr( iStart, i - iStart ), iStart ); //判断当前atom是否能作为操作数

            ExprLiteral * strAtom = new ExprLiteral(v);

            __AddAtomToSuffix( e, &jctx, strAtom );
        }
        else if ( ch == '(' ) // 一个子表达式或者函数开始
        {
            __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );

            if (
                jctx.prevAtom &&
                jctx.prevAtom->getAtomType() == ExprAtom::eatOperand &&
                static_cast<ExprOperand *>(jctx.prevAtom)->getOperandType() == ExprOperand::eotIdentifier
            ) // 前一个原子是标识符，视作函数处理
            {
                ExprIdentifier * idAtom = static_cast<ExprIdentifier *>( e->_suffixAtoms.back() );
                e->_suffixAtoms.pop_back(); // 弹出原先的标识符原子
                winux::String funcName = idAtom->getName(); // 标识符名作为函数名
                delete idAtom; // 删除这个标识符原子

                ExprFunc * funcAtom = new ExprFunc( e, funcName );
                epc.push_back(epcFuncParams);
                ++i; // skip `(`
                //进入函数参数解析
                _parseFuncParams( e, funcAtom, str, i, epc );

                __AddAtomToSuffix( e, &jctx, funcAtom );

            }
            else // 前一个原子不是标识符，视作一个子表达式操作数处理
            {
                __IsOkAsOperand( &jctx, "(...)", i ); //判断当前atom是否能作为操作数

                Expression * subExpr = new Expression( e->_package, NULL, e, NULL );
                epc.push_back(epcExpr); // 新一个子表达式
                ++i; // skip `(`
                _parseExpr( subExpr, str, i, epc );

                __AddAtomToSuffix( e, &jctx, subExpr );

            }
        }
        else if ( ch == ')' ) //子表达式结束的标记
        {
            __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );

            epc.pop_back(); // 弹出子表达式解析场景epcExpr
            ++i; // skip `)`
            break;
        }
        else // 算符，数字，标识符
        {
            bool isOprPossibility = true;
            bool isNumPossibility = true;
            bool isNumFloat = false;
            bool isNumExp = false;
            bool isIdPossibility = true;
            isOprPossibility = ExprOperator::Possibility( e->_package, jctx.strTmp + ch );
            isNumPossibility = ExprLiteral::NumberPossibility( jctx.strTmp + ch, &isNumFloat, &isNumExp );
            isIdPossibility = ExprIdentifier::Possibility( jctx.strTmp + ch );

            if ( !isOprPossibility && !isNumPossibility && !isIdPossibility )
            {
                if ( !jctx.strTmp.empty() )//可能性都不存在，存储的临时内容不空，那就说明上一次肯定有识别成功一个东西
                {
                    __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );
                }
                else // 可能性都不存在，存储的临时内容又是空，那就说明暂不能识别这个字符，跳过它
                {
                    ++i;
                }
            }
            else
            {
                jctx.isOprPrevPossibility = isOprPossibility;
                jctx.isNumPrevPossibility = isNumPossibility;
                jctx.isNumFloatPrev = isNumFloat;
                jctx.isNumExpPrev = isNumExp;
                jctx.isIdPrevPossibility = isIdPossibility;

                jctx.strTmp += ch;
                ++i;
            }

        }
    }

    __JudgeOprOrNumOrId( e, &jctx, i - (int)jctx.strTmp.length() );
    __PopStackOprToSuffix( e, &jctx );

}

// 解析函数参数，大体上和解析表达式差不多，但要考虑逗号分隔参数
void ExprParser::_parseFuncParams( Expression * exprOwner, ExprFunc * funcAtom, winux::String const & str, int & i, std::vector<ExprParseContext> & epc )
{
    Expression * eParam = new Expression( exprOwner->_package, NULL, exprOwner, NULL );

    __JudgeContext jctx; // 判断Atom需要的上下文变量

    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];

        if ( IsSpace(ch) ) // 跳过空白字符
        {
            __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );

            ++i; // skip `space-char`
        }
        else if ( ch == ',' ) // 另一个参数
        {
            __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );
            __PopStackOprToSuffix( eParam, &jctx );

            if ( eParam->isEmpty() )
            {
                delete eParam;
            }
            else
            {
                funcAtom->_addParam(eParam);
            }

            ++i; // skip `,`

            // 另起一个新参数
            eParam = new Expression( exprOwner->_package, NULL, exprOwner, NULL );
            jctx.reset();
        }
        else if ( ch == '\"' || ch == '\'' ) // 字符串
        {
            __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );

            int iStart = i;
            winux::String v;
            epc.push_back(epcString);
            _parseString( &v, str, i, epc );

            __IsOkAsOperand( &jctx, str.substr( iStart, i - iStart ), iStart ); //判断当前atom是否能作为操作数

            ExprLiteral * strAtom = new ExprLiteral(v);

            __AddAtomToSuffix( eParam, &jctx, strAtom );

        }
        else if ( ch == '(' ) // 一个子表达式或者函数开始
        {
            __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );

            if (
                jctx.prevAtom &&
                jctx.prevAtom->getAtomType() == ExprAtom::eatOperand &&
                static_cast<ExprOperand *>(jctx.prevAtom)->getOperandType() == ExprOperand::eotIdentifier
            ) // 前一个原子是标识符，视作函数处理
            {
                ExprIdentifier * idAtom = static_cast<ExprIdentifier *>( eParam->_suffixAtoms.back() );
                eParam->_suffixAtoms.pop_back(); // 弹出原先的标识符原子
                winux::String funcName = idAtom->getName(); // 标识符名作为函数名
                delete idAtom; // 删除这个标识符原子

                ExprFunc * funcAtom = new ExprFunc( eParam, funcName );
                epc.push_back(epcFuncParams);
                ++i; // skip `(`
                _parseFuncParams( eParam, funcAtom, str, i, epc );

                __AddAtomToSuffix( eParam, &jctx, funcAtom );

            }
            else // 前一个原子不是标识符，视作一个子表达式操作数处理
            {
                __IsOkAsOperand( &jctx, "(...)", i ); //判断当前atom是否能作为操作数

                Expression * subExpr = new Expression( exprOwner->_package, NULL, eParam, NULL );
                epc.push_back(epcExpr); // 新一个子表达式
                ++i; // skip `(`
                _parseExpr( subExpr, str, i, epc );

                __AddAtomToSuffix( eParam, &jctx, subExpr );

            }
        }
        else if ( ch == ')' ) //函数参数结束的标记
        {
            __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );

            epc.pop_back(); // 弹出函数参数解析场景epcFuncParams
            ++i; // skip `)`
            break;
        }
        else // 算符，数字，标识符
        {
            bool isOprPossibility = true;
            bool isNumPossibility = true;
            bool isNumFloat = false;
            bool isNumExp = false;
            bool isIdPossibility = true;
            isOprPossibility = ExprOperator::Possibility( exprOwner->_package, jctx.strTmp + ch );
            isNumPossibility = ExprLiteral::NumberPossibility( jctx.strTmp + ch, &isNumFloat, &isNumExp );
            isIdPossibility = ExprIdentifier::Possibility( jctx.strTmp + ch );

            if ( !isOprPossibility && !isNumPossibility && !isIdPossibility )
            {
                if ( !jctx.strTmp.empty() )//可能性都不存在，存储的临时内容不空，那就说明上一次肯定有识别成功一个东西
                {
                    __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );
                }
                else // 可能性都不存在，存储的临时内容又是空，那就说明暂不能识别这个字符，跳过它
                {
                    ++i;
                }
            }
            else
            {
                jctx.isOprPrevPossibility = isOprPossibility;
                jctx.isNumPrevPossibility = isNumPossibility;
                jctx.isNumFloatPrev = isNumFloat;
                jctx.isNumExpPrev = isNumExp;
                jctx.isIdPrevPossibility = isIdPossibility;

                jctx.strTmp += ch;
                ++i;
            }

        }
    }

    __JudgeOprOrNumOrId( eParam, &jctx, i - (int)jctx.strTmp.length() );
    __PopStackOprToSuffix( eParam, &jctx );

    if ( eParam->isEmpty() )
    {
        delete eParam;
    }
    else
    {
        funcAtom->_addParam(eParam);
    }
}

void ExprParser::_parseString( winux::String * v, winux::String const & str, int & i, std::vector<ExprParseContext> & epc )
{
    winux::String::value_type quote = str[i]; // 记下是什么引号

    v->clear(); // 初始化字符串

    int iStart = i;

    ++i; // skip quote
    winux::String::value_type ch = 0;
    while ( i < (int)str.length() )
    {
        ch = str[i];
        if ( ch == quote )
        {
            ++i;

            epc.pop_back(); // 弹出解析字符串场景标记`epcString`

            break;
        }
        else if ( ch == '\\' ) // 进入转义字符解析
        {
            epc.push_back(epcStrAntiSlashes);
            _parseStrAntiSlashes( v, str, i, epc );
        }
        else
        {
            *v += ch;
            ++i;
        }
    }

    if ( i >= (int)str.length() && ch != quote ) // 字符串未闭合
    {
        throw ExprError( ExprError::eeStringParseError, EXPRERRSTR_STRING_NOT_CLOSED(iStart) );
    }

}

void ExprParser::_parseStrAntiSlashes( winux::String * v, winux::String const & str, int & i, std::vector<ExprParseContext> & epc )
{
    ++i; // skip '\'
    while ( i < (int)str.length() )
    {
        winux::String::value_type ch = str[i];
        if ( ch == 'a' )
        {
            *v += '\a';
            ++i;
            break;
        }
        else if ( ch == 'b' )
        {
            *v += '\b';
            ++i;
            break;
        }
        else if ( ch == 't' )
        {
            *v += '\t';
            ++i;
            break;
        }
        else if ( ch == 'n' )
        {
            *v += '\n';
            ++i;
            break;
        }
        else if ( ch == 'v' )
        {
            *v += '\v';
            ++i;
            break;
        }
        else if ( ch == 'f' )
        {
            *v += '\f';
            ++i;
            break;
        }
        else if ( ch == 'r' )
        {
            *v += '\r';
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

            *v += StringToChar( octStr.c_str(), 8 );

            break;
        }
        else if ( ch == 'x' || ch == 'X' )
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

            *v += StringToChar( hexStr.c_str(), 16 );

            break;
        }
        else // 其余加\的字符都照原样输出
        {
            *v += ch;
            ++i;
            break;
        }
    }

    epc.pop_back();//弹出解析转义字符串场景标记`epcStrAntiSlashes`
}


}

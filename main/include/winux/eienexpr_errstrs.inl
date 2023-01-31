//错误Format字符串

//`%s`不是一个变量
#define EXPRERRSTR_NOT_VARIABLE(s) winux::Format( "`%s` is not a variable", (char const *)(s) )
//`%s`计算失败了
#define EXPRERRSTR_EVALUATE_FAILED(s) winux::Format( "`%s` evaluate failed", (char const *)(s) )
//`%s`不是一个可索引操作的对象
#define EXPRERRSTR_NOT_INDEXABLE_OBJECT(s) winux::Format( "`%s` is not a indexable object", (char const *)(s) )
//未关联变量场景对象
#define EXPRERRSTR_NOT_ASSOC_VARCTX(s) winux::Format( "Expression `%s` is not associated a VarContext object", (char const *)(s) )
//`%s`标识符未定义
#define EXPRERRSTR_IDENTIFIER_NOT_DEFINED(s) winux::Format( "The `%s` identifier is not defined", (char const *)(s) )
//`%s()`函数未定义
#define EXPRERRSTR_FUNCTION_NOT_DEFINED(s) winux::Format( "The `%s()` function is not defined", (char const *)(s) )
//"`%s()`参数不足"
#define EXPRERRSTR_NOT_ENOUGH_PARAMETERS(s) winux::Format( "`%s()` not enough parameters", (char const *)(s) )

//`%s`操作数出现在错误位置:%d, 不可能连续两个操作数
#define EXPRERRSTR_OPERAND_WRONG_PLACE_IMPOSSIBLE_CONTINUOUS_OPERAND( s, i ) winux::Format( "The `%s` operand to be in the wrong place:%d, it's impossible what continuous two operand", (char const *)(s), (int)(i) )
//`%s`操作数出现在错误位置:%d, 单目左结合算符右边不可能是操作数
#define EXPRERRSTR_OPERAND_WRONG_PLACE_IMPOSSIBLE_OPERAND_ATRIGHTOF_UNARYLEFTASSOC_OPR( s, i ) winux::Format( "The `%s` operand to be in the wrong place:%d, it's impossible what an operand at right of unary left-assoc operator", (char const *)(s), (int)(i) )
//`%s`操作符出现在错误位置:%d
#define EXPRERRSTR_OPERATOR_WRONG_PLACE( s, i ) winux::Format( "The `%s` operator to be in the wrong place:%d", (char const *)(s), (int)(i) )
// 在`%s`中，操作符`%s`出现在错误位置
#define EXPRERRSTR_OPERATOR_WRONG_PLACE_IN( s, s1 ) winux::Format( "`%s()`: `%s` appears in the wrong place", (char const *)(s), (char const *)(s1) )
//字符串未关闭, 起始位置:%d
#define EXPRERRSTR_STRING_NOT_CLOSED(i) winux::Format( "String is not closed, the starting position:%d", (int)(i) )
//`%s`不是可赋值的表达式
#define EXPRERRSTR_EXPR_NOT_WRITEABLE(s) winux::Format( "`%s` is not writeable expression", (char const *)(s) )
//`%s`不是%s类型的值
#define EXPRERRSTR_ERROR_VALUE_TYPE( s, s1 ) winux::Format( "`%s` is not %s type", (char const *)(s), (char const *)(s1) )

// 数组越界
#define EXPRERRSTR_OUT_OF_ARRAY_BOUND( s, n, i ) winux::Format( "`%s`: Out of array bound: size=%d, index=%d", (char const *)(s), (int)(n), (int)(i) )
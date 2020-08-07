#include "functional.hpp"

// 处理字符串
void Process_String( ProcessContext * ctx, String const & codeText, String::size_type * pI )
{
    String::size_type & i = *pI;

    String::value_type quote = codeText[i];
    i++; // skip quote
    while ( i < codeText.length() )
    {
        String::value_type ch = codeText[i];
        if ( ch == quote ) // 匹配引号结束
        {
            i++; // skip right quote
            break;
        }
        else if ( ch == '\\' ) // 转义字符
        {
            i++; // skip '\'
            if ( i < codeText.length() )
            {
                i++; // skip anyone char after'\'
            }
        }
        else if ( ch == '\n' )
        {
            i++;
        }
        else // 其他字符
        {
            i++;
        }
    }
}

// 处理行注释
void Process_LineComment( ProcessContext * ctx, String const & codeText, String::size_type * pI )
{
    String::size_type & i = *pI;

    i++; // 跳过//的第二个'/'
    while ( i < codeText.length() )
    {
        String::value_type ch = codeText[i];
        if ( ch == '\n' )
        {
            break;
        }
        else if ( ch == '\\' ) // 断行符
        {
            i++;
            if ( i < codeText.length() )
            {
                ch = codeText[i];
                if ( ch == '\n' ) // 如果断行符'\'背后直接是'\n'，说明续行注释，lines不+1
                {
                    i++; // skip '\n'
                }
            }
        }
        else
        {
            i++;
        }
    }
}

// 处理块注释
void Process_BlockComment( ProcessContext * ctx, String const & codeText, String::size_type * pI )
{
    String::size_type & i = *pI;

    i++; // 跳过/*的'*'
    while ( i < codeText.length() )
    {
        String::value_type ch = codeText[i];
        if ( ch == '\n' )
        {
            i++;
        }
        else if ( ch == '*' )
        {
            i++;
            if ( i < codeText.length() )
            {
                ch = codeText[i];
                if ( ch == '/' ) // '*/'块注释结束
                {
                    i++;
                    break;
                }
            }
        }
        else
        {
            i++;
        }
    }
}

// 处理代码，输出处理后的代码
void ProcessCode( ProcessContext * ctx, String const & codeText, String * pOutputCode )
{
    String & outputCode = *pOutputCode;
    String::size_type prevPos = 0; // 上一次位置
    String::size_type i = 0;

    while ( i < codeText.length() )
    {
        String::value_type ch = codeText[i];
        if ( ch == '\'' || ch == '\"' ) // 进入字符、字符串行数统计场景
        {
            auto t = i;
            Process_String( ctx, codeText, &i );
            //cout << ConsoleColor( winux::fgFuchsia, codeText.substr( t, i - t ) ) << endl;
        }
        else if ( !ctx->m && ch == '/' ) // 可能进入注释
        {
            String::size_type curCommentStart = i; // 注释开始位置
            i++;
            if ( i < codeText.length() )
            {
                ch = codeText[i];
                if (  ch == '/' ) // 是行注释
                {
                    Process_LineComment( ctx, codeText, &i );
                    //cout << ConsoleColor( winux::fgAtrovirens, codeText.substr( curCommentStart, i - curCommentStart ) ) << endl;

                    // 判断注释开始之前是否有空白,去掉空白
                    auto tmpStart = curCommentStart - 1;
                    while ( tmpStart >= 0 && ( codeText[tmpStart] == ' ' || codeText[tmpStart] == '\t' ) ) tmpStart--;
                    curCommentStart = tmpStart + 1;

                    outputCode += codeText.substr( prevPos, curCommentStart - prevPos );

                    // 如果注释后跟着换行符，并且注释开始位置之前也是换行符或是代码开始，说明此行全是注释，可以去除此行
                    if ( i < codeText.length() && codeText[i] == '\n' && ( curCommentStart == 0 || codeText[curCommentStart - 1] == '\n' ) ) i++; // skip '\n'

                    prevPos = i;
                }
                else if ( ch == '*' ) // 是块注释
                {
                    Process_BlockComment( ctx, codeText, &i );
                    //cout << ConsoleColor( winux::fgGreen, codeText.substr( curCommentStart, i - curCommentStart ) ) << endl;

                    auto tmpStart = curCommentStart - 1;
                    while ( tmpStart >= 0 && ( codeText[tmpStart] == ' ' || codeText[tmpStart] == '\t' ) ) tmpStart--;
                    curCommentStart = tmpStart + 1;

                    outputCode += codeText.substr( prevPos, curCommentStart - prevPos );

                    if ( i < codeText.length() && codeText[i] == '\n' && ( curCommentStart == 0 || codeText[curCommentStart - 1] == '\n' ) ) i++; // skip '\n'

                    prevPos = i;
                }
            }
        }
        else if ( !ctx->l && ch == '\n' ) // 遇到换行符
        {
            if ( i > 0 && codeText[i - 1] == '\n' ) // 前一个也是换行，说明这是一个空行，去除
            {
                outputCode += codeText.substr( prevPos, i - prevPos );
                prevPos = i + 1;
            }

            i++;
        }
        else
        {
            i++;
        }
    }

    if ( prevPos < codeText.length() )
    {
        outputCode += codeText.substr(prevPos);
    }
}

// 计算行数
uint CalcLines( String const & codeText )
{
    uint lines = 0;
    String::size_type i = 0, lastNlPos = String::npos;

    while ( i < codeText.length() )
    {
        if ( codeText[i] == '\n' )
        {
            lastNlPos = i;
            lines++;
        }

        i++;
    }
    if ( lastNlPos != codeText.length() - 1 )
    {
        lines++;
    }

    return lines;
}

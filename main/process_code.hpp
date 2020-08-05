#pragma once
#include <winux.hpp>
using namespace winux;

struct ProcessContext
{
    bool m; ///< 是否保留注释
    bool l; ///< 是否保留空行
};

void Process_CodeText( ProcessContext * ctx, String const & codeText, String * pOutputCode );

uint CalcLines( String const & codeText );
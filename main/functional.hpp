#pragma once
#include <winux.hpp>
using namespace winux;

struct ProcessContext
{
    bool m; ///< 是否保留注释
    bool l; ///< 是否保留空行
    bool re; ///< 是否使用正则匹配文件名
    String outputPath;
    String inDirOutputPath;
};

void ProcessCode( ProcessContext * ctx, String const & codeText, String * pOutputCode );

uint CalcLines( String const & codeText );
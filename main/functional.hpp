#pragma once
#include <iostream>
#include <functional>
#include <regex>
#include <atomic>

#include <winux.hpp>
using namespace winux;

struct ProcessContext
{
    bool m; ///< 是否保留注释
    bool l; ///< 是否保留空行
    bool re; ///< 是否使用正则匹配文件名
    StringArray patterns;
    StringArray searchPaths;
    String expansionMode; ///< 目录展开模式
    std::vector<std::regex> rePatterns;
    String outputPath; ///< 输出路径
};

void ProcessCode( ProcessContext * ctx, String const & codeText, String * pOutputCode );

uint CalcLines( String const & codeText );
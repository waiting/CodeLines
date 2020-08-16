#pragma once
#include <iostream>
#include <functional>
#include <regex>

#include <winux.hpp>
using namespace winux;

struct ProcessContext
{
    bool m; ///< 是否保留注释
    bool l; ///< 是否保留空行
    bool re; ///< 是否使用正则匹配文件名
    StringArray patterns;  ///< 匹配的模式串
    StringArray searchPaths; ///< 搜索的路径
    String expansionMode; ///< 目录展开模式
    std::vector<std::regex> rePatterns; ///< 构建的正则表达式
    struct Result
    {
        int files;
        int lines;
        Result() : files(0), lines(0) { }
    };
    std::map< int, Result > results; ///< 统计结果
    String outputPath; ///< 输出路径

    ProcessContext() : m(false), l(false), re(false)
    {
    }
};

void ProcessCode( ProcessContext * ctx, String const & codeText, String * pOutputCode );

uint CalcLines( String const & codeText );
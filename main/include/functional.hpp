#pragma once
#include <iostream>
#include <functional>
#include <regex>

#include <winux.hpp>
using namespace winux;

struct ProcessContext
{
    bool m; // 是否保留注释
    bool l; // 是否保留空行
    bool re; // 是否使用正则匹配文件名
    bool silent; // 静默执行
    bool verbose; // 显示详细信息
    bool json; // JSON格式数据显示
    StringArray patterns;  // 匹配的模式串
    StringArray searchPaths; // 搜索的路径
    String expansionMode; // 目录展开模式
    std::vector<std::regex> rePatterns; // 构建的正则表达式
    struct Result
    {
        size_t files;
        size_t originBytes;
        size_t processedBytes;
        size_t originLines;
        size_t processedLines;
        Result() { memset( this, 0, sizeof(*this) ); }
    };
    std::map< int, Result > results; // 统计结果
    /*
    JSON
    {
        error?: 错误内容字符串,
        search_path: [ 搜索路径, ... ],
        result: {
            ext1: {
                files: [
                    {
                        path: 文件路径,
                        origin_bytes: 原文件大小,
                        origin_lines: 原文件行数,
                        bytes: 处理后文件大小,
                        lines: 处理后文件行数,
                        verbose: [ 更多信息, ...]
                    }, ...
                ],
                origin_bytes: 原文件大小,
                origin_lines: 原文件行数,
                bytes: 处理后文件大小,
                lines: 处理后文件行数,
            },
            ext2: ...
        },
        total: {
            files: 文件数,
            origin_bytes: 原文件大小,
            origin_lines: 原文件行数,
            bytes: 处理后文件大小,
            lines: 处理后文件行数,
        }
    } */
    Mixed jsonWhole; // JSON式全部结果
    String outputPath; // 输出路径

    ProcessContext() : m(false), l(false), re(false), silent(false), verbose(false), json(false)
    {
        this->jsonWhole.createCollection();
    }
};

// 处理代码
void ProcessCode( ProcessContext * ctx, String const & codeText, String * pOutputCode );

// 统计行数
uint CalcLines( String const & codeText, std::function< void ( int iLine, String const & line ) > func );
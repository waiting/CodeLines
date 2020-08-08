#include <iostream>
#include <functional>
#include <regex>
#include <atomic>
using namespace std;

#include "functional.hpp"

ushort fgColor[] = {
    fgBlack,
    fgNavy,
    fgAtrovirens,
    fgTeal,
    fgMaroon,
    fgPurple,
    fgOlive,
    fgSilver,
    fgGray,
    fgBlue,
    fgGreen,
    fgAqua,
    fgRed,
    fgFuchsia,
    fgYellow,
    fgWhite,
};

ushort bgColor[] = {
    bgBlack,
    bgNavy,
    bgAtrovirens,
    bgTeal,
    bgMaroon,
    bgPurple,
    bgOlive,
    bgSilver,
    bgGray,
    bgBlue,
    bgGreen,
    bgAqua,
    bgRed,
    bgFuchsia,
    bgYellow,
    bgWhite,
};

/**
    统计代码行数
    调用命令格式:
    CodeLines [--m] [--l] [--re] ext1 [ext2] [ext3] ... <-|+> search_path ... [-o output_path[</|:>{name}.{ext}]]

    --m:
        统计注释的行数
    --l:
        统计空行的行数
    --re:
        使用正则表达式
    ext1 ext2 ext3 ...:
        当有--re时可以是正则表达式，匹配文件名。（如果要匹配扩展名可以末尾加$，ext$）
    -:
        表示递归搜索指定的路径列表
    +:
        表示搜索指定的路径列表，不搜索子文件夹
    search_path ...:
        搜索路径
    -o:
        基于当前目录，在output_path目录进行输出。
        当使用 / 时，在output_path目录输出处理后的源代码文件。
        当使用 : 时，在output_path目录按原有的目录结构输出处理后的源代码文件。
        源代码文件命名规则由之后的字符串指定，可使用的变量为{name}、{ext}，分别表示文件名和扩展名。

*/

bool AnalyzeParams( CommandLineVars const & cmdVars, StringArray * patterns, String * searchMode, StringArray * searchPaths )
{
    int k = 0;
    for ( ; k < cmdVars.getValuesCount(); k++ )
    {
        thread_local regex re("\\+|\\-");
        String v = cmdVars.getValue(k);
        if ( regex_match( v, re ) )
        {
            *searchMode = v;
            break;
        }
    }
    if ( searchMode->empty() )
    {
        cerr << ConsoleColor( fgRed, "未指定搜索指定路径的方式: + 或 -" ) << endl;
        return false;
    }

    for ( int t = 0; t < k; t++ )
    {
        patterns->push_back( cmdVars.getValue(t) );
    }
    if ( patterns->empty() )
    {
        cerr << ConsoleColor( fgRed, "未指定匹配文件的模式: 扩展名 或 正则表达式" ) << endl;
        return false;
    }

    for ( int l = k + 1; l < cmdVars.getValuesCount(); l++ )
    {
        searchPaths->push_back( cmdVars.getValue(l) );
    }
    if ( searchPaths->empty() )
    {
        cerr << ConsoleColor( fgRed, "未指定搜索路径" ) << endl;
        return false;
    }

    return true;
}

int SearchCodeFiles( ProcessContext * ctx, vector<regex> const & rePatterns, String const & searchMode, StringArray const & searchPaths, vector<StringArray> * codeFiles )
{
    int filesCount = 0;

    for ( StringArray::size_type i = 0; i < searchPaths.size(); ++i )
    {
        StringArray files, folders;
        FolderData( searchPaths[i], &files, &folders );
        cout << files << folders;
    }

    return filesCount;
}

int main( int argc, char const * argv[] )
{
    CommandLineVars cmdVars( argc, argv, "-o", "", "--m,--l,--re" );
    ProcessContext ctx = { 0 };
    ctx.m = cmdVars.hasFlag("--m");
    ctx.l = cmdVars.hasFlag("--l");
    ctx.re = cmdVars.hasFlag("--re");
    ctx.outputPath = cmdVars.getParam( "-o", "" );
    ctx.inDirOutputPath = cmdVars.getParam( "-i", "" );

    StringArray patterns, searchPaths;
    String searchMode;
    if ( !AnalyzeParams( cmdVars, &patterns, &searchMode, &searchPaths ) )
    {
        return 1;
    }
    cout << patterns << endl;
    cout << searchMode << endl;
    cout << searchPaths << endl;


    // build regex expr
    vector<regex> rePatterns;
    for ( auto it = patterns.begin(); it != patterns.end(); it++ )
    {
        rePatterns.emplace_back( ( ctx.re ? *it : (*it) + "$" ) );
    }

    vector<StringArray> codeFiles;
    SearchCodeFiles( &ctx, rePatterns, searchMode, searchPaths, &codeFiles );

    //FolderData();
    //regex_search()
    //regex_match()

    //ConvFrom<UnicodeString> cfu("UCS-2LE");
    //cout << cfu.convert(L"你好");
    //File f("main.cpp","r");
    //String pureCode;
    //Process_CodeText( &ctx, f.buffer(), &pureCode );
    //String localCode = LocalFromUtf8(pureCode);
    //FilePutContents( "main_nocomment.cpp", pureCode );
    //FilePutContents( "main_nocomment1.cpp", localCode );
    //auto localCode = FileGetContents("main_nocomment.cpp");
    //cout << LocalFromUtf8(pureCode);

    /*for ( int i = 0; i < 0x10; i++ )
    {
        for ( int j = 0; j < 0x10; j++ )
        {
            ushort attr = fgColor[i] | bgColor[ 15 - j ];
            auto s = Format( "%02X", attr );
            cout << ConsoleColor( attr, s );
        }
        cout  << endl;
    }//*/


    return 0;
}

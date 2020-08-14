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

ushort fgColorForPatterns[] = {
    fgAqua, fgRed, fgYellow, fgGreen, fgWhite, fgFuchsia, fgBlue, fgTeal, fgMaroon, fgOlive, fgAtrovirens, fgSilver, fgPurple, fgGray
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

int DoScanCodeFiles(
    ProcessContext * ctx,
    vector<regex> const & rePatterns,
    String const & searchMode,
    String searchTopDir, ///< 记录搜索起始的顶层目录
    StringArray const & searchPaths,
    std::function< void ( String const & topDir, size_t patternIndex, String const & path, String const & fileName ) > func
)
{
    int filesCount = 0;
    for ( auto const & searchPath : searchPaths )
    {
        if ( searchTopDir.empty() ) searchTopDir = PathNoSep(searchPath);

        StringArray files, subDirs;
        FolderData( searchPath, &files, &subDirs );
        for ( vector<regex>::size_type i = 0; i < rePatterns.size(); i++ )
        {
            for ( auto const & fileName : files )
            {
                if ( regex_search( fileName, rePatterns[i] ) )
                {
                    func( searchTopDir, i, searchPath, fileName );
                    filesCount++;
                }
            }
        }

        if ( searchMode == "+" )
        {
        }
        else if ( searchMode == "-" )
        {
            transform( subDirs.begin(), subDirs.end(), subDirs.begin(), [searchPath] ( auto & subFolder ) { return searchPath + DirSep + subFolder; } );
            filesCount += DoScanCodeFiles( ctx, rePatterns, searchMode, searchTopDir, subDirs, func );
        }
    }

    return filesCount;
}

void TryOutputFile( ProcessContext * ctx, String const & searchTopDir, size_t patternIndex, String const & path, String const & fileName )
{
    String ext; // 扩展名
    String fileTitle = FileTitle( fileName, &ext ); // 文件名

    MultiMatch mmr;
    mmr.addMatchReplacePair( "{name}", fileTitle );
    mmr.addMatchReplacePair( "{ext}", ext );

    String outputDir, outputFile;
    String::size_type pos;
    if ( ctx->outputPath.empty() ) // 不输出文件
    {
    }
    else if ( ( pos = ctx->outputPath.rfind( ':' ) ) != String::npos ) // 含有 ':' 在指定目录按代码原有目录结构输出
    {
        outputDir = ctx->outputPath.substr( 0, pos ); // "";
        outputFile = ctx->outputPath.substr( pos + 1 );
        if ( outputFile.empty() ) outputFile = fileName;
        outputFile = mmr.replace(outputFile);

        //计算相对于topDir目录的结构
        if ( strncmp( searchTopDir.c_str(), path.c_str(), searchTopDir.length() ) == 0 )
        {
            String tPath = path.substr( searchTopDir.length() );
            if ( !tPath.empty() && ( tPath[0] == '/' || tPath[0] == '\\' ) )
                tPath = tPath.substr(1);

            outputDir = CombinePath( outputDir, tPath );
        }
        //输出文件
        cout << searchTopDir << " : " << CombinePath( path, fileName ) << " => " << CombinePath(outputDir,outputFile) << endl;


    }
    else if ( ( ( pos = ctx->outputPath.rfind( '/' ) ) != String::npos || ( pos = ctx->outputPath.rfind( '\\' ) ) != String::npos ) ) // 含有 '/' 或 '\\' 在指定目录输出
    {
        if ( pos == ctx->outputPath.length() - 1 ) // '/'在末尾
        {
            if ( pos == 0 ) // '/'也在开头，说明只有一个'/'
            {
                outputDir = ctx->outputPath.substr( 0, pos ); // "";
                outputFile = fileName;
            }
            else // 
            {
                outputDir = ctx->outputPath.substr( 0, pos );
                outputFile = fileName;
            }
        }
        else
        {
            bool isdir = false;
            DetectPath( ctx->outputPath, &isdir );
            if ( isdir )
            {
                outputDir = ctx->outputPath;
                outputDir = fileName;
            }
            else
            {
                String lastPart = ctx->outputPath.substr( pos + 1 );
                if ( lastPart.find('.') != String::npos ) // 如果搜到.，则lastPart当成文件名
                {
                    outputDir = ctx->outputPath.substr( 0, pos );
                    outputFile = mmr.replace(lastPart);
                }
                else
                {
                    outputDir = ctx->outputPath;
                    outputFile = fileName;
                }
            }
        }
        //输出文件
        cout << CombinePath(outputDir,outputFile) << endl;
    }
}

int main( int argc, char const * argv[] )
{
    CommandLineVars cmdVars( argc, argv, "-o", "", "--m,--l,--re" );
    ProcessContext ctx = { 0 };
    ctx.m = cmdVars.hasFlag("--m");
    ctx.l = cmdVars.hasFlag("--l");
    ctx.re = cmdVars.hasFlag("--re");
    ctx.outputPath = cmdVars.getParam( "-o", "" );

    StringArray patterns, searchPaths;
    String searchMode;
    if ( !AnalyzeParams( cmdVars, &patterns, &searchMode, &searchPaths ) )
    {
        return 1;
    }
    cout << patterns << endl;
    cout << searchMode << endl;
    cout << searchPaths << endl;


    // 构建正则表达式对象
    vector<regex> rePatterns;
    for ( auto it = patterns.begin(); it != patterns.end(); it++ )
    {
        rePatterns.emplace_back( ( ctx.re ? *it : "\\." + (*it) + "$" ) );
    }

    // 扫描文件
    DoScanCodeFiles( &ctx, rePatterns, searchMode, "", searchPaths, [ &ctx, &patterns ] (auto && topDir, auto i, auto path, auto && f ) {
        auto fg = fgColorForPatterns[i % countof(fgColorForPatterns)];
        cout << ConsoleColor( fg, patterns[i] ) << "：" << ConsoleColor( fg, NormalizePath( CombinePath( path, f ) ) ) << endl;

        TryOutputFile( &ctx, topDir, i, path, f );
    } );

    //cout << codeFiles << endl;
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


    return 0;
}

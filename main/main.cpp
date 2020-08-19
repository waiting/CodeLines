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

#include "functional.hpp"
using namespace std;

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
    fgAqua, fgFuchsia, fgYellow, fgGreen, fgWhite, fgRed, fgBlue,
    fgTeal, fgPurple, fgOlive, fgAtrovirens, fgSilver, fgMaroon, fgGray
};

inline ushort FgColor( int patternIndex ) { return fgColorForPatterns[patternIndex % countof(fgColorForPatterns)]; }

template < typename _Ty >
inline ConsoleAttrT<_Ty> ErrorStyle( _Ty const & v )
{
    return ConsoleColor( fgWhite | bgMaroon, v, true );
}

// 分析命令参数
bool AnalyzeParams( ProcessContext * ctx, CommandLineVars const & cmdVars )
{
    int k = 0;
    for ( ; k < cmdVars.getValuesCount(); k++ )
    {
        thread_local regex re("\\+|\\-");
        String v = cmdVars.getValue(k);
        if ( regex_match( v, re ) )
        {
            ctx->expansionMode = v;
            break;
        }
    }
    if ( ctx->expansionMode.empty() )
    {
        cerr << ErrorStyle("未指定搜索指定路径的方式: + 或 -") << endl;
        return false;
    }

    for ( int t = 0; t < k; t++ )
    {
        ctx->patterns.push_back( cmdVars.getValue(t) );
    }
    if ( ctx->patterns.empty() )
    {
        cerr << ErrorStyle("未指定匹配文件的模式: 扩展名 或 正则表达式") << endl;
        return false;
    }
    try
    {
        // 构建正则表达式对象
        for ( auto && pa : ctx->patterns )
        {
            ctx->rePatterns.emplace_back( ( ctx->re ? pa : "\\." + pa + "$" ) );
        }
    }
    catch ( std::regex_error const & e )
    {
        cerr << ErrorStyle( e.what() ) << endl;
        return false;
    }

    for ( int l = k + 1; l < cmdVars.getValuesCount(); l++ )
    {
        ctx->searchPaths.push_back( cmdVars.getValue(l) );
    }
    if ( ctx->searchPaths.empty() )
    {
        cerr << ErrorStyle("未指定搜索路径") << endl;
        return false;
    }

    return true;
}

// 扫描代码文件
int DoScanCodeFiles(
    ProcessContext * ctx,
    String searchTopDir, ///< 记录搜索起始的顶层目录
    StringArray const & searchPaths,
    std::function< void ( String const & topDir, int patternIndex, String const & path, String const & fileName ) > func
)
{
    int filesCount = 0;
    for ( auto const & searchPath : searchPaths )
    {
        if ( searchTopDir.empty() ) searchTopDir = PathNoSep(searchPath);

        StringArray files, subDirs;
        FolderData( searchPath, &files, &subDirs );
        for ( vector<regex>::size_type i = 0; i < ctx->rePatterns.size(); i++ )
        {
            for ( auto const & fileName : files )
            {
                if ( regex_search( fileName, ctx->rePatterns[i] ) )
                {
                    func( searchTopDir, (int)i, searchPath, fileName );
                    filesCount++;
                }
            }
        }

        if ( ctx->expansionMode == "+" )
        {
        }
        else if ( ctx->expansionMode == "-" )
        {
            transform( subDirs.begin(), subDirs.end(), subDirs.begin(), [ searchPath ] ( String const & subFolder ) { return searchPath + DirSep + subFolder; } );
            filesCount += DoScanCodeFiles( ctx, searchTopDir, subDirs, func );
        }
    }

    return filesCount;
}

// 处理一个代码文件
void DoProcessCodeFile( ProcessContext * ctx, String const & searchTopDir, int patternIndex, String const & path, String const & fileName, String const & contents )
{
    auto fg = FgColor(patternIndex); // 文字颜色

    String processedCodeText;
    // 处理代码 去除注释、空行
    ProcessCode( ctx, contents, &processedCodeText );

    // 处理前的原始行数
    StringArray originCodes;
    CalcLines( contents, [&originCodes] ( auto i, auto const & line ) {
        originCodes.push_back(line);
    } );

    // 计算处理后的行数
    StringArray problemCodes;
    uint linesThisFile = CalcLines( processedCodeText, [&problemCodes] ( int iLine, String const & line ) {
        auto l1 = StrTrim(line);
        if ( l1.length() > 80 * 1.618 ) // 一行太长了
        {
            problemCodes.push_back(l1);
        }
    } );

    // 输出文件大小和行数
    ConsoleAttrT<int> ca( fg, 0 );
    ca.modify();
    cout << String( 80, '-' ) << endl;
    cout << ctx->patterns[patternIndex] << "：" << CombinePath( path, fileName ) << endl;
    cout << "处理前: bytes=" << contents.length() << ", lines=" << originCodes.size() << "  处理后: bytes=" << processedCodeText.length() << ", lines=" << linesThisFile << endl;
    cout << String( 80, '-' ) << endl;
    ca.resume();

    // 输出问题行在原始代码中的行数
    size_t start = 0;
    for ( size_t i = 0; i < problemCodes.size(); ++i )
    {
        for ( size_t j = start; j < originCodes.size(); ++j )
        {
            if ( originCodes[j].find( problemCodes[i] ) != String::npos )
            {
                cout << "Line(" << ConsoleColor( fgRed, j + 1 ) << "): ";
                cout << problemCodes[i];
                cout << "  第" << j + 1 << "行代码" << ConsoleColor( fgFuchsia, problemCodes[i].length() ) << "长度过长！" << endl;
                start = j + 1;
            }
        }
    }

    // 记下统计结果
    ctx->results[patternIndex].files++;
    ctx->results[patternIndex].processedBytes += processedCodeText.length();
    ctx->results[patternIndex].processedLines += linesThisFile;
    ctx->results[patternIndex].originBytes += contents.length();
    ctx->results[patternIndex].originLines += originCodes.size();

    String extName; // 扩展名
    String fileTitle = FileTitle( fileName, &extName ); // 文件名

    MultiMatch mmr;
    mmr.addMatchReplacePair( "{name}", fileTitle );
    mmr.addMatchReplacePair( "{ext}", extName );

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

        // 计算相对于topDir目录的结构
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
            outputDir = ctx->outputPath.substr( 0, pos );
            outputFile = fileName;
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
                thread_local regex reVar("\\{.+\\}");
                if ( lastPart.find('.') != String::npos || regex_search( lastPart, reVar ) ) // 如果搜到.或搜到{xxx}，则lastPart当成文件名
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
        cout << searchTopDir << " : " << CombinePath( path, fileName ) << " => " << CombinePath(outputDir,outputFile) << endl;
    }
}

int main( int argc, char const * argv[] )
{
    CommandLineVars cmdVars( argc, argv, "-o", "", "--m,--l,--re" );
    ProcessContext ctx;
    ctx.m = cmdVars.hasFlag("--m");
    ctx.l = cmdVars.hasFlag("--l");
    ctx.re = cmdVars.hasFlag("--re");
    ctx.outputPath = cmdVars.getParam( "-o", "" );

    if ( !AnalyzeParams( &ctx, cmdVars ) ) return 1;

    //cout << ctx.patterns << endl;
    //cout << ctx.expansionMode << endl;
    //cout << ctx.searchPaths << endl;

    // 扫描文件
    DoScanCodeFiles( &ctx, "", ctx.searchPaths, [ &ctx ] ( String const & searchTopDir, auto i, String const & path, String const & f ) {
        DoProcessCodeFile( &ctx, searchTopDir, i, path, f, FileGetContents( CombinePath( path, f ) ) );
    } );

    cout << endl;
    // 输出结果
    for ( auto i = 0U; i < ctx.patterns.size(); ++i )
    {
        ConsoleAttrT<int> ca( FgColor(i), 0 );
        ca.modify();
        cout << ctx.patterns[i] << ":\n    files=" << ctx.results[i].files << "\n"
            << "    lines=" << ctx.results[i].processedLines << ", bytes=" << ctx.results[i].processedBytes << endl
            << "    origin_lines=" << ctx.results[i].originLines << ", origin_bytes=" << ctx.results[i].originBytes << endl;
        ca.resume();
    }

    // 总计
    size_t totalFiles = 0;
    size_t totalProcessedLines = 0, totalProcessedBytes = 0;
    size_t totalOriginLines = 0, totalOriginBytes = 0;
    for ( auto & kv : ctx.results )
    {
        totalFiles += kv.second.files;
        totalProcessedLines += kv.second.processedLines;
        totalProcessedBytes += kv.second.processedBytes;
        totalOriginLines += kv.second.originLines;
        totalOriginBytes += kv.second.originBytes;
    }
    cout << "\nTotal:\n";
    cout << "    files=" << totalFiles << "\n"
        << "    lines=" << totalProcessedLines << ", bytes=" << totalProcessedBytes << endl
        << "    origin_lines=" << totalOriginLines << ", origin_bytes=" << totalOriginBytes << endl;

    return 0;
}

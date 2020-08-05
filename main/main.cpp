#include <iostream>
#include <functional>
#include <regex>
using namespace std;

#include "process_code.hpp"

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
    CodeLines [--m] [--l] ext1 [ext2] [ext3] ... {-|+} search_path ... [-o output_path[/{name}_t.{ext}] ] [-i indir_output_path[/{name}_t.{ext}] ]

        --m: 统计注释的行数
        --l: 统计空行的行数
        ext1,ext2,ext3: 可以是正则表达式，匹配文件名。（如果要匹配扩展名可以末尾加$，ext$）
        -: 表示递归搜索指定的路径列表
        +: 表示搜索指定的路径列表，不搜索子文件夹
        search_path: 搜索路径
        -o: 基于当前目录，在output_path目录输出处理后的代码，文件命名规则由{name} {ext}指定
        -i: 基于代码所在目录，在indir_output_path目录输出处理后的代码，文件命名规则由{name} {ext}指定

*/

int main( int argc, char const * argv[] )
{
    CommandLineVars cmdVars( argc, argv, "-o,-i", "", "--m,--l" );
    ProcessContext ctx = { 0 };
    ctx.m = cmdVars.hasFlag("--m");
    ctx.l = cmdVars.hasFlag("--l");

    //ConvFrom<UnicodeString> cfu("UCS-2LE");
    //cout << cfu.convert(L"你好");
    File f("main.cpp","r");
    String pureCode;
    Process_CodeText( &ctx, f.buffer(), &pureCode );
    //String localCode = LocalFromUtf8(pureCode);
    FilePutContents( "main_nocomment.cpp", pureCode );
    //FilePutContents( "main_nocomment1.cpp", localCode );
    //auto localCode = FileGetContents("main_nocomment.cpp");
    cout << LocalFromUtf8(pureCode);

    for ( int i = 0; i < 0x10; i++ )
    {
        for ( int j = 0; j < 0x10; j++ )
        {
            ushort attr = fgColor[i] | bgColor[ 15 - j ];
            auto s = Format( "%02X", attr );
            cout << ConsoleColor( attr, s );
        }
        cout  << endl;
    }

    return 0;
}

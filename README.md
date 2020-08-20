# CodeLines
[![Build status](https://ci.appveyor.com/api/projects/status/oach5ro1fk86xa23?svg=true)](https://ci.appveyor.com/project/waiting/codelines)

一个统计代码行数的工具，也可以用来去掉代码中的注释和空行。支持所有C系编程语言。


## 命令详细

    CodeLines [--m] [--l] [--re] [--silent] ext1 [ext2] [ext3] ... <-|+> search_path ... [-o output_path[</|:>{name}.{ext}]]

    --m:
        统计注释的行数
    --l:
        统计空行的行数
    --re:
        使用正则表达式
    --silent:
        静默模式
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


## 关于+ -目录模式的注意
当模式是-时，会展开子文件夹。因此search_path不得有包含情况，否则会重复统计文件。
譬如

    C:\aaa\bbb C:\aaa\bbb\ccc

上面两个路径，前面的路径已经包含后面的路径，所以后面路径不需要指定，否则重复。

当模式是+时，不会展开子文件夹，因此，上述的路径不会重复统计。


## 例子
1. 统计当前路径下的cpp,c,h文件行:

        CodeLines cpp c h - .

2. 统计C:\Project下的cpp,c,h行:

        CodeLines cpp c h - C:\Project

3. 统计C:\Project,D:\Project,E:\Project下的cpp,c,h行:

        CodeLines cpp c h - C:\Project D:\Project E:\Project

4. 使用正则表达式匹配源代码文件:

        CodeLines --re cpp$ main\.c .+\.h - C:\Project D:\Project

5. 在当前目录输出处理后的代码文件:

        CodeLines cpp hpp c h - C:\Project -o .
        or
        CodeLines cpp hpp c h - C:\Project -o ./
        or
        CodeLines cpp hpp c h - C:\Project -o ./{name}.{ext}
        rename
        CodeLines cpp hpp c h - C:\Project -o ./{name}_tmp.{ext}

6. 在当前目录按源代码目录结构输出处理后的代码文件:

        CodeLines cpp hpp c h - C:\Project -o .:
        or
        CodeLines cpp hpp c h - C:\Project -o .:{name}.{ext}
        rename
        CodeLines cpp hpp c h - C:\Project -o .:{name}_tmp.{ext}

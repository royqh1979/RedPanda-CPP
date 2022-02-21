# 依赖
 
 小熊猫C++需要Qt 5(>=5.12)

# Windows

 我使用msys2打包的最新版的GCC和MinGW-w64工具链来编译小熊猫C++。VC和其他版本的gcc不一定能够正常编译。

 编译步骤：
 - 安装msys2 (https://www.msys2.org)
 - 使用msys2的pacman程序安装mingw-w64-x86_64-qt5和mingw-w64-x86_64-gcc
 - 安装qtcreator
 - 使用qtcreator打开Red_Panda_CPP.pro文件

# Linux

步骤:
 - 安装 gcc 和 qt5
 - 使用qtcreator打开Red_Panda_CPP.pro文件
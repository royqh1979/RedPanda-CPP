$QtConfigureDebugOrRelease = "-release"
$OfficialQtDirectory = "C:\Qt"
$QtInstallPrefix = "C:\Qt\5.6.3"

function Get-QtModuleDirectory {
    param([string]$ModuleName)
    "qt$ModuleName-opensource-src-5.6.3"
}

function Prepare-QtSource {
    param([string]$ModuleName)
    $moduleDirectory = $(Get-QtModuleDirectory $ModuleName)
    $fileName = "$moduleDirectory.zip"

    if (!(Test-Path $moduleDirectory)) {
        if (!(Test-Path $fileName)) {
            $downloadUrl = "http://download.qt.io/new_archive/qt/5.6/5.6.3/submodules/$fileName"
            Invoke-WebRequest $downloadUrl -OutFile $fileName
        }
        Expand-Archive $fileName -DestinationPath .
    }
}

function Prepare-QtSources {
    Prepare-QtSource "base"
    Prepare-QtSource "svg"
    Prepare-QtSource "tools"
}

function Prepare-MinHookSource {
    $directory = "minhook-1.3.3"
    $fileName = "$directory.zip"

    if (!(Test-Path $directory)) {
        if (!(Test-Path $fileName)) {
            $downloadUrl = "https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.3.zip"
            Invoke-WebRequest $downloadUrl -OutFile $fileName
        }
        Expand-Archive $fileName -DestinationPath .
    }
}

function Build-QtBase {
    param([string]$Configuration)

    $buildDir = "build-qtbase-$Configuration"
    mkdir $buildDir
    pushd $buildDir

    $prefix = "$QtInstallPrefix\$Configuration"
    & "..\$(Get-QtModuleDirectory "base")\configure.bat" `
        -prefix $prefix $QtConfigureDebugOrRelease `
        -opensource -confirm-license `
        -no-use-gold-linker -static -static-runtime -platform win32-g++ -target xp `
        -opengl desktop -no-angle -iconv -gnu-iconv -no-icu -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -no-fontconfig -qt-harfbuzz -no-ssl -no-openssl `
        -nomake examples -nomake tests -nomake tools
    mingw32-make "-j$Env:NUMBER_OF_PROCESSORS"
    mingw32-make install
    $Env:Path = "$prefix\bin;$Env:Path"

    popd
}

function Build-QtModule {
    param([string]$Configuration, [string]$ModuleName)

    $buildDir = "build-qt$ModuleName-$Configuration"
    mkdir $buildDir
    pushd $buildDir

    qmake "..\$(Get-QtModuleDirectory $ModuleName)"
    mingw32-make "-j$Env:NUMBER_OF_PROCESSORS"
    mingw32-make install

    popd
}

function Build-Qt {
    param([string]$Configuration)

    Build-QtBase $Configuration
    Build-QtModule $Configuration "svg"
    Build-QtModule $Configuration "tools"
}

function Build-MinHook {
    param([string]$Configuration)

    $prefix = "$QtInstallPrefix\$Configuration"
    $directory = "minhook-1.3.3"
    pushd $directory

    mingw32-make -f build/MinGW/Makefile libMinHook.a "-j$Env:NUMBER_OF_PROCESSORS"
    mv libMinHook.a "$prefix\lib"
    cp "include\MinHook.h" "$prefix\include"
    rm src\*.o
    rm src\hde\*.o

    popd
}

function Main {
    $BasePath = $Env:Path
    Prepare-QtSources
    Prepare-MinHookSource

    ## 32-bit
    $Env:Path = "$OfficialQtDirectory\Tools\mingw810_32\bin;$BasePath"
    Build-Qt mingw81_32-static
    Build-MinHook mingw81_32-static

    ## 64-bit
    $Env:Path = "$OfficialQtDirectory\Tools\mingw810_64\bin;$BasePath"
    Build-Qt mingw81_64-static
    Build-MinHook mingw81_64-static
}

Main

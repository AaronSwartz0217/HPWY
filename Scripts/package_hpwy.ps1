param(
    [ValidateSet('Win64','Android')] [string]$Platform='Win64',
    [string]$ArchiveDirectory='C:/HPWY_Packages/20261119',
    [string]$Project='C:/HPWHBuild/HPWH/HPWY.uproject'
)
$ErrorActionPreference='Stop'
$uatArgs=@('-WaitForUATMutex','BuildCookRun',"-project=$Project",'-target=HPWY',"-platform=$Platform",'-clientconfig=Shipping','-build','-cook','-stage','-package','-pak','-compressed','-archive','-nop4','-utf8output','-nocompileeditor','-skipbuildeditor')
if($Platform -eq 'Android'){
    $env:ANDROID_HOME='C:/Users/29717/AppData/Local/Android/Sdk'
    $env:ANDROID_SDK_ROOT=$env:ANDROID_HOME
    $env:NDKROOT="$env:ANDROID_HOME/ndk/25.1.8937393"
    $env:JAVA_HOME='C:/Program Files/Android/Android Studio/jbr'
    $uatArgs+=@('-cookflavor=ASTC',"-archivedirectory=$ArchiveDirectory/Android")
}else{
    $uatArgs+=@('-iostore','-prereqs',"-archivedirectory=$ArchiveDirectory/Windows")
}
& 'C:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/RunUAT.bat' @uatArgs
exit $LASTEXITCODE

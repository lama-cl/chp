$ErrorActionPreference = "Stop"

if ($env:TS -eq '0') {
    $ts_part = 'nts'
} else {
    $ts_part = 'ts';
}

if ($env:APPVEYOR_REPO_TAG -eq "true") {
    $bname = "php_rdkafka-$env:APPVEYOR_REPO_TAG_NAME-$env:PHP_VER-$ts_part-$env:VC-$env:ARCH"
} else {
    $bname = "php_rdkafka-$($env:APPVEYOR_REPO_COMMIT.substring(0, 8))-$env:PHP_VER-$ts_part-$env:VC-$env:ARCH"
}
$zip_bname = "$bname.zip"

$dir = 'C:\projects\rdkafka\';
if ($env:ARCH -eq 'x64') {
    $dir += 'x64\'
}
$dir += 'Release'
if ($env:TS -eq '1') {
    $dir += '_TS'
}

$files = @(
    "$dir\php_rdkafka.dll",
    "$dir\php_rdkafka.pdb",
    "C:\projects\rdkafka\CREDITS",
    "C:\projects\rdkafka\LICENSE",
    "C:\projects\rdkafka\README.md",
    "C:\build-cache\deps\bin\librdkafka.dll",
    "C:\build-cache\deps\bin\librdkafka.pdb",
    "C:\build-cache\deps\LICENSE.LIBRDKAFKA"
)
Compress-Archive $files "C:\$zip_bname"
Push-AppveyorArtifact "C:\$zip_bname"

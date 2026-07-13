# Kill existing game process
Stop-Process -Name "game" -Force -ErrorAction SilentlyContinue

$projectDirectory = (Get-Location).Path
$compiler = "gcc.exe"

$sourceFiles = @(
    (Resolve-Path "main.c").Path
) + @(
    Get-ChildItem "src" -Filter "*.c" |
    ForEach-Object { $_.FullName }
)

$commonFlags = @(
    "-O1"
    "-Wall"
    "-std=c99"
    "-Wno-missing-braces"
    "-I", (Join-Path $projectDirectory "include")
    "-g"
)

# Build executable
$buildArguments = @(
    $sourceFiles
    "-o", "bin/game.exe"
    $commonFlags
    "-L", (Join-Path $projectDirectory "lib")
    "-lraylib"
    "-lopengl32"
    "-lgdi32"
    "-lws2_32",
    "-lwinmm",
    "-Wno-unused-function",
    "-Wno-maybe-uninitialized",
    "-Wno-switch"

)

& $compiler $buildArguments

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

# Run and save output to log.txt
#cmd /c ".\game.exe 2>&1" | Tee-Object -FilePath "log.txt"
cmd /c ".\bin\game.exe"

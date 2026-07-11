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

# Generate compile_commands.json for clangd
$compileCommands = foreach ($sourceFile in $sourceFiles) {
    $arguments = @(
        $commonFlags
        "-c"
        $sourceFile
    )

    @{
        directory = $projectDirectory
        file      = $sourceFile
        arguments = @($compiler) + $arguments
    }
}

$compileCommands |
ConvertTo-Json -Depth 5 |
Set-Content -Path "compile_commands.json" -Encoding UTF8

Write-Host "Generated compile_commands.json"

# Build executable
$buildArguments = @(
    $sourceFiles
    "-o", "game.exe"
    $commonFlags
    "-L", (Join-Path $projectDirectory "lib")
    "-lraylib"
    "-lopengl32"
    "-lgdi32"
    "-lwinmm"
)

& $compiler $buildArguments

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

$ctags = Get-Command ctags -ErrorAction SilentlyContinue
if ($ctags) {
    & $ctags.Source -R --languages=C --exclude=build --exclude=lib .
} else {
    Write-Host "ctags not found; skipping tag generation"
}

# Run and save output to log.txt
cmd /c ".\game.exe 2>&1" | Tee-Object -FilePath "log.txt"

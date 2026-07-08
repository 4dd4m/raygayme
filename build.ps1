# Kill existing game process
Stop-Process -Name "game" -Force -ErrorAction SilentlyContinue

# Build
gcc.exe main.c src/*.c -o game.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

# Run and save output to log.txt
& .\game.exe 2>&1 | Tee-Object -FilePath log.txt
# Kill existing game process
Stop-Process -Name "game" -Force -ErrorAction SilentlyContinue

# Build
# Build
# Build
gcc.exe main.c src/*.c ../shared/src/*.c -o bin/game.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -I ../shared/include -L lib/ -lraylib -lopengl32 -lgdi32 -lws2_32 -lwinmm -Wno-unused-function -Wno-maybe-uninitialized -Wno-switch -g

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

# Run and save output to log.txt
#cmd /c ".\game.exe 2>&1" | Tee-Object -FilePath "log.txt"
mingw32-make clear
if ($LASTEXITCODE -ne 0) {
    Write-Host "Clear failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

& .\bin\game.exe
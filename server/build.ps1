# Kill existing game process
Stop-Process -Name "server" -Force -ErrorAction SilentlyContinue

# Build
# Build
# Build
gcc.exe main.c src/*.c ../shared/src/*.c ../client/src/cJSON.c -o bin/server.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -I ../shared/include -I ../client/include -L lib/ -lraylib -lws2_32 -lopengl32 -lgdi32 -lwinmm -g

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

# Run and save output to log.txt
#& .\bin\server.exe 2>&1 | Tee-Object -FilePath log.txt
mingw32-make clear
if ($LASTEXITCODE -ne 0) {
    Write-Host "Clear failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

& .\bin\server.exe

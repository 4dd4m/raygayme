# Kill existing game process
Stop-Process -Name "server" -Force -ErrorAction SilentlyContinue

# Build
# Build
# Build
gcc.exe main.c src/*.c ../shared/*.c -o server.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -I ../shared -L lib/ -lraylib -lws2_32 -lopengl32 -lgdi32 -lwinmm -g

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!"
    Read-Host "Press Enter to exit"
    exit $LASTEXITCODE
}

# Run and save output to log.txt
& .\server.exe 2>&1 | Tee-Object -FilePath log.txt
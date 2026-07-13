#!/usr/bin/env bash

set -e

PROJECT_DIR="$(pwd)"
CC="gcc"
UNAME="$(uname -s)"
RAYLIB_DIR="$PROJECT_DIR/lib"

if [ "$UNAME" = "Linux" ] && [ -f "$PROJECT_DIR/lib/linux/libraylib.a" ]; then
    RAYLIB_DIR="$PROJECT_DIR/lib/linux"
fi

# Source files
SOURCE_FILES=(
    "$PROJECT_DIR/main.c"
)

while IFS= read -r -d '' file; do
    SOURCE_FILES+=("$file")
done < <(find "$PROJECT_DIR/src" -name "*.c" -print0)

COMMON_FLAGS=(
    -O1
    -Wall
    -std=c99
    -Wno-missing-braces
    -I"$PROJECT_DIR/include"
    -g
)

# Generate compile_commands.json
echo "[" > compile_commands.json

for ((i=0; i<${#SOURCE_FILES[@]}; i++)); do
    file="${SOURCE_FILES[$i]}"

    printf '  {\n' >> compile_commands.json
    printf '    "directory": "%s",\n' "$PROJECT_DIR" >> compile_commands.json
    printf '    "file": "%s",\n' "$file" >> compile_commands.json

    printf '    "arguments": [' >> compile_commands.json
    printf '"%s"' "$CC" >> compile_commands.json

    for flag in "${COMMON_FLAGS[@]}"; do
        printf ', "%s"' "$flag" >> compile_commands.json
    done

    printf ', "-c", "%s"' "$file" >> compile_commands.json
    printf ']\n' >> compile_commands.json

    if [ "$i" -lt $((${#SOURCE_FILES[@]}-1)) ]; then
        printf "  },\n" >> compile_commands.json
    else
        printf "  }\n" >> compile_commands.json
    fi
done

echo "]" >> compile_commands.json

echo "Generated compile_commands.json"

# Build
$CC \
    "${SOURCE_FILES[@]}" \
    -o game \
    "${COMMON_FLAGS[@]}" \
    -L"$RAYLIB_DIR" \
    -lraylib \
    -lGL \
    -lm \
    -lpthread \
    -ldl \
    -lrt \
    -lX11 \
    -Wno-unused-function \
    -Wno-maybe-uninitialized \
    -Wno-switch

echo "Build successful."

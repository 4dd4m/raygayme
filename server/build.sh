#!/usr/bin/env bash

set -e

PROJECT_DIR="$(pwd)"
CC="${CC:-gcc}"
UNAME="$(uname -s)"
TARGET="server"

SOURCE_FILES=(
    "$PROJECT_DIR/main.c"
)

if [ -d "$PROJECT_DIR/src" ]; then
    while IFS= read -r -d '' file; do
        SOURCE_FILES+=("$file")
    done < <(find "$PROJECT_DIR/src" -name "*.c" -print0)
fi

if [ -d "$PROJECT_DIR/../shared" ]; then
    while IFS= read -r -d '' file; do
        SOURCE_FILES+=("$file")
    done < <(find "$PROJECT_DIR/../shared" -name "*.c" -print0)
fi

COMMON_FLAGS=(
    -O1
    -Wall
    -std=c99
    -Wno-missing-braces
    -I"$PROJECT_DIR/include"
    -I"$PROJECT_DIR/../shared"
    -g
)

LINK_FLAGS=()

if [ "$UNAME" = "Linux" ]; then
    COMMON_FLAGS+=(-D_POSIX_C_SOURCE=200809L)
elif [[ "$UNAME" == MINGW* || "$UNAME" == MSYS* || "$UNAME" == CYGWIN* ]]; then
    TARGET="server.exe"
    LINK_FLAGS+=(-lws2_32)
fi

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

    if [ "$i" -lt $((${#SOURCE_FILES[@]} - 1)) ]; then
        printf "  },\n" >> compile_commands.json
    else
        printf "  }\n" >> compile_commands.json
    fi
done

echo "]" >> compile_commands.json

echo "Generated compile_commands.json"

"$CC" \
    "${SOURCE_FILES[@]}" \
    -o "$TARGET" \
    "${COMMON_FLAGS[@]}" \
    "${LINK_FLAGS[@]}"

echo "Build successful: $TARGET"

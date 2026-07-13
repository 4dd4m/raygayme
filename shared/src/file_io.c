#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

char* LoadFile(char* filePath) {
    FILE* file = fopen(filePath, "r");

    if (!file) {
        perror("Error opening static objects file");
        return NULL;
    }

    // check file length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length <= 0) {
        perror("Object file size problem");
        fclose(file);
        return NULL;
    }

    // allocate memory
    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        perror("Memory allocation has been failed");
        return NULL;
    }

    // read file into the buffer
    size_t read_size = fread(buffer, 1, length, file);
    buffer[read_size] = '\0';

    fclose(file);

    return buffer;
}
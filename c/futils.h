#ifndef FUTILS_H
#define FUTILS_H

#include <stdio.h>

FILE *fopen_mkdir(char *path, char *mode);

void fcopy(FILE *dst, FILE *src, size_t nbytes);
void fxor (FILE *dst, FILE *src, size_t nbytes, char *key);

void fputs0(char *str, FILE *file);
void fgets0(char *str, FILE *file);

#endif

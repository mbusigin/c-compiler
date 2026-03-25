/**
 * preproc.h - C preprocessor
 */

#ifndef PREPROC_H
#define PREPROC_H

// Add an include search path
void preproc_add_include_path(const char *path);

// Preprocess C source code
char *preprocess(const char *source);

// Preprocess a file
char *preprocess_file(const char *filename);

#endif // PREPROC_H

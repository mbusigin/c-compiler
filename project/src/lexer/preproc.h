/**
 * preproc.h - Simple C preprocessor
 */

#ifndef PREPROC_H
#define PREPROC_H

// Preprocess C source code
// Removes #include lines and adds extern declarations
char *preprocess(const char *source);

#endif // PREPROC_H

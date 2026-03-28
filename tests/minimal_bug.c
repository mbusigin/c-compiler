/* Minimal test case for struct member assignment bug
 * 
 * Expected behavior:
 *   - options.input_file should be set to argv[1]
 *   - Program should print the first argument
 *
 * Current buggy behavior:
 *   - options.input_file is never assigned (store instruction missing)
 *   - Program prints "(null)" or crashes
 */

#include <stdio.h>
#include <string.h>

typedef struct {
    int dump_tokens;       /* offset 0 */
    int dump_ast;          /* offset 4 */
    int dump_ir;           /* offset 8 */
    int dump_asm;          /* offset 12 */
    int syntax_only;       /* offset 16 */
    int preprocess_only;   /* offset 20 */
    int generate_debug;    /* offset 24 */
    int optimization_level;/* offset 28 */
    char *output_file;     /* offset 32 */
    char *input_file;     /* offset 40 */
} CompileOptions;

int main(int argc, char **argv) {
    CompileOptions options;
    
    /* Initialize all fields */
    options.dump_tokens = 0;
    options.dump_ast = 0;
    options.dump_ir = 0;
    options.dump_asm = 0;
    options.syntax_only = 0;
    options.preprocess_only = 0;
    options.generate_debug = 0;
    options.optimization_level = 0;
    options.output_file = NULL;
    options.input_file = NULL;
    
    printf("Before assignment: input_file = %p\n", options.input_file);
    
    /* This is the bug: struct member assignment after condition check
     * This mimics the pattern in main.c where options.input_file = argv[i]
     * is inside an if statement checking argv[i][0] != '-'
     */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            /* BUG: This assignment should store the pointer value */
            options.input_file = argv[i];
            printf("Assigned: input_file = %p, argv[%d] = %p\n", 
                   options.input_file, i, argv[i]);
            break;
        }
    }
    
    /* Check if the assignment worked */
    if (options.input_file != NULL) {
        printf("PASS: input_file = '%s'\n", options.input_file);
        return 0;
    } else {
        printf("FAIL: input_file is NULL (assignment did not work)\n");
        return 1;
    }
}

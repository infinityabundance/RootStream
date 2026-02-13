/*
 * test_harness.c - Test runner implementation
 *
 * Provides test execution and result reporting for RootStream tests.
 */

#include "test_harness.h"
#include <stdio.h>

int run_test_suite(const test_case_t *tests) {
    if (!tests) return 1;
    
    int passed = 0, failed = 0, skipped = 0;
    
    for (int i = 0; tests[i].name; i++) {
        printf("  [%d] %s...", i + 1, tests[i].name);
        fflush(stdout);
        
        test_result_t ret = tests[i].fn();
        
        switch (ret) {
            case TEST_PASS:
                printf(" PASS\n");
                passed++;
                break;
            case TEST_FAIL:
                printf(" FAIL\n");
                failed++;
                break;
            case TEST_SKIP:
                printf(" SKIP\n");
                skipped++;
                break;
        }
    }
    
    printf("\nResults: %d passed, %d failed, %d skipped\n", 
           passed, failed, skipped);
    
    return failed > 0 ? 1 : 0;
}

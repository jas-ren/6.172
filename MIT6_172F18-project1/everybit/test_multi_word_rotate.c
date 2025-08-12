#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "bitarray.h"

// Helper function to print binary representation of an array
void print_binary_array(uint64_t* array, int num_words, const char* label) {
    printf("%s: ", label);
    for (int w = 0; w < num_words; w++) {
        for (int i = 63; i >= 0; i--) {
            printf("%ld", (array[w] >> i) & 1);
            if (i % 8 == 0 && i > 0) printf(" ");
        }
        if (w < num_words - 1) printf(" | ");
    }
    printf("\n");
}

// Helper function to set up test data
void setup_test_array(uint64_t* array, int num_words) {
    // Create a pattern that's easy to verify: 0x0123456789ABCDEF pattern
    for (int i = 0; i < num_words; i++) {
        array[i] = 0x0123456789ABCDEFULL + (i * 0x1111111111111111ULL);
    }
}

// Helper function to copy array
void copy_array(uint64_t* dest, uint64_t* src, int num_words) {
    for (int i = 0; i < num_words; i++) {
        dest[i] = src[i];
    }
}

// Test case structure
typedef struct {
    int num_words;
    size_t offset;
    size_t length;
    size_t rotation;
    const char* description;
} multi_word_test_t;

// Test cases for multi-word rotation
multi_word_test_t multi_tests[] = {
    // Simple cases
    {2, 0, 64, 8, "Single word, 8-bit rotation"},
    {2, 0, 128, 16, "Two full words, 16-bit rotation"},
    {3, 32, 128, 24, "Cross-word boundary rotation"},
    {2, 16, 96, 12, "Partial words with offset"},
    {4, 0, 200, 40, "Large multi-word rotation"},
    
    // Edge cases
    {2, 63, 65, 1, "Edge case: spans word boundary by 1 bit"},
    {3, 0, 65, 64, "Edge case: rotation = full word"},
    {2, 0, 100, 0, "Edge case: no rotation"},
    {3, 10, 150, 75, "Edge case: rotation > length/2"},
};

// Run a single multi-word test
int run_multi_word_test(const multi_word_test_t* test) {
    printf("\n=== %s ===\n", test->description);
    printf("Words: %d, Offset: %zu, Length: %zu, Rotation: %zu\n", 
           test->num_words, test->offset, test->length, test->rotation);
    
    // Setup arrays
    uint64_t* original = calloc(test->num_words, sizeof(uint64_t));
    uint64_t* test_array = calloc(test->num_words, sizeof(uint64_t));
    
    setup_test_array(original, test->num_words);
    copy_array(test_array, original, test->num_words);
    
    print_binary_array(original, test->num_words, "Before");
    
    // Perform rotation
    multi_word_rotate(test_array, test->offset, test->length, test->rotation);
    
    print_binary_array(test_array, test->num_words, "After ");
    
    // For now, just check that something changed (unless rotation is 0 or full cycle)
    int changed = 0;
    for (int i = 0; i < test->num_words; i++) {
        if (original[i] != test_array[i]) {
            changed = 1;
            break;
        }
    }
    
    int should_change = (test->rotation % test->length) != 0;
    
    if (changed == should_change) {
        printf("✓ PASS (rotation behavior as expected)\n");
        free(original);
        free(test_array);
        return 1;
    } else {
        printf("✗ FAIL (unexpected rotation behavior)\n");
        free(original);
        free(test_array);
        return 0;
    }
}

// Test against word_size_bitarray_rotate for single-word cases
int test_single_word_compatibility() {
    printf("\n=== Single Word Compatibility Test ===\n");
    
    uint64_t test_value = 0x123456789ABCDEFULL;
    uint64_t multi_word_array[1] = {test_value};
    
    // Test parameters
    size_t offset = 8;
    size_t length = 48;
    size_t rotation = 12;
    
    // Test single-word function
    uint64_t single_result = word_size_bitarray_rotate(test_value, offset, length, rotation);
    
    // Test multi-word function
    multi_word_rotate(multi_word_array, offset, length, rotation);
    uint64_t multi_result = multi_word_array[0];
    
    printf("Original:      0x%016lX\n", test_value);
    printf("Single-word:   0x%016lX\n", single_result);
    printf("Multi-word:    0x%016lX\n", multi_result);
    
    if (single_result == multi_result) {
        printf("✓ PASS (functions match)\n");
        return 1;
    } else {
        printf("✗ FAIL (functions don't match)\n");
        return 0;
    }
}

int main() {
    printf("Testing multi_word_rotate function\n");
    printf("===================================\n");
    
    int passed = 0;
    int total = sizeof(multi_tests) / sizeof(multi_tests[0]);
    
    // Run multi-word tests
    for (int i = 0; i < total; i++) {
        if (run_multi_word_test(&multi_tests[i])) {
            passed++;
        }
    }
    
    // Run compatibility test
    if (test_single_word_compatibility()) {
        passed++;
    }
    total++; // Account for compatibility test
    
    printf("\n===================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    
    if (passed == total) {
        printf("🎉 All tests passed!\n");
        return 0;
    } else {
        printf("❌ Some tests failed!\n");
        return 1;
    }
}

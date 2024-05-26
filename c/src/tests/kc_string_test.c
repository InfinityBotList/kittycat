#include "../lib/kc_string.h"
#include "../lib/alloc.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    kittycat_set_allocator(malloc, realloc, free, memcpy);

    printf("Running tests: %s...\n", ":)");

    char *str = "test.bc";
    struct kittycat_string *s = kittycat_string_new(str, strlen(str));

    struct kittycat_string *out[2];
    int i = kittycat_string_splitn(s, '.', out, 2);

    printf("out splits: %d\n", i);

    printf("Namespace: %s\n", out[0]->str);
    printf("Perm: %s\n", out[1]->str);
    printf("Is Namespace NULL: %s\n", kittycat_string_empty(out[0]) ? "true" : "false");
    printf("Namespace Length: %zu\n", out[0]->len);
    printf("Perm Length: %zu\n", out[1]->len);
    printf("Is Perm NULL: %s\n", kittycat_string_empty(out[1]) ? "true" : "false");

    kittycat_string_arr_free(out, i);
    kittycat_string_free(s);

    s = kittycat_string_new("abc", 3);

    struct kittycat_string *out2[2];
    i = kittycat_string_splitn(s, '.', out2, 2);

    printf("out2 splits: %d\n", i);

    printf("Namespace: %s\n", out2[0]->str);
    printf("Perm: %s\n", !kittycat_string_empty(out2[1]) ? out2[1]->str : "NULL");
    printf("Is Namespace NULL: %s\n", kittycat_string_empty(out2[0]) ? "true" : "false");
    printf("Is Perm NULL: %s\n", kittycat_string_empty(out2[1]) ? "true" : "false");
    printf("Namespace Length: %zu\n", out2[0]->len);
    printf("Perm Length: %zu\n", kittycat_string_empty(out2[1]) ? 0 : out2[1]->len);

    // Free s
    kittycat_string_free(s);

    struct kittycat_string *s1 = kittycat_string_new("abc", 3);
    struct kittycat_string *s2 = kittycat_string_new("def", 3);

    struct kittycat_string *s3 = kittycat_string_concat(s1, s2);

    printf("Concatenated: %s\n", s3->str);
    printf("Concatenated len: %zu\n", s3->len);
    printf("String contains 'a': %s\n", kittycat_string_contains(s3, 'a') ? "true" : "false");
    printf("String contains 'e': %s\n", kittycat_string_contains(s3, 'e') ? "true" : "false");
    printf("String contains 'q': %s\n", kittycat_string_contains(s3, 'q') ? "true" : "false");

    // Free memory
    kittycat_string_arr_free(out2, i);
    kittycat_string_free(s1);
    kittycat_string_free(s2);
    kittycat_string_free(s3);
}
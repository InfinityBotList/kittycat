#include "kc_string.c"

int main()
{
    printf("Running tests: %s...\n", ":)");

    char *str = "test.bc";
    struct string *s = new_string(str, strlen(str));

    struct string *out[2];
    string_splitn(s, '.', out, 2);

    printf("Namespace: %s\n", out[0]->str);
    printf("Perm: %s\n", out[1]->str);
    printf("Is Namespace NULL: %s\n", string_is_empty(out[0]) ? "true" : "false");
    printf("Namespace Length: %zu\n", out[0]->len);
    printf("Perm Length: %zu\n", out[1]->len);
    printf("Is Perm NULL: %s\n", string_is_empty(out[1]) ? "true" : "false");

    string_arr_free(out, 2);
    string_free(s);

    s = new_string("abc", 3);

    struct string *out2[2];
    string_splitn(s, '.', out2, 2);

    printf("Namespace: %s\n", out2[0]->str);
    printf("Perm: %s\n", out2[1]->str);
    printf("Is Namespace NULL: %s\n", string_is_empty(out2[0]) ? "true" : "false");
    printf("Is Perm NULL: %s\n", string_is_empty(out2[1]) ? "true" : "false");
    printf("Namespace Length: %zu\n", out2[0]->len);
    printf("Perm Length: %zu\n", out2[1]->len);

    // Free s
    string_free(s);

    struct string *s1 = new_string("abc", 3);
    struct string *s2 = new_string("def", 3);

    struct string *s3 = string_concat(s1, s2);

    printf("Concatenated: %s\n", s3->str);
    printf("Concatenated len: %zu\n", s3->len);
    printf("String contains 'a': %s\n", string_contains(s3, 'a') ? "true" : "false");
    printf("String contains 'e': %s\n", string_contains(s3, 'e') ? "true" : "false");
    printf("String contains 'q': %s\n", string_contains(s3, 'q') ? "true" : "false");

    // Free memory
    string_arr_free(out2, 2);
    string_free(s1);
    string_free(s2);
    string_free(s3);
}
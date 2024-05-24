#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// String
struct string
{
    char *str;
    size_t len;
    bool isCloned;
};

// Create a new string
//
// Note: callers must free the string after use using `string_free`
// Note 2: Callers must manually call strndup if the string should be copied (or use new_string_cloned)
struct string *new_string(char *str, const size_t len)
{
    struct string *s = malloc(sizeof(struct string));
    s->str = str;
    s->len = len;
    s->isCloned = false;
    return s;
}

// Create a new string
//
// Note: callers must free the string after use using `string_free`
// Note 2: Callers must manually call strndup if the string should be copied (or use new_string_cloned)
struct string *new_string_cloned(char *str, const size_t len)
{
    char *cp_str = strndup(str, len); // Copy the string to prevent memory leaks
    struct string *s = malloc(sizeof(struct string));
    s->str = cp_str;
    s->len = len;
    s->isCloned = true;
    return s;
}

// Clone a string
struct string *string_clone(struct string *s)
{
    return new_string_cloned(s->str, s->len);
}

// Clone the chars of a string
char *string_clone_chars(struct string *s)
{
    return strndup(s->str, s->len);
}

struct string *string_substr(struct string *s, const size_t start, const size_t end)
{
    size_t len = end - start;
    char *str = malloc(len + 1);
    memcpy(str, s->str + start, len);
    str[len] = '\0'; // Null terminate the string
    struct string *ns = new_string(str, len);
    ns->isCloned = true; // Mark as cloned as string_substr copies to another string
    return ns;
}

void string_splitn(struct string *s, const char sep, struct string **out, const size_t n)
{
    size_t i = 0;
    size_t start = 0;
    size_t j = 0;
    while (i < s->len)
    {
        if (j >= n)
        {
            break;
        }

        if (s->str[i] == sep)
        {
            out[j] = string_substr(s, start, i);
            j++;
            start = i + 1;
        }
        i++;
    }

    out[j] = string_substr(s, start, s->len);

    // Allocate empty strings for the rest of the array
    for (size_t k = j + 1; k < n; k++)
    {
        out[k] = new_string("", 0);
    }
}

struct string *string_concat(struct string *s1, struct string *s2)
{
    size_t len = s1->len + s2->len;
    char *str = malloc(len + 1);              // Allocate memory for the concatenated string + 1 for null terminator
    memcpy(str, s1->str, s1->len);            // Copy the first string
    memcpy(str + s1->len, s2->str, s2->len);  // Copy the second string
    str[len] = '\0';                          // Null terminate the string
    struct string *ns = new_string(str, len); // Return the concatenated string
    ns->isCloned = true;                      // Mark as cloned as string_concat copies to another string
    return ns;
}

size_t string_length(struct string *s)
{
    return s->len;
}

void string_free(struct string *s)
{
    // Already freed if NULL
    if (s == NULL)
        return;
    if (s->isCloned)
    {
        free(s->str); // Free the string
        s->str = NULL;
    }
    free(s);
    s = NULL;
}

void string_arr_free(struct string **arr, const size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        string_free(arr[i]);
    }
}

bool string_is_empty(struct string *s)
{
    return s == NULL || s->len == 0 || s->str == NULL;
}

bool string_is_equal(struct string *s1, struct string *s2)
{
    if (s1->len != s2->len)
    {
        return false;
    }

    return strcmp(s1->str, s2->str) == 0;
}

bool string_is_equal_char(struct string *s, const char *c)
{
    return strcmp(s->str, c) == 0;
}

bool string_contains(struct string *s, const char c)
{
    for (size_t i = 0; i < s->len; i++)
    {
        if (s->str[i] == c)
        {
            return true;
        }
    }
    return false;
}

int string_print(struct string *s, FILE *stream)
{
    return fwrite(s->str, sizeof(char), s->len, stream);
}

struct string *string_trim_prefix(struct string *s, const char c)
{
    size_t start = 0;
    while (s->str[start] == c)
    {
        start++;
    }

    return string_substr(s, start, s->len);
}
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
};

// Create a new string
//
// Note: callers must free the string after use using `string_free`
struct string *new_string(char *str, const size_t len)
{
    char *cp_str = strndup(str, len); // Copy the string to prevent memory leaks
    struct string *s = malloc(sizeof(struct string));
    s->str = cp_str;
    s->len = len;
    return s;
}

// Create a new string from a char array
struct string *string_from_char(char *str)
{
    size_t len = strlen(str);
    return new_string(str, len);
}

struct string *string_substr(struct string *s, const size_t start, const size_t end)
{
    size_t len = end - start;
    char *str = malloc(len);
    memcpy(str, s->str + start, len);
    struct string *ns = new_string(str, len);
    free(str); // Free string now that we have copied it
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
    char *str = malloc(len);                  // Allocate memory for the concatenated string
    memcpy(str, s1->str, s1->len);            // Copy the first string
    memcpy(str + s1->len, s2->str, s2->len);  // Copy the second string
    struct string *ns = new_string(str, len); // Return the concatenated string
    free(str);                                // Free the memory allocated for the concatenated string
    return ns;
}

size_t string_length(struct string *s)
{
    return s->len;
}

void string_free(struct string *s)
{
    free(s->str); // Free the string
    free(s);
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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// String
struct kittycat_string
{
    char *str;
    size_t len;

    // Internal
    bool __isCloned;
};

// Create a new string
//
// Note: callers must free the string after use using `string_free`
// Note 2: Callers must manually call strndup if the string should be copied (or use new_string_cloned)
struct kittycat_string *new_string(char *str, const size_t len)
{
    struct kittycat_string *s = malloc(sizeof(struct kittycat_string));
    s->str = str;
    s->len = len;
    s->__isCloned = false;
    return s;
}

// Create a new string
//
// Note: callers must free the string after use using `string_free`
// Note 2: Callers must manually call strndup if the string should be copied (or use new_string_cloned)
struct kittycat_string *new_string_cloned(const char *const str, const size_t len)
{
    char *cp_str = strndup(str, len); // Copy the string to prevent memory leaks
    struct kittycat_string *s = malloc(sizeof(struct kittycat_string));
    s->str = cp_str;
    s->len = len;
    s->__isCloned = true;
    return s;
}

// Clone a string
struct kittycat_string *string_clone(const struct kittycat_string *const s)
{
    return new_string_cloned(s->str, s->len);
}

// Clone the chars of a string
char *string_clone_chars(const struct kittycat_string *const s)
{
    return strndup(s->str, s->len);
}

struct kittycat_string *string_substr(const struct kittycat_string *const s, const size_t start, const size_t end)
{
    size_t len = end - start;
    char *str = malloc(len + 1);
    memcpy(str, s->str + start, len);
    str[len] = '\0'; // Null terminate the string
    struct kittycat_string *ns = new_string(str, len);
    ns->__isCloned = true; // Mark as cloned as string_substr copies to another string
    return ns;
}

void string_splitn(struct kittycat_string *s, const char sep, struct kittycat_string **out, const size_t n)
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

// Concatenate two strings together
//
// Note: this returns a new string that must be freed by the caller
struct kittycat_string *string_concat(struct kittycat_string *s1, struct kittycat_string *s2)
{
    size_t len = s1->len + s2->len;
    char *str = malloc(len + 1);                       // Allocate memory for the concatenated string + 1 for null terminator
    memcpy(str, s1->str, s1->len);                     // Copy the first string
    memcpy(str + s1->len, s2->str, s2->len);           // Copy the second string
    str[len] = '\0';                                   // Null terminate the string
    struct kittycat_string *ns = new_string(str, len); // Return the concatenated string
    ns->__isCloned = true;                             // Mark as cloned as string_concat copies to another string
    return ns;
}

size_t string_length(struct kittycat_string *s)
{
    return s->len;
}

void string_free(struct kittycat_string *s)
{
    // Already freed if NULL
    if (s == NULL)
        return;
    if (s->__isCloned)
    {
        free(s->str); // Free the string
        s->str = NULL;
    }
    free(s);
    s = NULL;
}

void string_arr_free(struct kittycat_string **arr, const size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        string_free(arr[i]);
    }
}

bool string_is_empty(const struct kittycat_string *const s)
{
    return s == NULL || s->len == 0 || s->str == NULL;
}

bool string_is_equal(const struct kittycat_string *const s1, const struct kittycat_string *const s2)
{
    if (s1->len != s2->len)
    {
        return false;
    }

    return strcmp(s1->str, s2->str) == 0;
}

bool string_is_equal_char(const struct kittycat_string *const s, const char *c)
{
    return strcmp(s->str, c) == 0;
}

bool string_contains(const struct kittycat_string *const s, const char c)
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

int string_print(struct kittycat_string *s, FILE *stream)
{
    return fwrite(s->str, sizeof(char), s->len, stream);
}

struct kittycat_string *string_trim_prefix(const struct kittycat_string *const s, const char c)
{
    size_t start = 0;
    while (s->str[start] == c)
    {
        start++;
    }

    return string_substr(s, start, s->len);
}
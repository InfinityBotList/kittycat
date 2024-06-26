#include "kc_string.h"

static void *(*__kittycat_malloc)(size_t) = NULL;
static void *(*__kittycat_realloc)(void *, size_t) = NULL;
static void (*__kittycat_free)(void *) = NULL;
static void *(*__kittycat_memcpy)(void *, const void *, size_t) = NULL;

void kittycat_kc_string_set_allocator(
    void *(*malloc)(size_t),
    void *(*realloc)(void *, size_t),
    void (*free)(void *),
    void *(*memcpy)(void *, const void *, size_t))
{
    __kittycat_malloc = malloc;
    __kittycat_realloc = realloc;
    __kittycat_free = free;
    __kittycat_memcpy = memcpy;
}

// Create a new string
//
// Note: callers must free the string after use using `string_free`
// Note 2: Callers must manually call strndup if the string should be copied (or use kittycat_string_clone_from_chararr). new_string will store the char* array directly in the string
struct kittycat_string *kittycat_string_new(char *str, const size_t len)
{
    struct kittycat_string *s = __kittycat_malloc(sizeof(struct kittycat_string));
    s->str = str;
    s->len = len;
    s->__isCloned = false;
    return s;
}

#ifdef C99
// Custom strndup implementation for C99, untested(!!)
char *__kittycat_strndup(const char *s, size_t n)
{
    char *result;
    size_t len = strlen(s);

    if (n < len)
    {
        len = n;
    }

    result = __kittycat_malloc(len + 1);
    if (!result)
    {
        return NULL;
    }

    result[len] = '\0';
    return (char *)__kittycat_memcpy(result, s, len);
}
#else
#define __kittycat_strndup strndup
#endif

struct kittycat_string *kittycat_string_clone_from_chararr(const char *const str, const size_t len)
{
    char *cp_str = __kittycat_strndup(str, len); // Copy the string to prevent memory leaks
    struct kittycat_string *s = __kittycat_malloc(sizeof(struct kittycat_string));
    s->str = cp_str;
    s->len = len;
    s->__isCloned = true;
    return s;
}

struct kittycat_string *kittycat_string_clone(const struct kittycat_string *const s)
{
    return kittycat_string_clone_from_chararr(s->str, s->len);
}

struct kittycat_string *kittycat_string_substr(const struct kittycat_string *const s, const size_t start, const size_t end)
{
    size_t len = end - start;
    char *str = __kittycat_malloc(len + 1);
    __kittycat_memcpy(str, s->str + start, len);
    str[len] = '\0'; // Null terminate the string
    struct kittycat_string *ns = kittycat_string_new(str, len);
    ns->__isCloned = true; // Mark as cloned as kittycat_string_substr copies to another string
    return ns;
}

int kittycat_string_splitn(struct kittycat_string *s, const char sep, struct kittycat_string **out, const size_t n)
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
            out[j] = kittycat_string_substr(s, start, i);
            j++;
            start = i + 1;
        }
        i++;
    }

    out[j] = kittycat_string_substr(s, start, s->len);

    return j + 1; // Return the number of splits performed
}

struct kittycat_string *kittycat_string_concat(struct kittycat_string *s1, struct kittycat_string *s2)
{
    size_t len = s1->len + s2->len;
    char *str = __kittycat_malloc(len + 1);                     // Allocate memory for the concatenated string + 1 for null terminator
    __kittycat_memcpy(str, s1->str, s1->len);                   // Copy the first string
    __kittycat_memcpy(str + s1->len, s2->str, s2->len);         // Copy the second string
    str[len] = '\0';                                            // Null terminate the string
    struct kittycat_string *ns = kittycat_string_new(str, len); // Return the concatenated string
    ns->__isCloned = true;                                      // Mark as cloned as kittycat_string_concat copies to another string
    return ns;
}

void kittycat_string_free(struct kittycat_string *s)
{
    // Already freed if NULL
    if (!s)
        return;
    if (s->__isCloned)
    {
        __kittycat_free(s->str); // Free the string
        s->str = NULL;
    }
    __kittycat_free(s);
    s = NULL;
}

void kittycat_string_arr_free(struct kittycat_string **arr, const size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        kittycat_string_free(arr[i]);
    }
}

bool kittycat_string_empty(const struct kittycat_string *const s)
{
    return !(s) || s->len == 0 || !(s->str);
}

bool kittycat_string_equal(const struct kittycat_string *const s1, const struct kittycat_string *const s2)
{
    if (s1->len != s2->len)
    {
        return false;
    }

    return strcmp(s1->str, s2->str) == 0;
}

bool kittycat_string_contains(const struct kittycat_string *const s, const char c)
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

int kittycat_string_print(struct kittycat_string *s, FILE *stream)
{
    return fwrite(s->str, sizeof(char), s->len, stream);
}

struct kittycat_string *kittycat_string_trim_prefix(const struct kittycat_string *const s, const char c)
{
    size_t start = 0;
    while (s->str[start] == c)
    {
        start++;
    }

    return kittycat_string_substr(s, start, s->len);
}
#include "prelude.h"

// String
struct kittycat_string
{
    char *str;
    size_t len;

    // Internal
    bool __isCloned;
};

// String functions

// Create a new string
//
// Note: callers must free the string after use using `kittycat_string_free`. This will *not* free the underlying char array
struct kittycat_string *kittycat_string_new(char *str, const size_t len);

// Create a new string
//
// Note: callers must free the string after use using `kittycat_string_free`
// Note 2: This function will copy the string to prevent memory leaks and internally mark the string as cloned.
// When `kittycat_string_free` is called, the underlying char array will also be freed
struct kittycat_string *kittycat_string_clone_from_chararr(const char *const str, const size_t len);

// Helper function. Equivalent to calling `kittycat_string_clone_from_chararr(s->str, s->len)`
struct kittycat_string *kittycat_string_clone(const struct kittycat_string *const s);

// Returns the substring of the string from start to end. Note that this *copies* the string and internally marks the string as cloned.
// Like `kittycat_string_clone_from_chararr`, the underlying char array will be freed when `kittycat_string_free` is called
struct kittycat_string *kittycat_string_substr(const struct kittycat_string *const s, const size_t start, const size_t end);

// Splits the string at most N times by a seperator `sep`, returning the number of splits performed
int kittycat_string_splitn(struct kittycat_string *s, const char sep, struct kittycat_string **out, const size_t n);

// Concatenate two strings together
//
// Note: this returns a new string that must be freed by the caller by `kittycat_string_free`. Note that the returned string is marked as cloned
// and calling `kittycat_string_free` will also free the underlying char array
struct kittycat_string *kittycat_string_concat(struct kittycat_string *s1, struct kittycat_string *s2);

// Frees the string
void kittycat_string_free(struct kittycat_string *s);

// Helper method. Loops over `n` kittycat_strings and calls `kittycat_string_free` on all of them
void kittycat_string_arr_free(struct kittycat_string **arr, const size_t n);

// Returns if a string `s` is empty or not
bool kittycat_string_empty(const struct kittycat_string *const s);

// Returns if two strings are equal or not
bool kittycat_string_equal(const struct kittycat_string *const s1, const struct kittycat_string *const s2);

// Returns if a string is equal to a char array
//
// This is faster than naive `strcmp` on the underlying char array as it also equates the lengths of the strings
bool kittycat_string_equal_char(const struct kittycat_string *const s, const char *c, const size_t len);

// Returns if a string `s` contains a character `c`
bool kittycat_string_contains(const struct kittycat_string *const s, const char c);

// Prints a string to a file stream
int kittycat_string_print(struct kittycat_string *s, FILE *stream);

// Trims the prefix of a string `s` by a character `c`
//
// EX: `kittycat_string_trim_prefix("aabc", 'a')` will return "bc"
struct kittycat_string *kittycat_string_trim_prefix(const struct kittycat_string *const s, const char c);
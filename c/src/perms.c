#include "kc_string.c"

struct Permission
{
    struct string *namespace;
    struct string *perm;
    bool negator;
};

struct Permission *new_permission(char *namespace, char *perm, bool negator)
{
    struct Permission *p = malloc(sizeof(struct Permission));
    p->namespace = string_from_char(namespace);
    p->perm = string_from_char(perm);
    p->negator = negator;
    return p;
}

// Creates a new permission from a string
//
// Note 1: Caller must free the permission after use using `permission_free`
// Note 2: The string must be in the format `namespace.perm`
// Note 3: This function may modify the string inputted
struct Permission *permission_from_str(struct string *str)
{
    if (str->len == 0)
    {
        return NULL;
    }

    struct string *str_split[2];
    string_splitn(str, '.', str_split, 2);

    // If first character is ~, then it is a negator
    bool negator = str_split[0]->str[0] == '~';

    // If negator, remove the ~
    if (negator)
    {
        struct string *new_perm = string_trim_prefix(str_split[0], '~');
        string_free(str_split[0]);
        str_split[0] = new_perm;
        str_split[0]->len--;
    }

    // If perm is empty, then namespace is global and perm is first part
    struct Permission *p;
    if (string_is_empty(str_split[1]))
    {
        p = new_permission("global", str_split[0]->str, negator);
    }
    else
    {
        p = new_permission(str_split[0]->str, str_split[1]->str, negator);
    }

    string_arr_free(str_split, 2);

    return p;
}

struct string *permission_to_str(struct Permission *p)
{
    // Permissions are of the form `namespace.perm`
    char *namespace = strndup(p->namespace->str, p->namespace->len);
    char *perm = strndup(p->perm->str, p->perm->len);

    char *finalPerm;

    if (p->negator)
    {
        finalPerm = malloc(p->namespace->len + p->perm->len + 3);
        snprintf(finalPerm, p->namespace->len + p->perm->len + 3, "~%s.%s", namespace, perm);
    }
    else
    {
        finalPerm = malloc(p->namespace->len + p->perm->len + 2);
        snprintf(finalPerm, p->namespace->len + p->perm->len + 2, "%s.%s", namespace, perm);
    }

    struct string *str = string_from_char(finalPerm);

    free(namespace);
    free(perm);
    free(finalPerm);

    return str;
}

void permission_free(struct Permission *p)
{
    string_free(p->namespace);
    string_free(p->perm);
    free(p);
}
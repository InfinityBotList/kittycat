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

struct PermissionList
{
    struct Permission **perms;
    size_t len;
};

struct PermissionList *new_permission_list()
{
    struct PermissionList *pl = malloc(sizeof(struct PermissionList));
    pl->perms = malloc(sizeof(struct Permission *));
    pl->len = 0;
    return pl;
}

void permission_list_add(struct PermissionList *pl, struct Permission *perm)
{
    pl->perms = realloc(pl->perms, (pl->len + 1) * sizeof(struct Permission *));
    pl->perms[pl->len] = perm;
    pl->len++;
}

void permission_list_rm(struct PermissionList *pl, size_t i)
{
    if (i >= pl->len)
    {
        return;
    }

    permission_free(pl->perms[i]);

    for (size_t j = i; j < pl->len - 1; j++)
    {
        pl->perms[j] = pl->perms[j + 1];
    }

    pl->len--;
    pl->perms = realloc(pl->perms, pl->len * sizeof(struct Permission *));
}

struct string *permission_list_join(struct PermissionList *pl, char *sep)
{
    struct string *joined = new_string("", 0);

    for (size_t i = 0; i < pl->len; i++)
    {
        struct string *perm_str = permission_to_str(pl->perms[i]);
        struct string *new_joined = string_concat(joined, perm_str);

        if (i != pl->len - 1)
        {
            struct string *sep_str = new_string(sep, strlen(sep));
            struct string *new_joined_sep = string_concat(new_joined, sep_str);
            string_free(sep_str);
            string_free(joined);
            string_free(new_joined);
            joined = new_joined_sep;
        }
        else
        {
            string_free(joined);
            joined = new_joined;
        }

        string_free(perm_str);
    }

    return joined;
}

void permission_list_free(struct PermissionList *pl)
{
    for (size_t i = 0; i < pl->len; i++)
    {
        permission_free(pl->perms[i]);
    }
    free(pl->perms);
    free(pl);
}

bool has_perm(struct PermissionList *perms, struct Permission *perm)
{
    bool has_perm = false;
    bool has_negator = false;

    for (size_t i = 0; i < perms->len; i++)
    {
        struct Permission *user_perm = perms->perms[i];

        // Special case of global.*
        if (!user_perm->negator && string_is_equal_char(user_perm->namespace, "global") && string_is_equal_char(user_perm->perm, "*"))
        {
            return true;
        }

        if ((string_is_equal(user_perm->namespace, perm->namespace) || string_is_equal_char(user_perm->namespace, "global")) &&
            (string_is_equal_char(user_perm->perm, "*") || string_is_equal(user_perm->perm, perm->perm)))
        {
            // We have to check for negator
            has_perm = true;

            if (user_perm->negator)
            {
                has_negator = true;
            }
        }
    }

    return has_perm && !has_negator;
}
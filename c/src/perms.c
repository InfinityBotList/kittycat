#include "kc_string.c"
#include "hashmap/hashmap.h"
struct Permission
{
    struct kittycat_string *namespace;
    struct kittycat_string *perm;
    bool negator;

    // Internal
    bool __isCloned;
};

struct Permission *new_permission(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct Permission *p = malloc(sizeof(struct Permission));
    p->namespace = namespace;
    p->perm = perm;
    p->negator = negator;
    p->__isCloned = false;
    return p;
}

// strndup is not available in C99
struct Permission *new_permission_cloned(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct Permission *p = malloc(sizeof(struct Permission));
    p->namespace = string_clone(namespace);
    p->perm = string_clone(perm);
    p->negator = negator;
    p->__isCloned = true;
    return p;
}

// Creates a new permission from a string
//
// Note 1: Caller must free the permission after use using `permission_free`
// Note 2: The string must be in the format `namespace.perm`
struct Permission *permission_from_str(struct kittycat_string *str)
{
    if (str->len == 0)
    {
        return NULL;
    }

    struct kittycat_string *str_split[2];
    string_splitn(str, '.', str_split, 2);

    // If first character is ~, then it is a negator
    bool negator = str_split[0]->str[0] == '~';

    // If negator, remove the ~
    if (negator)
    {
        struct kittycat_string *new_perm = string_trim_prefix(str_split[0], '~');
        string_free(str_split[0]);
        str_split[0] = new_perm;
    }

    // If perm is empty, then namespace is global and perm is first part
    struct Permission *p;
    if (string_is_empty(str_split[1]))
    {
        string_free(str_split[1]);
        p = new_permission(new_string("global", strlen("global")), str_split[0], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }
    else
    {
        p = new_permission(str_split[0], str_split[1], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }

    return p;
}

char *permission_to_str(struct Permission *p)
{
    // Permissions are of the form `namespace.perm`
    char *finalPerm;

    if (p->negator)
    {
        finalPerm = malloc(p->namespace->len + p->perm->len + 3);
        snprintf(finalPerm, p->namespace->len + p->perm->len + 3, "~%s.%s", p->namespace->str, p->perm->str);
    }
    else
    {
        finalPerm = malloc(p->namespace->len + p->perm->len + 2);
        snprintf(finalPerm, p->namespace->len + p->perm->len + 2, "%s.%s", p->namespace->str, p->perm->str);
    }

    return finalPerm;
}

void permission_free(struct Permission *p)
{
#if defined(DEBUG_FULL) || defined(DEBUG_FREE)
    printf("Freeing permission with ns %s and perm %s at ptr %p\n", p->namespace->str, p->perm->str, p);
#endif

    // Only call string_free if new_permission_cloned was used
    if (p->__isCloned)
    {
        string_free(p->namespace);
        string_free(p->perm);
    }
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

struct PermissionList *new_permission_list_with_perms(
    struct Permission **perms, size_t len)
{
    struct PermissionList *pl = malloc(sizeof(struct PermissionList));
    pl->perms = malloc(len * sizeof(struct Permission *));
    pl->len = len;
    for (size_t i = 0; i < len; i++)
    {
        pl->perms[i] = perms[i];
    }
    return pl;
}

// Joins a permission list to produce a string
//
// The canonical string representation is used for each individual input permission
// The returned string must be freed by the caller
struct kittycat_string *permission_list_join(struct PermissionList *pl, char *sep)
{
    struct kittycat_string *joined = new_string("", 0);

    for (size_t i = 0; i < pl->len; i++)
    {
        char *perm_chararr = permission_to_str(pl->perms[i]);
        struct kittycat_string *perm_str = new_string(perm_chararr, strlen(perm_chararr));
        struct kittycat_string *new_joined = string_concat(joined, perm_str);
        free(perm_chararr); // SAFETY: string_concat creates a copy of the string
        if (i != pl->len - 1)
        {
            struct kittycat_string *sep_str = new_string(sep, strlen(sep));
            struct kittycat_string *new_joined_sep = string_concat(new_joined, sep_str);
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

bool permission_lists_equal(struct PermissionList *pl1, struct PermissionList *pl2)
{
    if (pl1->len != pl2->len)
    {
        return false;
    }

    for (size_t i = 0; i < pl1->len; i++)
    {
        struct Permission *p1 = pl1->perms[i];
        struct Permission *p2 = pl2->perms[i];

        if (!string_is_equal(p1->namespace, p2->namespace) || !string_is_equal(p1->perm, p2->perm) || p1->negator != p2->negator)
        {
            return false;
        }
    }

    return true;
}

void permission_list_free(struct PermissionList *pl)
{
    if (pl == NULL)
    {
        return;
    }

    if (pl->perms != NULL)
    {
        for (size_t i = 0; i < pl->len; i++)
        {
            permission_free(pl->perms[i]);
            pl->perms[i] = NULL;
        }

        free(pl->perms);
        pl->perms = NULL;
    }

    free(pl);
    pl = NULL;
}

bool has_perm(const struct PermissionList *const perms, const struct Permission *const perm)
{
    bool has_perm = false;
    bool has_negator = false;

    for (size_t i = 0; i < perms->len; i++)
    {
        struct Permission *user_perm = perms->perms[i];

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        printf("perms: Namespace: %s, Perm: %s, Negator: %s\n", perm->namespace->str, perm->perm->str, perm->negator ? "true" : "false");
        printf("user_perms: Namespace: %s, Perm: %s, Negator: %s\n", user_perm->namespace->str, user_perm->perm->str, user_perm->negator ? "true" : "false");
#endif

        // Special case of global.*
        if (!user_perm->negator && string_is_equal_char(user_perm->namespace, "global") && string_is_equal_char(user_perm->perm, "*"))
        {
            return true;
        }

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        printf("NS = NS: %s, Perm = Perm: %s\n", string_is_equal(user_perm->namespace, perm->namespace) ? "true" : "false", string_is_equal(user_perm->perm, perm->perm) ? "true" : "false");
#endif

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

/* Permission resolution */

struct PartialStaffPosition
{
    // The id of the position
    struct kittycat_string *id;
    // The index of the permission. Lower means higher in the list of hierarchy
    int32_t index;
    // The preset permissions of this position
    struct PermissionList *perms;
};

struct PartialStaffPositionList
{
    struct PartialStaffPosition **positions;
    size_t len;
};

struct PartialStaffPosition *new_partial_staff_position(char *id, int32_t index, struct PermissionList *perms)
{
    struct PartialStaffPosition *p = malloc(sizeof(struct PartialStaffPosition));
    p->id = new_string(id, strlen(id));
    p->index = index;
    p->perms = perms;
    return p;
}

void partial_staff_position_free(struct PartialStaffPosition *p)
{
    // Already freed if NULL
    if (p == NULL)
    {
        return;
    }
    string_free(p->id);
    permission_list_free(p->perms);
    free(p);
    p = NULL;
}

struct PartialStaffPositionList *new_partial_staff_position_list()
{
    struct PartialStaffPositionList *pl = malloc(sizeof(struct PartialStaffPositionList));
    pl->positions = malloc(sizeof(struct PartialStaffPosition *));
    pl->len = 0;
    return pl;
}

void partial_staff_position_list_add(struct PartialStaffPositionList *pl, struct PartialStaffPosition *p)
{
    pl->positions = realloc(pl->positions, (pl->len + 1) * sizeof(struct PartialStaffPosition *));
    pl->positions[pl->len] = p;
    pl->len++;
}

void partial_staff_position_list_rm(struct PartialStaffPositionList *pl, size_t i)
{
    if (i >= pl->len)
    {
        return;
    }

    partial_staff_position_free(pl->positions[i]);

    for (size_t j = i; j < pl->len - 1; j++)
    {
        pl->positions[j] = pl->positions[j + 1];
    }

    pl->len--;
    pl->positions = realloc(pl->positions, pl->len * sizeof(struct PartialStaffPosition *));
}

void partial_staff_position_list_free(struct PartialStaffPositionList *pl)
{
    if (pl == NULL)
    {
        return;
    }
    for (size_t i = 0; i < pl->len; i++)
    {
        partial_staff_position_free(pl->positions[i]);
        pl->positions[i] = NULL;
    }
    free(pl->positions);
    pl->positions = NULL;
    free(pl);
    pl = NULL;
}

// A set of permissions for a staff member
//
// This is a list of permissions that the user has
struct StaffPermissions
{
    struct PartialStaffPositionList *user_positions;
    struct PermissionList *perm_overrides;
};

struct StaffPermissions *new_staff_permissions()
{
    struct StaffPermissions *sp = malloc(sizeof(struct StaffPermissions));
    sp->user_positions = new_partial_staff_position_list();
    sp->perm_overrides = new_permission_list();
    return sp;
}

void staff_permissions_free(struct StaffPermissions *sp)
{
    // Already freed if NULL
    if (sp == NULL)
    {
        return;
    }

    if (sp->user_positions != NULL)
    {
        partial_staff_position_list_free(sp->user_positions);
    }
    if (sp->perm_overrides != NULL)
    {
        permission_list_free(sp->perm_overrides);
    }
    free(sp);
    sp = NULL;
}

// Internally used for clearing permissions
struct __intArr
{
    size_t *arr;
    size_t len;
};

struct __intArr *new_int_arr()
{
    struct __intArr *ia = malloc(sizeof(struct __intArr));
    ia->arr = malloc(sizeof(int32_t));
    ia->len = 0;
    return ia;
}

void intArr_add(struct __intArr *ia, size_t i)
{
    ia->arr = realloc(ia->arr, (ia->len + 1) * sizeof(size_t));
    ia->arr[ia->len] = i;
    ia->len++;
}

void intArr_free(struct __intArr *ia)
{
    free(ia->arr);
    free(ia);
}

// A hashmap of permissions that are ordered
//
// Note that this struct is *unstable* and has ZERO API stability guarantees
struct __OrderedPermissionMap
{
    struct hashmap *map;
    struct Permission **order;
    size_t len;
};

uint64_t __permissionwc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct Permission *pc = item;
    struct Permission *p = (struct Permission *)pc;
    char *perm_str = permission_to_str(p);
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__permissionwc_hash: Hashing permission %s\n", perm_str);
#endif
    uint64_t hash = hashmap_sip(perm_str, strlen(perm_str), seed0, seed1);
    free(perm_str);
    return hash;
}

int __permissionwc_compare(const void *a, const void *b, void *udata)
{
    const struct Permission *pac = a;
    const struct Permission *pbc = b;
    struct Permission *pa = (struct Permission *)pac;
    struct Permission *pb = (struct Permission *)pbc;

    char *pa_str = permission_to_str(pa);
    char *pb_str = permission_to_str(pb);

    int cmp = strcmp(pa_str, pb_str);

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__permissionwc_compare: Comparing %s and %s: %d\n", pa_str, pb_str, cmp);
#endif

    free(pa_str);
    free(pb_str);

    return cmp;
}

struct __OrderedPermissionMap *__new_ordered_permission_map()
{
    struct __OrderedPermissionMap *opm = malloc(sizeof(struct __OrderedPermissionMap));
    opm->map = hashmap_new(sizeof(struct Permission), 0, 0, 0, __permissionwc_hash, __permissionwc_compare, NULL, NULL);
    opm->order = malloc(sizeof(struct Permission *));
    opm->len = 0;
    return opm;
}

struct Permission *__ordered_permission_map_get(struct __OrderedPermissionMap *opm, struct Permission *perm)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = permission_to_str(perm);
    printf("__ordered_permission_map_get: Getting permission %s\n", perm_str);
    free(perm_str);
#endif

    const void *pwc_void = hashmap_get(opm->map, perm);
    if (pwc_void == NULL)
    {
        return NULL;
    }
    struct Permission *pwc = (struct Permission *)pwc_void;
    return pwc;
}

// Deletes the permission from the ordered permission map
struct Permission *__ordered_permission_map_del(struct __OrderedPermissionMap *opm, struct Permission *perm)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = permission_to_str(perm);
    printf("__ordered_permission_map_del: Deleting permission %s\n", perm_str);
    free(perm_str);
#endif

    const void *pwc_void = hashmap_delete(opm->map, perm);

    if (pwc_void == NULL)
    {
// Nothing was deleted
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        char *perm_str = permission_to_str(perm);
        printf("__ordered_permission_map_del: Permission %s not found in map\n", perm_str);
        free(perm_str);
#endif
        return NULL;
    }

    struct Permission *pwc = (struct Permission *)pwc_void;

    // Find the index of the permission in the order
    size_t index = 0;

    for (size_t i = 0; i < opm->len; i++)
    {
        if (hashmap_get(opm->map, opm->order[i]) == NULL)
        {
            index = i;
            break;
        }
    }

    // Remove the permission from the order
    for (size_t i = index; i < opm->len - 1; i++)
    {
        opm->order[i] = opm->order[i + 1];
    }

    // Now set length correctly, as we have removed needed element
    opm->len = hashmap_count(opm->map);

    return pwc;
}

void __ordered_permission_map_free(struct __OrderedPermissionMap *opm)
{
    // Already freed if NULL
    if (opm == NULL)
    {
        return;
    }

    hashmap_free(opm->map);
    free(opm->order);
    free(opm);
    opm = NULL;
}

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
void __ordered_permission_map_printf_dbg(struct __OrderedPermissionMap *opm)
{
    for (size_t i = 0; i < opm->len; i++)
    {
        struct Permission *perm = opm->order[i];
        char *perm_str = permission_to_str(perm);
        printf("order iter: %s\n", perm_str);
        free(perm_str);
    }

    void *item;
    size_t i = 0;
    while (hashmap_iter(opm->map, &i, &item))
    {
        struct Permission *perm = item;
        char *perm_str = permission_to_str(perm);
        printf("applied perm: %s\n", perm_str);
        free(perm_str);
    }
}
#endif

void __ordered_permission_map_set(struct __OrderedPermissionMap *opm, struct Permission *p)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = permission_to_str(p);
    printf("__ordered_permission_map_set: Setting permission %s\n", perm_str);
    free(perm_str);
#endif

    hashmap_set(opm->map, p);
    opm->len = hashmap_count(opm->map);
    opm->order = realloc(opm->order, (opm->len * sizeof(struct Permission *)));
    opm->order[opm->len - 1] = p;

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    perm_str = permission_to_str(p);
    printf("__ordered_permission_map_set: Successfully set permission %s\n", perm_str);
    free(perm_str);
#endif

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    __ordered_permission_map_printf_dbg(opm);
#endif
}

void __ordered_permission_map_clear(struct __OrderedPermissionMap *opm)
{
    hashmap_clear(opm->map, false);
    opm->len = 0;
    free(opm->order);
    opm->order = malloc(sizeof(struct Permission *));
}

// Resolves the permissions of a staff member
//
// Note: consumers must free the ordered permission map after use
//
// For a general purpose function, use `staff_permissions_resolve`
//
// Note: multi-threaded functions should also use `staff_permissions_resolve` which creates the (not-threadsafe) hashmap on each invocation
struct PermissionList *__staff_permissions_resolve(const struct StaffPermissions *const sp, struct __OrderedPermissionMap *opm)
{
    struct PartialStaffPositionList *userPositions = new_partial_staff_position_list();

    for (size_t i = 0; i < sp->user_positions->len; i++)
    {
        struct PartialStaffPosition *pos = sp->user_positions->positions[i];
        partial_staff_position_list_add(userPositions, pos);
    }

    // Add the permission overrides as index 0
    struct PartialStaffPosition *permOverrides = new_partial_staff_position("perm_overrides", 0, sp->perm_overrides);
    partial_staff_position_list_add(userPositions, permOverrides);

    // Sort the positions by index in descending order
    for (size_t i = 0; i < userPositions->len; i++)
    {
        for (size_t j = i + 1; j < userPositions->len; j++)
        {
            if (userPositions->positions[i]->index < userPositions->positions[j]->index)
            {
                struct PartialStaffPosition *temp = userPositions->positions[i];
                userPositions->positions[i] = userPositions->positions[j];
                userPositions->positions[j] = temp;
            }
        }
    }

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_POSITION_LIST)
    // Send list of positions
    for (size_t i = 0; i < userPositions->len; i++)
    {
        struct PartialStaffPosition *pos = userPositions->positions[i];
        struct kittycat_string *permsStr = permission_list_join(pos->perms, ", ");
        printf("Position %s with index %d has perms: %s\n", pos->id->str, pos->index, permsStr->str);
        string_free(permsStr);
    }
#endif

    for (size_t i = 0; i < userPositions->len; i++)
    {
        struct PartialStaffPosition *pos = userPositions->positions[i];
        for (size_t j = 0; j < pos->perms->len; j++)
        {
            struct Permission *perm = pos->perms->perms[j];
            if (string_is_equal_char(perm->perm, "@clear"))
            {
                if (string_is_equal_char(perm->namespace, "global"))
                {
                    // Clear all permissions
                    __ordered_permission_map_clear(opm);
                }
                else
                {
                    // Clear all perms with this namespace
                    struct __intArr *toRemove = new_int_arr();
                    for (size_t k = 0; k < opm->len; k++)
                    {
                        struct Permission *key = opm->order[k];
                        if (string_is_equal(key->namespace, perm->namespace))
                        {
                            intArr_add(toRemove, k);
                        }
                    }

                    for (size_t k = 0; k < toRemove->len; k++)
                    {
                        __ordered_permission_map_del(opm, opm->order[toRemove->arr[k]]);
                    }

                    intArr_free(toRemove);
                }

                continue;
            }

            if (perm->negator)
            {
                // Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
                struct Permission *nonNegated = new_permission(perm->namespace, perm->perm, false);
                struct Permission *pwc = __ordered_permission_map_get(opm, nonNegated);
                if (pwc != NULL)
                {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    printf("Non-negated exists for %s\n", permission_to_str(nonNegated));
#endif

                    // Remove old permission
                    __ordered_permission_map_del(opm, nonNegated);

                    // Add the negator
                    __ordered_permission_map_set(opm, perm);
                }
                else
                {
                    struct Permission *pwc = __ordered_permission_map_get(opm, perm);
                    if (pwc != NULL)
                    {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                        printf("Non-negated exists for %s\n", permission_to_str(perm));
#endif
                        // Case 3: The negator is already applied, so we can ignore it
                        permission_free(nonNegated);
                        continue;
                    }
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    char *perm_str = permission_to_str(perm);
                    printf("Non-negated does not exist for %s\n", perm_str);
                    free(perm_str);
#endif

                    // Then we can freely add the negator
                    __ordered_permission_map_set(opm, perm);
                }

                permission_free(nonNegated);
            }
            else
            {
                // Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
                if (string_is_equal_char(perm->perm, "*"))
                {
                    // Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap
                    struct __intArr *toRemove = new_int_arr();
                    for (size_t k = 0; k < opm->len; k++)
                    {
                        struct Permission *key = opm->order[k];
                        if (!(key->negator))
                        {
                            continue; // This special case only applies to negators
                        }
                        if (string_is_equal(key->namespace, perm->namespace))
                        {
                            // Then we can ignore this negator
                            intArr_add(toRemove, k);
                        }
                    }

                    for (size_t k = 0; k < toRemove->len; k++)
                    {
                        __ordered_permission_map_del(opm, opm->order[toRemove->arr[k]]);
                    }
                    intArr_free(toRemove);
                }
                // If its not a negator, first check if there's a negator
                struct Permission *negated = new_permission(perm->namespace, perm->perm, true);
                struct Permission *pwc = __ordered_permission_map_get(opm, negated);
                if (pwc != NULL)
                {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    char *perm_str = permission_to_str(negated);
                    char *negated_str = permission_to_str(pwc);
                    printf("Negator exists for %s as %s\n", perm_str, negated_str);
                    free(perm_str);
                    free(negated_str);
#endif
                    // Remove old permission
                    __ordered_permission_map_del(opm, negated);

                    // Add the permission
                    __ordered_permission_map_set(opm, perm);
                }
                else
                {
                    struct Permission *pwc = __ordered_permission_map_get(opm, perm);
                    if (pwc != NULL)
                    {
                        // Case 3: The permission is already applied, so we can ignore it
                        permission_free(negated);
                        continue;
                    }
                    // Then we can freely add the permission
                    __ordered_permission_map_set(opm, perm);
                }

                permission_free(negated);
            }
        }
    }

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    __ordered_permission_map_printf_dbg(opm);
#endif

    struct PermissionList *appliedPerms = new_permission_list();

    for (size_t i = 0; i < opm->len; i++)
    {
        struct Permission *perm = opm->order[i];
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        char *perm_str = permission_to_str(perm);
        printf("order iter: %s\n", perm_str);
        free(perm_str);
#endif
        // Copy the permission
        struct Permission *new_perm = new_permission(perm->namespace, perm->perm, perm->negator);
        permission_list_add(appliedPerms, new_perm);
    }

    string_free(permOverrides->id);
    free(permOverrides);
    free(userPositions->positions);
    free(userPositions);

    return appliedPerms;
}

// Resolves the permissions of a staff member
//
// For increased efficiency at the cost of no API stability guarantees and loss of thread safety, use `__staff_permissions_resolve`
struct PermissionList *staff_permissions_resolve(const struct StaffPermissions *const sp)
{
    struct __OrderedPermissionMap *opm = __new_ordered_permission_map();
    struct PermissionList *appliedPerms = __staff_permissions_resolve(sp, opm);
    __ordered_permission_map_free(opm);
    return appliedPerms;
}
#include "kc_string.c"
#include "hashmap/hashmap.h"
struct KittycatPermission
{
    struct kittycat_string *namespace;
    struct kittycat_string *perm;
    bool negator;

    // Internal
    bool __isCloned;
};

struct KittycatPermission *kittycat_new_permission(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct KittycatPermission *p = malloc(sizeof(struct KittycatPermission));
    p->namespace = namespace;
    p->perm = perm;
    p->negator = negator;
    p->__isCloned = false;
    return p;
}

// Creates a new KittycatPermission from a string.
// The string must be in the format `namespace.perm`
// Caller must free the KittycatPermission after use using `kittycat_permission_free`
// Note that the original namespace+perm will be cloned and automatically freed when the KittycatPermission is freed using `kittycat_permission_free`
struct KittycatPermission *kittycat_new_permission_cloned(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct KittycatPermission *p = malloc(sizeof(struct KittycatPermission));
    p->namespace = string_clone(namespace);
    p->perm = string_clone(perm);
    p->negator = negator;
    p->__isCloned = true;
    return p;
}

// Creates a new KittycatPermission from a string
//
// Note 1: Caller must free the KittycatPermission after use using `kittycat_permission_free`
// Note 2: The string must be in the format `namespace.perm`
struct KittycatPermission *kittycat_permission_new_from_str(struct kittycat_string *str)
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
    struct KittycatPermission *p;
    if (string_is_empty(str_split[1]))
    {
        string_free(str_split[1]);
        p = kittycat_new_permission(new_string("global", strlen("global")), str_split[0], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }
    else
    {
        p = kittycat_new_permission(str_split[0], str_split[1], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }

    return p;
}

char *kittycat_permission_to_str(struct KittycatPermission *p)
{
    // KittycatPermissions are of the form `namespace.perm`
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

void kittycat_permission_free(struct KittycatPermission *p)
{
#if defined(DEBUG_FULL) || defined(DEBUG_FREE)
    printf("Freeing KittycatPermission with ns %s and perm %s at ptr %p\n", p->namespace->str, p->perm->str, p);
#endif

    // Only call string_free if kittycat_new_permission_cloned was used
    if (p->__isCloned)
    {
        string_free(p->namespace);
        string_free(p->perm);
    }
    free(p);
}

struct KittycatPermissionList
{
    struct KittycatPermission **perms;
    size_t len;
};

struct KittycatPermissionList *kittycat_permission_list_new()
{
    struct KittycatPermissionList *pl = malloc(sizeof(struct KittycatPermissionList));
    pl->perms = malloc(sizeof(struct KittycatPermission *));
    pl->len = 0;
    return pl;
}

void kittycat_permission_list_add(struct KittycatPermissionList *pl, struct KittycatPermission *const perm)
{
    pl->perms = realloc(pl->perms, (pl->len + 1) * sizeof(struct KittycatPermission *));
    pl->perms[pl->len] = perm;
    pl->len++;
}

void kittycat_permission_list_rm(struct KittycatPermissionList *pl, size_t i)
{
    if (i >= pl->len)
    {
        return;
    }

    kittycat_permission_free(pl->perms[i]);

    for (size_t j = i; j < pl->len - 1; j++)
    {
        pl->perms[j] = pl->perms[j + 1];
    }

    pl->len--;
    pl->perms = realloc(pl->perms, pl->len * sizeof(struct KittycatPermission *));
}

struct KittycatPermissionList *kittycat_permission_list_new_with_perms(
    struct KittycatPermission **perms, size_t len)
{
    struct KittycatPermissionList *pl = malloc(sizeof(struct KittycatPermissionList));
    pl->perms = malloc(len * sizeof(struct KittycatPermission *));
    pl->len = len;
    for (size_t i = 0; i < len; i++)
    {
        pl->perms[i] = perms[i];
    }
    return pl;
}

// Joins a KittycatPermission list to produce a string
//
// The canonical string representation is used for each individual input KittycatPermission
// The returned string must be freed by the caller
struct kittycat_string *kittycat_permission_list_join(struct KittycatPermissionList *pl, char *sep)
{
    struct kittycat_string *joined = new_string("", 0);

    for (size_t i = 0; i < pl->len; i++)
    {
        char *perm_chararr = kittycat_permission_to_str(pl->perms[i]);
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

bool kittycat_permission_lists_equal(struct KittycatPermissionList *pl1, struct KittycatPermissionList *pl2)
{
    if (pl1->len != pl2->len)
    {
        return false;
    }

    for (size_t i = 0; i < pl1->len; i++)
    {
        struct KittycatPermission *p1 = pl1->perms[i];
        struct KittycatPermission *p2 = pl2->perms[i];

        if (!string_is_equal(p1->namespace, p2->namespace) || !string_is_equal(p1->perm, p2->perm) || p1->negator != p2->negator)
        {
            return false;
        }
    }

    return true;
}

void kittycat_permission_list_free(struct KittycatPermissionList *pl)
{
    if (pl == NULL)
    {
        return;
    }

    if (pl->perms != NULL)
    {
        for (size_t i = 0; i < pl->len; i++)
        {
            kittycat_permission_free(pl->perms[i]);
            pl->perms[i] = NULL;
        }

        free(pl->perms);
        pl->perms = NULL;
    }

    free(pl);
    pl = NULL;
}

bool kittycat_has_perm(const struct KittycatPermissionList *const perms, const struct KittycatPermission *const perm)
{
    bool has_perm = false;
    bool has_negator = false;

    for (size_t i = 0; i < perms->len; i++)
    {
        struct KittycatPermission *user_perm = perms->perms[i];

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

/* KittycatPermission resolution */

struct PartialStaffPosition
{
    // The id of the position
    struct kittycat_string *id;
    // The index of the KittycatPermission. Lower means higher in the list of hierarchy
    int32_t index;
    // The preset KittycatPermissions of this position
    struct KittycatPermissionList *perms;
};

struct PartialStaffPositionList
{
    struct PartialStaffPosition **positions;
    size_t len;
};

struct PartialStaffPosition *partial_staff_position_new(char *id, int32_t index, struct KittycatPermissionList *perms)
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
    kittycat_permission_list_free(p->perms);
    free(p);
    p = NULL;
}

struct PartialStaffPositionList *partial_staff_position_list_new()
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

// A set of KittycatPermissions for a staff member
//
// This is a list of KittycatPermissions that the user has
struct StaffKittycatPermissions
{
    struct PartialStaffPositionList *user_positions;
    struct KittycatPermissionList *perm_overrides;
};

struct StaffKittycatPermissions *kittycat_staff_permissions_new()
{
    struct StaffKittycatPermissions *sp = malloc(sizeof(struct StaffKittycatPermissions));
    sp->user_positions = partial_staff_position_list_new();
    sp->perm_overrides = kittycat_permission_list_new();
    return sp;
}

void kittycat_staff_permissions_free(struct StaffKittycatPermissions *sp)
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
        kittycat_permission_list_free(sp->perm_overrides);
    }
    free(sp);
    sp = NULL;
}

// Internally used for clearing KittycatPermissions
struct __KittycatToRemoveArr
{
    size_t *arr;
    size_t len;
};

struct __KittycatToRemoveArr *__kittycat_toRemove_arr_new()
{
    struct __KittycatToRemoveArr *ia = malloc(sizeof(struct __KittycatToRemoveArr));
    ia->arr = malloc(sizeof(int32_t));
    ia->len = 0;
    return ia;
}

void __kittycat_toRemove_arr_add(struct __KittycatToRemoveArr *ia, size_t i)
{
    ia->arr = realloc(ia->arr, (ia->len + 1) * sizeof(size_t));
    ia->arr[ia->len] = i;
    ia->len++;
}

void __kittycat_toRemove_arr_free(struct __KittycatToRemoveArr *ia)
{
    free(ia->arr);
    free(ia);
}

// A kittycat_hashmap of KittycatPermissions that are ordered
//
// Note that this struct is *unstable* and has ZERO API stability guarantees
struct __KittycatOrderedPermissionMap
{
    struct kittycat_hashmap *map;
    struct KittycatPermission **order;
    size_t len;
};

uint64_t __kittycat_permission_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct KittycatPermission *pc = item;
    struct KittycatPermission *p = (struct KittycatPermission *)pc;
    char *perm_str = kittycat_permission_to_str(p);
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__KittycatPermissionwc_hash: Hashing KittycatPermission %s\n", perm_str);
#endif
    uint64_t hash = kittycat_hashmap_sip(perm_str, strlen(perm_str), seed0, seed1);
    free(perm_str);
    return hash;
}

int __kittycat_permission_compare(const void *a, const void *b, void *udata)
{
    const struct KittycatPermission *pac = a;
    const struct KittycatPermission *pbc = b;
    struct KittycatPermission *pa = (struct KittycatPermission *)pac;
    struct KittycatPermission *pb = (struct KittycatPermission *)pbc;

    char *pa_str = kittycat_permission_to_str(pa);
    char *pb_str = kittycat_permission_to_str(pb);

    int cmp = strcmp(pa_str, pb_str);

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__kittycat_ordered_permission_compare: Comparing %s and %s: %d\n", pa_str, pb_str, cmp);
#endif

    free(pa_str);
    free(pb_str);

    return cmp;
}

struct __KittycatOrderedPermissionMap *__kittycat_ordered_permission_map_new()
{
    struct __KittycatOrderedPermissionMap *opm = malloc(sizeof(struct __KittycatOrderedPermissionMap));
    opm->map = kittycat_hashmap_new(sizeof(struct KittycatPermission), 0, 0, 0, __kittycat_permission_hash, __kittycat_permission_compare, NULL, NULL);
    opm->order = malloc(sizeof(struct KittycatPermission *));
    opm->len = 0;
    return opm;
}

struct KittycatPermission *__kittycat_ordered_permission_map_get(struct __KittycatOrderedPermissionMap *opm, struct KittycatPermission *perm)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = kittycat_permission_to_str(perm);
    printf("__kittycat_ordered_permission_map_get: Getting KittycatPermission %s\n", perm_str);
    free(perm_str);
#endif

    const void *pwc_void = kittycat_hashmap_get(opm->map, perm);
    if (pwc_void == NULL)
    {
        return NULL;
    }
    struct KittycatPermission *pwc = (struct KittycatPermission *)pwc_void;
    return pwc;
}

// Deletes the KittycatPermission from the ordered KittycatPermission map
struct KittycatPermission *__kittycat_ordered_permission_map_del(struct __KittycatOrderedPermissionMap *opm, struct KittycatPermission *perm)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = kittycat_permission_to_str(perm);
    printf("__kittycat_ordered_permission_map_del: Deleting KittycatPermission %s\n", perm_str);
    free(perm_str);
#endif

    const void *pwc_void = kittycat_hashmap_delete(opm->map, perm);

    if (pwc_void == NULL)
    {
// Nothing was deleted
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        char *perm_str = kittycat_permission_to_str(perm);
        printf(" __kittycat_ordered_permission_map_del: KittycatPermission %s not found in map\n", perm_str);
        free(perm_str);
#endif
        return NULL;
    }

    struct KittycatPermission *pwc = (struct KittycatPermission *)pwc_void;

    // Find the index of the KittycatPermission in the order
    size_t index = 0;

    for (size_t i = 0; i < opm->len; i++)
    {
        if (kittycat_hashmap_get(opm->map, opm->order[i]) == NULL)
        {
            index = i;
            break;
        }
    }

    // Remove the KittycatPermission from the order
    for (size_t i = index; i < opm->len - 1; i++)
    {
        opm->order[i] = opm->order[i + 1];
    }

    // Now set length correctly, as we have removed needed element
    opm->len = kittycat_hashmap_count(opm->map);

    return pwc;
}

void __kittycat_ordered_permission_map_free(struct __KittycatOrderedPermissionMap *opm)
{
    // Already freed if NULL
    if (opm == NULL)
    {
        return;
    }

    kittycat_hashmap_free(opm->map);
    free(opm->order);
    free(opm);
    opm = NULL;
}

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
void __kittycat_ordered_permission_map_printf_dbg(struct __KittycatOrderedPermissionMap *opm)
{
    for (size_t i = 0; i < opm->len; i++)
    {
        struct KittycatPermission *perm = opm->order[i];
        char *perm_str = kittycat_permission_to_str(perm);
        printf("order iter: %s\n", perm_str);
        free(perm_str);
    }

    void *item;
    size_t i = 0;
    while (kittycat_hashmap_iter(opm->map, &i, &item))
    {
        struct KittycatPermission *perm = item;
        char *perm_str = kittycat_permission_to_str(perm);
        printf("applied perm: %s\n", perm_str);
        free(perm_str);
    }
}
#endif

void __kittycat_ordered_permission_map_set(struct __KittycatOrderedPermissionMap *opm, struct KittycatPermission *p)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    char *perm_str = kittycat_permission_to_str(p);
    printf(" __kittycat_ordered_permission_map_set: Setting KittycatPermission %s\n", perm_str);
    free(perm_str);
#endif

    kittycat_hashmap_set(opm->map, p);
    opm->len = kittycat_hashmap_count(opm->map);
    opm->order = realloc(opm->order, (opm->len * sizeof(struct KittycatPermission *)));
    opm->order[opm->len - 1] = p;

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    perm_str = kittycat_permission_to_str(p);
    printf(" __kittycat_ordered_permission_map_set: Successfully set KittycatPermission %s\n", perm_str);
    free(perm_str);
#endif

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    __kittycat_ordered_permission_map_printf_dbg(opm);
#endif
}

void __kittycat_ordered_permission_map_clear(struct __KittycatOrderedPermissionMap *opm)
{
    kittycat_hashmap_clear(opm->map, false);
    opm->len = 0;
    free(opm->order);
    opm->order = malloc(sizeof(struct KittycatPermission *));
}

// Resolves the KittycatPermissions of a staff member
//
// Note: consumers must free the ordered KittycatPermission map after use
//
// For a general purpose function, use `kittycat_staff_permissions_resolve`
//
// Note: multi-threaded functions should also use `kittycat_staff_permissions_resolve` which creates the (not-threadsafe) kittycat_hashmap on each invocation
struct KittycatPermissionList *__kittycat_staff_permissions_resolve(const struct StaffKittycatPermissions *const sp, struct __KittycatOrderedPermissionMap *opm)
{
    struct PartialStaffPositionList *userPositions = partial_staff_position_list_new();

    for (size_t i = 0; i < sp->user_positions->len; i++)
    {
        struct PartialStaffPosition *pos = sp->user_positions->positions[i];
        partial_staff_position_list_add(userPositions, pos);
    }

    // Add the KittycatPermission overrides as index 0
    struct PartialStaffPosition *permOverrides = partial_staff_position_new("perm_overrides", 0, sp->perm_overrides);
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
        struct kittycat_string *permsStr = kittycat_permission_list_join(pos->perms, ", ");
        printf("Position %s with index %d has perms: %s\n", pos->id->str, pos->index, permsStr->str);
        string_free(permsStr);
    }
#endif

    for (size_t i = 0; i < userPositions->len; i++)
    {
        struct PartialStaffPosition *pos = userPositions->positions[i];
        for (size_t j = 0; j < pos->perms->len; j++)
        {
            struct KittycatPermission *perm = pos->perms->perms[j];
            if (string_is_equal_char(perm->perm, "@clear"))
            {
                if (string_is_equal_char(perm->namespace, "global"))
                {
                    // Clear all KittycatPermissions
                    __kittycat_ordered_permission_map_clear(opm);
                }
                else
                {
                    // Clear all perms with this namespace
                    struct __KittycatToRemoveArr *toRemove = __kittycat_toRemove_arr_new();
                    for (size_t k = 0; k < opm->len; k++)
                    {
                        struct KittycatPermission *key = opm->order[k];
                        if (string_is_equal(key->namespace, perm->namespace))
                        {
                            __kittycat_toRemove_arr_add(toRemove, k);
                        }
                    }

                    for (size_t k = 0; k < toRemove->len; k++)
                    {
                        __kittycat_ordered_permission_map_del(opm, opm->order[toRemove->arr[k]]);
                    }

                    __kittycat_toRemove_arr_free(toRemove);
                }

                continue;
            }

            if (perm->negator)
            {
                // Check what gave the KittycatPermission. We *know* its sorted so we don't need to do anything but remove if it exists
                struct KittycatPermission *nonNegated = kittycat_new_permission(perm->namespace, perm->perm, false);
                struct KittycatPermission *pwc = __kittycat_ordered_permission_map_get(opm, nonNegated);
                if (pwc != NULL)
                {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    printf("Non-negated exists for %s\n", kittycat_permission_to_str(nonNegated));
#endif

                    // Remove old KittycatPermission
                    __kittycat_ordered_permission_map_del(opm, nonNegated);

                    // Add the negator
                    __kittycat_ordered_permission_map_set(opm, perm);
                }
                else
                {
                    struct KittycatPermission *pwc = __kittycat_ordered_permission_map_get(opm, perm);
                    if (pwc != NULL)
                    {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                        printf("Non-negated exists for %s\n", kittycat_permission_to_str(perm));
#endif
                        // Case 3: The negator is already applied, so we can ignore it
                        kittycat_permission_free(nonNegated);
                        continue;
                    }
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    char *perm_str = kittycat_permission_to_str(perm);
                    printf("Non-negated does not exist for %s\n", perm_str);
                    free(perm_str);
#endif

                    // Then we can freely add the negator
                    __kittycat_ordered_permission_map_set(opm, perm);
                }

                kittycat_permission_free(nonNegated);
            }
            else
            {
                // Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
                if (string_is_equal_char(perm->perm, "*"))
                {
                    // Remove negators. As the KittycatPermissions are sorted, we can just check if a negator is in the kittycat_hashmap
                    struct __KittycatToRemoveArr *toRemove = __kittycat_toRemove_arr_new();
                    for (size_t k = 0; k < opm->len; k++)
                    {
                        struct KittycatPermission *key = opm->order[k];
                        if (!(key->negator))
                        {
                            continue; // This special case only applies to negators
                        }
                        if (string_is_equal(key->namespace, perm->namespace))
                        {
                            // Then we can ignore this negator
                            __kittycat_toRemove_arr_add(toRemove, k);
                        }
                    }

                    for (size_t k = 0; k < toRemove->len; k++)
                    {
                        __kittycat_ordered_permission_map_del(opm, opm->order[toRemove->arr[k]]);
                    }
                    __kittycat_toRemove_arr_free(toRemove);
                }
                // If its not a negator, first check if there's a negator
                struct KittycatPermission *negated = kittycat_new_permission(perm->namespace, perm->perm, true);
                struct KittycatPermission *pwc = __kittycat_ordered_permission_map_get(opm, negated);
                if (pwc != NULL)
                {
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
                    char *perm_str = kittycat_permission_to_str(negated);
                    char *negated_str = kittycat_permission_to_str(pwc);
                    printf("Negator exists for %s as %s\n", perm_str, negated_str);
                    free(perm_str);
                    free(negated_str);
#endif
                    // Remove old KittycatPermission
                    __kittycat_ordered_permission_map_del(opm, negated);

                    // Add the KittycatPermission
                    __kittycat_ordered_permission_map_set(opm, perm);
                }
                else
                {
                    struct KittycatPermission *pwc = __kittycat_ordered_permission_map_get(opm, perm);
                    if (pwc != NULL)
                    {
                        // Case 3: The KittycatPermission is already applied, so we can ignore it
                        kittycat_permission_free(negated);
                        continue;
                    }
                    // Then we can freely add the KittycatPermission
                    __kittycat_ordered_permission_map_set(opm, perm);
                }

                kittycat_permission_free(negated);
            }
        }
    }

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    __kittycat_ordered_permission_map_printf_dbg(opm);
#endif

    struct KittycatPermissionList *appliedPerms = kittycat_permission_list_new();

    for (size_t i = 0; i < opm->len; i++)
    {
        struct KittycatPermission *perm = opm->order[i];
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        char *perm_str = kittycat_permission_to_str(perm);
        printf("order iter: %s\n", perm_str);
        free(perm_str);
#endif
        // Copy the KittycatPermission
        struct KittycatPermission *new_perm = kittycat_new_permission(perm->namespace, perm->perm, perm->negator);
        kittycat_permission_list_add(appliedPerms, new_perm);
    }

    string_free(permOverrides->id);
    free(permOverrides);
    free(userPositions->positions);
    free(userPositions);

    return appliedPerms;
}

// Resolves the KittycatPermissions of a staff member
//
// For increased efficiency at the cost of no API stability guarantees and loss of thread safety, use `__kittycat_staff_permissions_resolve`
struct KittycatPermissionList *kittycat_staff_permissions_resolve(const struct StaffKittycatPermissions *const sp)
{
    struct __KittycatOrderedPermissionMap *opm = __kittycat_ordered_permission_map_new();
    struct KittycatPermissionList *appliedPerms = __kittycat_staff_permissions_resolve(sp, opm);
    __kittycat_ordered_permission_map_free(opm);
    return appliedPerms;
}

enum KittycatPermissionCheckPatchChangesResultState
{
    KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_OK,
    KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION,
    KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD,
};

// Returned on calling `kittycat_permission_check_patch_changes`
//
// Users should check if state is `KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_OK`
struct KittycatPermissionCheckPatchChangesResult
{
    enum KittycatPermissionCheckPatchChangesResultState state;
    struct KittycatPermissionList *failing_perms;
};

// Frees the KittycatPermissionCheckPatchChangesResult
void kittycat_permission_check_patch_changes_result_free(struct KittycatPermissionCheckPatchChangesResult *result)
{
    if (result->failing_perms != NULL)
    {
        kittycat_permission_list_free(result->failing_perms);
    }
    free(result);
}

// Converts a KittycatPermissionCheckPatchChangesResult to a string
// KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION => You do not have permission to add this permission: {perm[0]}
// KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD => You do not have permission to add wildcard permission {perm[0]} with negators due to lack of negator {perm[1]}"
char *kittycat_permission_check_patch_changes_result_to_str(struct KittycatPermissionCheckPatchChangesResult *result)
{
    if (result->state == KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION)
    {
        char *perm_str = kittycat_permission_to_str(result->failing_perms->perms[0]);

        size_t error_msg_len = strlen("You do not have permission to add this permission: ") + strlen(perm_str) + 1;
        char *error_msg = malloc(error_msg_len);
        snprintf(
            error_msg,
            error_msg_len,
            "You do not have permission to add this permission: %s",
            perm_str);

        free(perm_str);

        return error_msg;
    }
    else if (result->state == KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD)
    {
        char *perm_str = kittycat_permission_to_str(result->failing_perms->perms[0]);
        char *negator_str = kittycat_permission_to_str(result->failing_perms->perms[1]);

        size_t error_msg_len = strlen("You do not have permission to add wildcard permission ") + strlen(perm_str) + strlen(" with negators due to lack of negator ") + strlen(negator_str) + 1;
        char *error_msg = malloc(error_msg_len);
        snprintf(
            error_msg,
            error_msg_len,
            "You do not have permission to add wildcard permission %s with negators due to lack of negator %s",
            perm_str,
            negator_str);

        free(perm_str);
        free(negator_str);

        return error_msg;
    }
    else
    {
        return "";
    }
}

// Checks whether or not a resolved set of KittycatPermissions allows the addition or removal of a KittycatPermission to a position
struct KittycatPermissionCheckPatchChangesResult kittycat_check_patch_changes(
    struct KittycatPermissionList *manager_perms,
    struct KittycatPermissionList *current_perms,
    struct KittycatPermissionList *new_perms)
{
    struct kittycat_hashmap *hset_1 = kittycat_hashmap_new(
        sizeof(struct KittycatPermission),
        0,
        0,
        0,
        __kittycat_permission_hash,
        __kittycat_permission_compare,
        NULL,
        NULL);

    for (size_t i = 0; i < manager_perms->len; i++)
    {
        struct KittycatPermission *perm = manager_perms->perms[i];
        kittycat_hashmap_set(hset_1, perm);
    }

    struct kittycat_hashmap *hset_2 = kittycat_hashmap_new(
        sizeof(struct KittycatPermission),
        0,
        0,
        0,
        __kittycat_permission_hash,
        __kittycat_permission_compare,
        NULL,
        NULL);

    for (size_t i = 0; i < current_perms->len; i++)
    {
        struct KittycatPermission *perm = current_perms->perms[i];
        kittycat_hashmap_set(hset_2, perm);
    }

    struct KittycatPermissionList *changed = kittycat_permission_list_new();

    size_t iter = 0;
    void *item;
    while (kittycat_hashmap_iter(hset_1, &iter, &item))
    {
        // If unique to hset_1, then add to changed
        struct KittycatPermission *p = item;
        if (kittycat_hashmap_get(hset_2, p) == NULL)
        {
            kittycat_permission_list_add(changed, kittycat_new_permission(p->namespace, p->perm, p->negator));
        }
    }

    iter = 0;
    while (kittycat_hashmap_iter(hset_2, &iter, &item))
    {
        // If unique to hset_2, then add to changed
        struct KittycatPermission *p = item;
        if (kittycat_hashmap_get(hset_1, p) == NULL)
        {
            kittycat_permission_list_add(changed, p);
        }
    }

    kittycat_hashmap_free(hset_1);
    kittycat_hashmap_free(hset_2);

    for (size_t i = 0; i < changed->len; i++)
    {
        struct KittycatPermission *perm = changed->perms[i];

        // Strip the negator to check it
        struct KittycatPermission *resolved_perm = kittycat_new_permission(perm->namespace, perm->perm, false);

        // Check if the user has the KittycatPermission
        if (!kittycat_has_perm(manager_perms, resolved_perm))
        {
            kittycat_permission_free(resolved_perm);
            kittycat_permission_list_free(changed);

            return (struct KittycatPermissionCheckPatchChangesResult){
                .state = KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION,
                .failing_perms = kittycat_permission_list_new_with_perms(
                    (struct KittycatPermission *[]){
                        perm},
                    1),
            };
        }

        if (string_is_equal_char(perm->perm, "*"))
        {
            // Ensure that new_perms has *at least* negators that manager_perms has within the namespace
            for (size_t j = 0; j < manager_perms->len; j++)
            {
                struct KittycatPermission *perms = manager_perms->perms[j];
                if (perms->negator)
                {
                    continue;
                }

                if (string_is_equal(perms->namespace, perm->namespace))
                {
                    // Then we have a negator in the same namespace
                    bool in_new_perms = false;
                    for (size_t k = 0; k < new_perms->len; k++)
                    {
                        struct KittycatPermission *new_perm = new_perms->perms[k];
                        if (string_is_equal(new_perm->namespace, perm->namespace) && string_is_equal(new_perm->perm, perm->perm) && new_perm->negator == perm->negator)
                        {
                            in_new_perms = true;
                            break;
                        }
                    }

                    if (!in_new_perms)
                    {
                        kittycat_permission_free(resolved_perm);
                        kittycat_permission_list_free(changed);

                        return (struct KittycatPermissionCheckPatchChangesResult){
                            .state = KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD,
                            .failing_perms = kittycat_permission_list_new_with_perms(
                                (struct KittycatPermission *[]){
                                    perm,
                                    perms},
                                2),
                        };
                    }
                }
            }
        }

        kittycat_permission_free(resolved_perm); // Free the resolved KittycatPermission
    }

    kittycat_permission_list_free(changed); // Free the changed KittycatPermissions

    return (struct KittycatPermissionCheckPatchChangesResult){
        .state = KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_OK};
}
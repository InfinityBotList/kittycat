#include "perms.h"
#include "hashmap.h"

static void *(*__kittycat_malloc)(size_t) = NULL;
static void *(*__kittycat_realloc)(void *, size_t) = NULL;
static void (*__kittycat_free)(void *) = NULL;

void kittycat_perms_set_allocator(
    void *(*malloc)(size_t),
    void *(*realloc)(void *, size_t),
    void (*free)(void *))
{
    __kittycat_malloc = malloc;
    __kittycat_realloc = realloc;
    __kittycat_free = free;
}

const struct kittycat_string __kittycat_perm_global_ns = {"global", 6, false};
const struct kittycat_string __kittycat_perm_clear_perm = {"@clear", 6, false};
const struct kittycat_string __kittycat_perm_global_perm = {"*", 1, false};

struct KittycatPermission *kittycat_new_permission(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct KittycatPermission *p = __kittycat_malloc(sizeof(struct KittycatPermission));
    p->namespace = namespace;
    p->perm = perm;
    p->negator = negator;
    p->__isCloned = false;
    return p;
}

struct KittycatPermission *kittycat_new_permission_cloned(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator)
{
    struct KittycatPermission *p = __kittycat_malloc(sizeof(struct KittycatPermission));
    p->namespace = kittycat_string_clone(namespace);
    p->perm = kittycat_string_clone(perm);
    p->negator = negator;
    p->__isCloned = true;
    return p;
}

struct KittycatPermission *kittycat_permission_new_from_str(struct kittycat_string *str)
{
    if (str->len == 0)
    {
        return NULL;
    }

    struct kittycat_string *str_split[2];
    int split = kittycat_string_splitn(str, '.', str_split, 2);

    if (split == 0)
    {
        return NULL;
    }

    // If first character is ~, then it is a negator
    bool negator = str_split[0]->str[0] == '~';

    // If negator, remove the ~
    if (negator)
    {
        struct kittycat_string *new_perm = kittycat_string_trim_prefix(str_split[0], '~');
        kittycat_string_free(str_split[0]);
        str_split[0] = new_perm;
    }

    // If perm is empty, then namespace is global and perm is first part
    struct KittycatPermission *p;
    if (split < 2)
    {
        p = kittycat_new_permission(kittycat_string_new("global", strlen("global")), str_split[0], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }
    else
    {
        p = kittycat_new_permission(str_split[0], str_split[1], negator);
        p->__isCloned = true; // substr clones the string, so flag it as cloned
    }

    return p;
}

struct kittycat_string *kittycat_permission_to_str(struct KittycatPermission *p)
{
    // KittycatPermissions are of the form `namespace.perm`
    char *finalPerm;
    size_t len;

    if (p->negator)
    {
        len = p->namespace->len + p->perm->len + 3;
        finalPerm = __kittycat_malloc(len);
        snprintf(finalPerm, len, "~%s.%s", p->namespace->str, p->perm->str);
    }
    else
    {
        len = p->namespace->len + p->perm->len + 2;
        finalPerm = __kittycat_malloc(len);
        snprintf(finalPerm, len, "%s.%s", p->namespace->str, p->perm->str);
    }

    struct kittycat_string *ps = kittycat_string_new(finalPerm, len);
    ps->__isCloned = true; // We created a new string, so flag it as cloned
    return ps;
}

void kittycat_permission_free(struct KittycatPermission *p)
{
    // Only call string_free if kittycat_new_permission_cloned was used
    if (p->__isCloned)
    {
        kittycat_string_free(p->namespace);
        kittycat_string_free(p->perm);
    }
    __kittycat_free(p);
}

struct KittycatPermissionList *kittycat_permission_list_new()
{
    struct KittycatPermissionList *pl = __kittycat_malloc(sizeof(struct KittycatPermissionList));
    pl->perms = __kittycat_malloc(sizeof(struct KittycatPermission *));
    pl->len = 0;
    return pl;
}

void kittycat_permission_list_add(struct KittycatPermissionList *pl, struct KittycatPermission *const perm)
{
    pl->perms = __kittycat_realloc(pl->perms, (pl->len + 1) * sizeof(struct KittycatPermission *));
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
    pl->perms = __kittycat_realloc(pl->perms, pl->len * sizeof(struct KittycatPermission *));
}

struct KittycatPermissionList *kittycat_permission_list_new_with_perms(
    struct KittycatPermission **perms, size_t len)
{
    struct KittycatPermissionList *pl = __kittycat_malloc(sizeof(struct KittycatPermissionList));
    pl->perms = __kittycat_malloc(len * sizeof(struct KittycatPermission *));
    pl->len = len;
    for (size_t i = 0; i < len; i++)
    {
        pl->perms[i] = perms[i];
    }
    return pl;
}

struct kittycat_string *kittycat_permission_list_join(struct KittycatPermissionList *pl, char *sep)
{
    struct kittycat_string *joined = kittycat_string_new("", 0);

    for (size_t i = 0; i < pl->len; i++)
    {
        struct kittycat_string *perm_str = kittycat_permission_to_str(pl->perms[i]);
        struct kittycat_string *new_joined = kittycat_string_concat(joined, perm_str);
        kittycat_string_free(perm_str); // Free the perm_str as it is cloned anyways
        if (i != pl->len - 1)
        {
            struct kittycat_string *sep_str = kittycat_string_new(sep, strlen(sep));
            struct kittycat_string *new_joined_sep = kittycat_string_concat(new_joined, sep_str);
            kittycat_string_free(sep_str);
            kittycat_string_free(joined);
            kittycat_string_free(new_joined);
            joined = new_joined_sep;
        }
        else
        {
            kittycat_string_free(joined);
            joined = new_joined;
        }
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

        if (!kittycat_string_equal(p1->namespace, p2->namespace) || !kittycat_string_equal(p1->perm, p2->perm) || p1->negator != p2->negator)
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

        __kittycat_free(pl->perms);
        pl->perms = NULL;
    }

    __kittycat_free(pl);
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
        if (!user_perm->negator && kittycat_string_equal(user_perm->namespace, &__kittycat_perm_global_ns) && kittycat_string_equal(user_perm->perm, &__kittycat_perm_global_perm))
        {
            return true;
        }

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
        printf("NS = NS: %s, Perm = Perm: %s\n", kittycat_string_equal(user_perm->namespace, perm->namespace) ? "true" : "false", kittycat_string_equal(user_perm->perm, perm->perm) ? "true" : "false");
#endif

        if ((kittycat_string_equal(user_perm->namespace, perm->namespace) || kittycat_string_equal(user_perm->namespace, &__kittycat_perm_global_ns)) &&
            (kittycat_string_equal(user_perm->perm, &__kittycat_perm_global_perm) || kittycat_string_equal(user_perm->perm, perm->perm)))
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

struct KittycatPartialStaffPosition *kittycat_partial_staff_position_new(char *id, int32_t index, struct KittycatPermissionList *perms)
{
    struct KittycatPartialStaffPosition *p = __kittycat_malloc(sizeof(struct KittycatPartialStaffPosition));
    p->id = kittycat_string_new(id, strlen(id));
    p->index = index;
    p->perms = perms;
    return p;
}

void kittycat_partial_staff_position_free(struct KittycatPartialStaffPosition *p)
{
    // Already freed if NULL
    if (p == NULL)
    {
        return;
    }
    kittycat_string_free(p->id);
    kittycat_permission_list_free(p->perms);
    __kittycat_free(p);
    p = NULL;
}

struct KittycatPartialStaffPositionList *kittycat_partial_staff_position_list_new()
{
    struct KittycatPartialStaffPositionList *pl = __kittycat_malloc(sizeof(struct KittycatPartialStaffPositionList));
    pl->positions = __kittycat_malloc(sizeof(struct KittycatPartialStaffPosition *));
    pl->len = 0;
    return pl;
}

void kittycat_partial_staff_position_list_add(struct KittycatPartialStaffPositionList *pl, struct KittycatPartialStaffPosition *p)
{
    pl->positions = __kittycat_realloc(pl->positions, (pl->len + 1) * sizeof(struct KittycatPartialStaffPosition *));
    pl->positions[pl->len] = p;
    pl->len++;
}

void kittycat_partial_staff_position_list_rm(struct KittycatPartialStaffPositionList *pl, size_t i)
{
    if (i >= pl->len)
    {
        return;
    }

    kittycat_partial_staff_position_free(pl->positions[i]);

    for (size_t j = i; j < pl->len - 1; j++)
    {
        pl->positions[j] = pl->positions[j + 1];
    }

    pl->len--;
    pl->positions = __kittycat_realloc(pl->positions, pl->len * sizeof(struct KittycatPartialStaffPosition *));
}

void kittycat_partial_staff_position_list_free(struct KittycatPartialStaffPositionList *pl)
{
    if (pl == NULL)
    {
        return;
    }
    for (size_t i = 0; i < pl->len; i++)
    {
        kittycat_partial_staff_position_free(pl->positions[i]);
        pl->positions[i] = NULL;
    }
    __kittycat_free(pl->positions);
    pl->positions = NULL;
    __kittycat_free(pl);
    pl = NULL;
}

struct StaffKittycatPermissions *kittycat_staff_permissions_new()
{
    struct StaffKittycatPermissions *sp = __kittycat_malloc(sizeof(struct StaffKittycatPermissions));
    sp->user_positions = kittycat_partial_staff_position_list_new();
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
        kittycat_partial_staff_position_list_free(sp->user_positions);
    }
    if (sp->perm_overrides != NULL)
    {
        kittycat_permission_list_free(sp->perm_overrides);
    }
    __kittycat_free(sp);
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
    struct __KittycatToRemoveArr *ia = __kittycat_malloc(sizeof(struct __KittycatToRemoveArr));
    ia->arr = __kittycat_malloc(sizeof(int32_t));
    ia->len = 0;
    return ia;
}

void __kittycat_toRemove_arr_add(struct __KittycatToRemoveArr *ia, size_t i)
{
    ia->arr = __kittycat_realloc(ia->arr, (ia->len + 1) * sizeof(size_t));
    ia->arr[ia->len] = i;
    ia->len++;
}

void __kittycat_toRemove_arr_free(struct __KittycatToRemoveArr *ia)
{
    __kittycat_free(ia->arr);
    __kittycat_free(ia);
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
    struct kittycat_string *perm_str = kittycat_permission_to_str(p);
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__KittycatPermissionwc_hash: Hashing KittycatPermission %s\n", perm_str->str);
#endif
    uint64_t hash = kittycat_hashmap_sip(perm_str->str, perm_str->len, seed0, seed1);
    kittycat_string_free(perm_str);
    return hash;
}

int __kittycat_permission_compare(const void *a, const void *b, void *udata)
{
    const struct KittycatPermission *pac = a;
    const struct KittycatPermission *pbc = b;
    struct KittycatPermission *pa = (struct KittycatPermission *)pac;
    struct KittycatPermission *pb = (struct KittycatPermission *)pbc;

    struct kittycat_string *pa_str = kittycat_permission_to_str(pa);
    struct kittycat_string *pb_str = kittycat_permission_to_str(pb);

    bool cmp = kittycat_string_equal(pa_str, pb_str);

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    printf("__kittycat_ordered_permission_compare: Comparing %s and %s: %d\n", pa_str->str, pb_str->str, cmp);
#endif

    kittycat_string_free(pa_str);
    kittycat_string_free(pb_str);

    return cmp ? 0 : 1;
}

struct __KittycatOrderedPermissionMap *__kittycat_ordered_permission_map_new()
{
    struct __KittycatOrderedPermissionMap *opm = __kittycat_malloc(sizeof(struct __KittycatOrderedPermissionMap));
    opm->map = kittycat_hashmap_new(sizeof(struct KittycatPermission), 0, 0, 0, __kittycat_permission_hash, __kittycat_permission_compare, NULL, NULL);
    opm->order = __kittycat_malloc(sizeof(struct KittycatPermission *));
    opm->len = 0;
    return opm;
}

struct KittycatPermission *__kittycat_ordered_permission_map_get(struct __KittycatOrderedPermissionMap *opm, struct KittycatPermission *perm)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    struct kittycat_string *perm_str = kittycat_permission_to_str(perm);
    printf("__kittycat_ordered_permission_map_get: Getting KittycatPermission %s\n", perm_str->str);
    kittycat_string_free(perm_str);
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
    struct kittycat_string *perm_str = kittycat_permission_to_str(perm);
    printf("__kittycat_ordered_permission_map_del: Deleting KittycatPermission %s\n", perm_str->str);
    kittycat_string_free(perm_str);
#endif

    const void *pwc_void = kittycat_hashmap_delete(opm->map, perm);

    if (pwc_void == NULL)
    {
        // Nothing was deleted
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
    __kittycat_free(opm->order);
    __kittycat_free(opm);
    opm = NULL;
}

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
void __kittycat_ordered_permission_map_printf_dbg(struct __KittycatOrderedPermissionMap *opm)
{
    for (size_t i = 0; i < opm->len; i++)
    {
        struct KittycatPermission *perm = opm->order[i];
        struct kittycat_string *perm_str = kittycat_permission_to_str(perm);
        printf("order iter: %s\n", perm_str->str);
        kittycat_string_free(perm_str);
    }

    void *item;
    size_t i = 0;
    while (kittycat_hashmap_iter(opm->map, &i, &item))
    {
        struct KittycatPermission *perm = item;
        struct kittycat_string *perm_str = kittycat_permission_to_str(perm);
        printf("applied perm: %s\n", perm_str->str);
        kittycat_string_free(perm_str);
    }
}
#endif

void __kittycat_ordered_permission_map_set(struct __KittycatOrderedPermissionMap *opm, struct KittycatPermission *p)
{
#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    struct kittycat_string *perm_str = kittycat_permission_to_str(p);
    printf(" __kittycat_ordered_permission_map_set: Setting KittycatPermission %s\n", perm_str->str);
    kittycat_string_free(perm_str);
#endif

    kittycat_hashmap_set(opm->map, p);
    opm->len = kittycat_hashmap_count(opm->map);
    opm->order = __kittycat_realloc(opm->order, (opm->len * sizeof(struct KittycatPermission *)));
    opm->order[opm->len - 1] = p;

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    struct kittycat_string *perm_str = kittycat_permission_to_str(p);
    printf(" __kittycat_ordered_permission_map_set: Successfully set KittycatPermission %s\n", perm_str->str);
    kittycat_string_free(perm_str);
#endif

#if defined(DEBUG_FULL) || defined(DEBUG_PRINTF_MINI)
    __kittycat_ordered_permission_map_printf_dbg(opm);
#endif
}

void __kittycat_ordered_permission_map_clear(struct __KittycatOrderedPermissionMap *opm)
{
    kittycat_hashmap_clear(opm->map, false);
    opm->len = 0;
    __kittycat_free(opm->order);
    opm->order = __kittycat_malloc(sizeof(struct KittycatPermission *));
}

struct KittycatPermissionList *kittycat_staff_permissions_resolve(const struct StaffKittycatPermissions *const sp)
{
    struct __KittycatOrderedPermissionMap *opm = __kittycat_ordered_permission_map_new();
    struct KittycatPartialStaffPositionList *userPositions = kittycat_partial_staff_position_list_new();

    for (size_t i = 0; i < sp->user_positions->len; i++)
    {
        struct KittycatPartialStaffPosition *pos = sp->user_positions->positions[i];
        kittycat_partial_staff_position_list_add(userPositions, pos);
    }

    // Add the KittycatPermission overrides as index 0
    struct KittycatPartialStaffPosition *permOverrides = kittycat_partial_staff_position_new("perm_overrides", 0, sp->perm_overrides);
    kittycat_partial_staff_position_list_add(userPositions, permOverrides);

    // Sort the positions by index in descending order
    for (size_t i = 0; i < userPositions->len; i++)
    {
        for (size_t j = i + 1; j < userPositions->len; j++)
        {
            if (userPositions->positions[i]->index < userPositions->positions[j]->index)
            {
                struct KittycatPartialStaffPosition *temp = userPositions->positions[i];
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
        kittycat_string_free(permsStr);
    }
#endif

    for (size_t i = 0; i < userPositions->len; i++)
    {
        struct KittycatPartialStaffPosition *pos = userPositions->positions[i];
        for (size_t j = 0; j < pos->perms->len; j++)
        {
            struct KittycatPermission *perm = pos->perms->perms[j];
            if (kittycat_string_equal(perm->perm, &__kittycat_perm_clear_perm))
            {
                if (kittycat_string_equal(perm->namespace, &__kittycat_perm_global_ns))
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
                        if (kittycat_string_equal(key->namespace, perm->namespace))
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
                        // Case 3: The negator is already applied, so we can ignore it
                        kittycat_permission_free(nonNegated);
                        continue;
                    }

                    // Then we can freely add the negator
                    __kittycat_ordered_permission_map_set(opm, perm);
                }

                kittycat_permission_free(nonNegated);
            }
            else
            {
                // Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
                if (kittycat_string_equal(perm->perm, &__kittycat_perm_global_perm))
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
                        if (kittycat_string_equal(key->namespace, perm->namespace))
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
        struct kittycat_string *perm_str = kittycat_permission_to_str(perm);
        printf("order iter: %s\n", perm_str->str);
        kittycat_string_free(perm_str);
#endif
        // Copy the KittycatPermission
        struct KittycatPermission *new_perm = kittycat_new_permission(perm->namespace, perm->perm, perm->negator);
        kittycat_permission_list_add(appliedPerms, new_perm);
    }

    kittycat_string_free(permOverrides->id);
    __kittycat_free(permOverrides);
    __kittycat_free(userPositions->positions);
    __kittycat_free(userPositions);

    __kittycat_ordered_permission_map_free(opm);

    return appliedPerms;
}

void kittycat_permission_check_patch_changes_result_free(struct KittycatPermissionCheckPatchChangesResult *result)
{
    if (result->failing_perms != NULL)
    {
        kittycat_permission_list_free(result->failing_perms);
    }
    __kittycat_free(result);
}

struct kittycat_string *kittycat_permission_check_patch_changes_result_to_str(struct KittycatPermissionCheckPatchChangesResult *result)
{
    if (result->state == KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION)
    {
        struct kittycat_string *perm_str = kittycat_permission_to_str(result->failing_perms->perms[0]);

        size_t error_msg_len = strlen("You do not have permission to add this permission: ") + perm_str->len + 1;
        char *error_msg = __kittycat_malloc(error_msg_len);
        snprintf(
            error_msg,
            error_msg_len,
            "You do not have permission to add this permission: %s",
            perm_str->str);

        kittycat_string_free(perm_str);

        return kittycat_string_new(error_msg, error_msg_len);
    }
    else if (result->state == KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD)
    {
        struct kittycat_string *perm_str = kittycat_permission_to_str(result->failing_perms->perms[0]);
        struct kittycat_string *negator_str = kittycat_permission_to_str(result->failing_perms->perms[1]);

        size_t error_msg_len = strlen("You do not have permission to add wildcard permission ") + perm_str->len + strlen(" with negators due to lack of negator ") + negator_str->len + 1;
        char *error_msg = __kittycat_malloc(error_msg_len);
        snprintf(
            error_msg,
            error_msg_len,
            "You do not have permission to add wildcard permission %s with negators due to lack of negator %s",
            perm_str->str,
            negator_str->str);

        kittycat_string_free(perm_str);
        kittycat_string_free(negator_str);

        return kittycat_string_new(error_msg, error_msg_len);
    }
    else
    {
        return NULL;
    }
}

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

        if (kittycat_string_equal(perm->perm, &__kittycat_perm_global_perm))
        {
            // Ensure that new_perms has *at least* negators that manager_perms has within the namespace
            for (size_t j = 0; j < manager_perms->len; j++)
            {
                struct KittycatPermission *perms = manager_perms->perms[j];
                if (perms->negator)
                {
                    continue;
                }

                if (kittycat_string_equal(perms->namespace, perm->namespace))
                {
                    // Then we have a negator in the same namespace
                    bool in_new_perms = false;
                    for (size_t k = 0; k < new_perms->len; k++)
                    {
                        struct KittycatPermission *new_perm = new_perms->perms[k];
                        if (kittycat_string_equal(new_perm->namespace, perm->namespace) && kittycat_string_equal(new_perm->perm, perm->perm) && new_perm->negator == perm->negator)
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
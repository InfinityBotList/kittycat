#include "kc_string.c"
#include "hashmap/hashmap.h"

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

char *permission_to_str(struct Permission *p)
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

    free(namespace);
    free(perm);

    return finalPerm;
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
        char *perm_chararr = permission_to_str(pl->perms[i]);
        struct string *perm_str = string_from_char(perm_chararr);
        free(perm_chararr);
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

/* Permission resolution */

struct PartialStaffPosition
{
    // The id of the position
    struct string *id;
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
    p->id = string_from_char(id);
    p->index = index;
    p->perms = perms;
    return p;
}

void partial_staff_position_free(struct PartialStaffPosition *p)
{
    string_free(p->id);
    permission_list_free(p->perms);
    free(p);
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
    for (size_t i = 0; i < pl->len; i++)
    {
        partial_staff_position_free(pl->positions[i]);
    }
    free(pl->positions);
    free(pl);
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
    partial_staff_position_list_free(sp->user_positions);
    permission_list_free(sp->perm_overrides);
    free(sp);
}

// Internally used for staff permission resolution
struct __PermissionWithCounts
{
    struct Permission *perm;
    size_t count;
};

struct __PermissionWithCounts *__new_permission_with_counts(struct Permission *perm)
{
    struct __PermissionWithCounts *pwc = malloc(sizeof(struct __PermissionWithCounts));
    pwc->perm = perm;
    pwc->count = 1;
    return pwc;
}

struct __PermissionWithCounts *__permission_with_counts_copy(struct __PermissionWithCounts *pwc)
{
    struct __PermissionWithCounts *new_pwc = malloc(sizeof(struct __PermissionWithCounts));
    new_pwc->perm = pwc->perm;
    new_pwc->count = pwc->count;
    return new_pwc;
}

void __permission_with_counts_free(struct __PermissionWithCounts *pwc)
{
    free(pwc);
}

struct __OrderedPermissionMap
{
    struct hashmap *map;
    struct Permission **order;
    size_t len;
};

uint64_t __permissionwc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct __PermissionWithCounts *p = item;
    char *perm_str = permission_to_str(p->perm);
    uint64_t hash = hashmap_sip(perm_str, strlen(perm_str), seed0, seed1);
    free(perm_str);
    return hash;
}

int __permissionwc_compare(const void *a, const void *b, void *udata)
{
    const struct __PermissionWithCounts *pa = a;
    const struct __PermissionWithCounts *pb = b;

    char *pa_str = permission_to_str(pa->perm);
    char *pb_str = permission_to_str(pb->perm);

    int cmp = strcmp(pa_str, pb_str);

    free(pa_str);
    free(pb_str);

    return cmp;
}

struct __OrderedPermissionMap *__new_ordered_permission_map()
{
    struct __OrderedPermissionMap *opm = malloc(sizeof(struct __OrderedPermissionMap));
    opm->map = hashmap_new(sizeof(struct __PermissionWithCounts), 0, 0, 0, __permissionwc_hash, __permissionwc_compare, NULL, NULL);
    opm->order = malloc(sizeof(struct Permission *));
    return opm;
}

void __ordered_permission_map_free(struct __OrderedPermissionMap *opm)
{
    hashmap_free(opm->map);
    free(opm->order);
    free(opm);
}

void __ordered_permission_map_set(struct __OrderedPermissionMap *opm, struct __PermissionWithCounts *p)
{
    hashmap_set(opm->map, p);
    opm->len = hashmap_count(opm->map);
    opm->order = realloc(opm->order, (opm->len * sizeof(struct Permission *)));
    opm->order[opm->len] = p->perm;
}

struct __PermissionWithCounts *__ordered_permission_map_get(struct __OrderedPermissionMap *opm, struct Permission *perm)
{
    struct __PermissionWithCounts *pwc = hashmap_get(opm->map, perm);
    return pwc == NULL ? NULL : __permission_with_counts_copy(pwc);
}

void __ordered_permission_map_rm(struct __OrderedPermissionMap *opm, struct Permission *perm)
{
    struct __PermissionWithCounts *pwc = hashmap_delete(opm->map, perm);
    __permission_with_counts_free(pwc);
    opm->len = hashmap_count(opm->map);
    opm->order = realloc(opm->order, (opm->len * sizeof(struct Permission *)));
}

void __ordered_permission_map_clear(struct __OrderedPermissionMap *opm)
{
    hashmap_clear(opm->map, false);
    opm->len = 0;
    free(opm->order);
    opm->order = malloc(sizeof(struct Permission *));
}

// NOTE+TODO: This function is not yet implemented fully
struct PermissionList *staff_permissions_resolve(struct StaffPermissions *sp)
{
    struct __OrderedPermissionMap *opm = __new_ordered_permission_map();

    struct PartialStaffPositionList *userPositions = sp->user_positions;

    // Add the permission overrides as index 0
    partial_staff_position_list_add(userPositions, new_partial_staff_position("perm_overrides", 0, sp->perm_overrides));

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

    __ordered_permission_map_free(opm);
}
#include "kc_string.h"

// Represents a permission
struct KittycatPermission
{
    struct kittycat_string *namespace;
    struct kittycat_string *perm;
    bool negator;

    // Internal
    bool __isCloned;
};

// Creates a new KittycatPermission from a string.
// The string must be in the format `namespace.perm`
// Caller must free the KittycatPermission after use using `kittycat_permission_free`
struct KittycatPermission *kittycat_new_permission(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator);

// Same as `kittycat_new_permission` but clones the underlying namespace and perm strings.
// Note that the original namespace+perm will be cloned and automatically freed when the KittycatPermission is freed using `kittycat_permission_free`
struct KittycatPermission *kittycat_new_permission_cloned(struct kittycat_string *namespace, struct kittycat_string *perm, bool negator);

// Creates a new KittycatPermission from the canonical representation of the permission `str`
// Note: Caller must free the KittycatPermission after use using `kittycat_permission_free`
struct KittycatPermission *kittycat_permission_new_from_str(struct kittycat_string *str);

// Converts a permission to its canonical string representation
struct kittycat_string *kittycat_permission_to_str(struct KittycatPermission *p);

// Frees the KittycatPermission
void kittycat_permission_free(struct KittycatPermission *p);

// Represents a list of permissions
struct KittycatPermissionList
{
    struct KittycatPermission **perms;
    size_t len;
};

// Creates a new KittycatPermissionList
struct KittycatPermissionList *kittycat_permission_list_new();

// Adds a permission to the list
void kittycat_permission_list_add(struct KittycatPermissionList *pl, struct KittycatPermission *const perm);

// Removes a permission from the list
void kittycat_permission_list_rm(struct KittycatPermissionList *pl, size_t i);

// Creates a new KittycatPermissionList from a list of permissions
struct KittycatPermissionList *kittycat_permission_list_new_with_perms(
    struct KittycatPermission **perms, size_t len);

// Joins a KittycatPermission list to produce a string
//
// The canonical string representation is used for each individual input KittycatPermission
// The returned string must be freed by the caller
struct kittycat_string *kittycat_permission_list_join(struct KittycatPermissionList *pl, char *sep);

// Returns if two KittycatPermissionLists are equal
bool kittycat_permission_lists_equal(struct KittycatPermissionList *pl1, struct KittycatPermissionList *pl2);

// Frees the KittycatPermissionList
void kittycat_permission_list_free(struct KittycatPermissionList *pl);

// Returns if a user having permission list `perms` has permission `perm`
//
// This is the key primitive within kittycat
bool kittycat_has_perm(const struct KittycatPermissionList *const perms, const struct KittycatPermission *const perm);

// A PartialStaffPosition is a partial representation of a staff position
// for the purposes of permission resolution
struct KittycatPartialStaffPosition
{
    // The id of the position
    struct kittycat_string *id;
    // The index of the KittycatPermission. Lower means higher in the list of hierarchy
    int32_t index;
    // The preset KittycatPermissions of this position
    struct KittycatPermissionList *perms;
};

// Creates a new KittycatPartialStaffPosition given its id, index (lower means higher in hierarchy) and the permission list of the position
struct KittycatPartialStaffPosition *kittycat_partial_staff_position_new(char *id, int32_t index, struct KittycatPermissionList *perms);

// Frees the KittycatPartialStaffPosition
void kittycat_partial_staff_position_free(struct KittycatPartialStaffPosition *p);

// A list of KittycatPartialStaffPosition's
struct KittycatPartialStaffPositionList
{
    struct KittycatPartialStaffPosition **positions;
    size_t len;
};

// Creates a new KittycatPartialStaffPositionList (list of KittycatPartialStaffPosition's)
struct KittycatPartialStaffPositionList *kittycat_partial_staff_position_list_new();

// Adds a KittycatPartialStaffPosition to the list
void kittycat_partial_staff_position_list_add(struct KittycatPartialStaffPositionList *pl, struct KittycatPartialStaffPosition *p);

// Removes a KittycatPartialStaffPosition from the list based on its index `i`
void kittycat_partial_staff_position_list_rm(struct KittycatPartialStaffPositionList *pl, size_t i);

// Frees the KittycatPartialStaffPositionList
void kittycat_partial_staff_position_list_free(struct KittycatPartialStaffPositionList *pl);

// A set of KittycatPermissions for a staff member
//
// This is a list of KittycatPermissions that the user has
struct StaffKittycatPermissions
{
    struct KittycatPartialStaffPositionList *user_positions;
    struct KittycatPermissionList *perm_overrides;
};

// Creates a new StaffKittycatPermissions
//
// Positions and perm overrides can then be added to their corresponding lists in the structure
// using their methods
struct StaffKittycatPermissions *kittycat_staff_permissions_new();

// Free the StaffKittycatPermissions including all user_positions and perm_overrides
void kittycat_staff_permissions_free(struct StaffKittycatPermissions *sp);

// Resolves the KittycatPermissions of a staff member
struct KittycatPermissionList *kittycat_staff_permissions_resolve(const struct StaffKittycatPermissions *const sp);

// Stores the result of `kittycat_permission_check_patch_changes`
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
void kittycat_permission_check_patch_changes_result_free(struct KittycatPermissionCheckPatchChangesResult *result);

// Converts a KittycatPermissionCheckPatchChangesResult to a string
// KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_NO_PERMISSION => You do not have permission to add this permission: {perm[0]}
// KITTYCAT_PERMISSION_CHECK_PATCH_CHANGES_RESULT_STATE_LACKS_NEGATOR_FOR_WILDCARD => You do not have permission to add wildcard permission {perm[0]} with negators due to lack of negator {perm[1]}"
struct kittycat_string *kittycat_permission_check_patch_changes_result_to_str(struct KittycatPermissionCheckPatchChangesResult *result);

// Checks whether or not a resolved set of KittycatPermissions allows the addition or removal of a KittycatPermission to a position
struct KittycatPermissionCheckPatchChangesResult kittycat_check_patch_changes(
    struct KittycatPermissionList *manager_perms,
    struct KittycatPermissionList *current_perms,
    struct KittycatPermissionList *new_perms);
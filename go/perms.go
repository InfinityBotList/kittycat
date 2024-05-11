// Port of https://github.com/InfinityBotList/kittycat/blob/main/src/perms.rs

package perms

// AI-converted from https://www.codeconvert.ai/app with some manual fixes

import (
	"fmt"
	"slices"
	"sort"
	"strings"
	"testing"

	mapset "github.com/deckarep/golang-set/v2"
	orderedmap "github.com/wk8/go-ordered-map/v2"
)

// A permission is defined as the following structure
//
// namespace.permission
//
// If a user has the * permission, they will have all permissions in that namespace
// If namespace is global then only the permission is checked. E.g. global.view allows using the view permission in all namespaces
//
// # If a permission has no namespace, it will be assumed to be global
//
// If a permission has ~ in the beginning of its namespace, it is a negator permission that removes that specific permission from the user
//
// Negators work to negate a specific permission *excluding the global.* permission* (for now, until this gets a bit more refined to not need a global.* special case)
//
// Anything past the <namespace>.<permission> may be ignored or indexed at the discretion of the implementation and is * behaviour*
//
// # Permission Overrides
//
// Permission overrides are a special case of permissions that are used to override permissions for a specific user.
// They use the same structure and follow the same rules as a normal permission, but are stored separately from the normal permissions.
//
// # Special Cases
//
// - Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
//
// # Clearing Permissions
//
// In some cases, it may desired to start from a fresh slate of permissions. To do this, add a '@clear' permission to the namespace. All permissions after this on that namespace will be cleared
//
//
// # Permission Management
//
// A permission can be added/managed by a user to a position if the following applies:
//
// - The user must *have* the permission themselves. If the permission is a negator, then the user must have the 'base' permission (permission without the negator operator)
// - If the permission is `*`, then the user has no negators in that namespace that the target perm set would not also have
//
// Note on point 2: this means that if a user is trying to add/remove rpc.* but also has ~rpc.test, then they cannot add rpc.* unless the target user also has ~rpc.test

//

type PartialStaffPosition struct {
	// The id of the position
	ID string `json:"id"`
	// The index of the permission. Lower means higher in the list of hierarchy
	Index int32 `json:"index"`
	// The preset permissions of this position
	Perms []Permission `json:"perms"`
}

// A set of permissions for a staff member
//
// This is a list of permissions that the user has
type StaffPermissions struct {
	UserPositions []PartialStaffPosition `json:"user_positions"`
	PermOverrides []Permission           `json:"perm_overrides"`
}

// A deconstructed permission
// This allows converting a permission (e.g. rpc.test) into a Permission struct
type Permission struct {
	Namespace string `json:"namespace"`
	Perm      string `json:"perm"`
	Negator   bool   `json:"negator"`
}

func PermissionFromString(perm string) Permission {
	negator := strings.HasPrefix(perm, "~")
	perm = strings.TrimPrefix(perm, "~")
	permSplit := strings.SplitN(perm, ".", 2)

	if len(permSplit) < 2 {
		// Assume global namespace
		permSplit = append([]string{"global"}, permSplit...)
	}

	namespace := permSplit[0]
	permission := permSplit[1]
	return Permission{
		Namespace: namespace,
		Perm:      permission,
		Negator:   negator,
	}
}

// Same as PermissionFromString but shorter function name
func PFS(perm string) Permission {
	return PermissionFromString(perm)
}

func PermissionFromStrings(perms []string) []Permission {
	permsList := []Permission{}
	for _, perm := range perms {
		permsList = append(permsList, PermissionFromString(perm))
	}
	return permsList
}

// Same as PermissionFromStrings but shorter function name
//
// PFSS = PermissionFromStrings
func PFSS(perms []string) []Permission {
	return PermissionFromStrings(perms)
}

func (p Permission) String() string {
	s := strings.Builder{}

	if p.Negator {
		s.Write([]byte("~"))
	}

	s.WriteString(p.Namespace)
	s.Write([]byte("."))
	s.WriteString(p.Perm)

	return s.String()
}

func (s StaffPermissions) Resolve() []Permission {
	appliedPermsVal := orderedmap.New[Permission, int32]()

	// Sort the positions by index in descending order
	userPositions := s.UserPositions
	// Add the permission overrides as index 0
	userPositions = append(userPositions, PartialStaffPosition{
		ID:    "perm_overrides",
		Index: 0,
		Perms: s.PermOverrides,
	})
	sort.Slice(userPositions, func(i, j int) bool {
		return userPositions[i].Index > userPositions[j].Index
	})

	for _, pos := range userPositions {
		for _, perm := range pos.Perms {
			if perm.Perm == "@clear" {
				if perm.Namespace == "global" {
					// Clear all permissions
					appliedPermsVal = orderedmap.New[Permission, int32]()
				} else {
					// Clear all perms with this namespace
					toRemove := []Permission{}
					for key := appliedPermsVal.Oldest(); key != nil; key = key.Next() {
						if key.Key.Namespace == perm.Namespace {
							toRemove = append(toRemove, key.Key)
						}
					}

					for _, key := range toRemove {
						appliedPermsVal.Delete(key)
					}
				}
				continue
			}
			if perm.Negator {
				// Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
				nonNegated := Permission{
					Namespace: perm.Namespace,
					Perm:      perm.Perm,
					Negator:   false,
				}
				if _, ok := appliedPermsVal.Get(nonNegated); ok {
					// Remove old permission
					appliedPermsVal.Delete(nonNegated)
					// Add the negator
					appliedPermsVal.Set(perm, pos.Index)
				} else {
					if _, ok := appliedPermsVal.Get(perm); ok {
						// Case 3: The negator is already applied, so we can ignore it
						continue
					}
					// Then we can freely add the negator
					appliedPermsVal.Set(perm, pos.Index)
				}
			} else {
				// Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
				if perm.Perm == "*" {
					// Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap
					toRemove := []Permission{}
					for key := appliedPermsVal.Oldest(); key != nil; key = key.Next() {
						if !key.Key.Negator {
							continue // This special case only applies to negators
						}
						if key.Key.Namespace == perm.Namespace {
							// Then we can ignore this negator
							toRemove = append(toRemove, key.Key)
						}
					}
					// Remove here to avoid immutable borrow
					for _, key := range toRemove {
						appliedPermsVal.Delete(key)
					}
				}
				// If its not a negator, first check if there's a negator
				negated := Permission{
					Namespace: perm.Namespace,
					Perm:      perm.Perm,
					Negator:   true,
				}
				if _, ok := appliedPermsVal.Get(negated); ok {
					// Remove the negator
					appliedPermsVal.Delete(negated)
					// Add the permission
					appliedPermsVal.Set(perm, pos.Index)
				} else {
					if _, ok := appliedPermsVal.Get(perm); ok {
						// Case 3: The permission is already applied, so we can ignore it
						continue
					}
					// Then we can freely add the permission
					appliedPermsVal.Set(perm, pos.Index)
				}
			}
		}
	}
	appliedPerms := []Permission{}
	for key := appliedPermsVal.Oldest(); key != nil; key = key.Next() {
		appliedPerms = append(appliedPerms, key.Key)
	}

	if testing.Testing() {
		fmt.Println("Applied perms: ", appliedPerms, " with hashmap: ", appliedPermsVal)
	}

	return appliedPerms
}

// Check if the user has a permission given a set of user permissions and a permission to check for
//
// This assumes a resolved set of permissions
func HasPerm(perms []Permission, perm Permission) bool {
	var hasPerm bool
	var hasNegator bool
	for _, userPerm := range perms {
		if !userPerm.Negator && userPerm.Namespace == "global" && userPerm.Perm == "*" {
			// Special case
			return true
		}

		if (userPerm.Namespace == perm.Namespace ||
			userPerm.Namespace == "global") &&
			(userPerm.Perm == "*" || userPerm.Perm == perm.Perm) {
			// We have to check for negator
			hasPerm = true

			if userPerm.Negator {
				hasNegator = true // While we can optimize here by returning false, we may want to add more negation systems in the future
			}
		}
	}
	return hasPerm && !hasNegator
}

func HasPermString(perms []string, perm string) bool {
	return HasPerm(PFSS(perms), PFS(perm))
}

// Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position
func CheckPatchChanges(managerPerms, currentPerms, newPerms []Permission) error {
	// Take the symmetric_difference between currentPerms and newPerms
	hset1 := mapset.NewSet[Permission]()
	for _, perm := range currentPerms {
		hset1.Add(perm)
	}
	hset2 := mapset.NewSet[Permission]()
	for _, perm := range newPerms {
		hset2.Add(perm)
	}
	changed := hset2.SymmetricDifference(hset1).ToSlice()

	for _, perm := range changed {
		resolvedPerm := perm

		if perm.Negator {
			// Strip the namespace to check it
			resolvedPerm.Negator = false
		}

		// Check if the user has the permission
		if !HasPerm(managerPerms, resolvedPerm) {
			return fmt.Errorf("you do not have permission to add this permission: %s", resolvedPerm)
		}

		if perm.Perm == "*" {
			// Ensure that newPerms has *at least* negators that managerPerms has within the namespace
			for _, perms := range managerPerms {
				if !perms.Negator {
					continue // Only check negators
				}

				if perms.Namespace == perm.Namespace {
					fmt.Println("=>", newPerms, perms)
					// Then we have a negator in the same namespace
					if !slices.Contains(newPerms, perms) {
						return fmt.Errorf("you do not have permission to add wildcard permission %s with negators due to lack of negator %s", perm, perms)
					}
				}
			}
		}
	}
	return nil
}

func CheckPatchChangesString(managerPerms, currentPerms, newPerms []string) error {
	return CheckPatchChanges(PFSS(managerPerms), PFSS(currentPerms), PFSS(newPerms))
}

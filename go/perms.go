package perms

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

// permAccumulator tracks the permissions applied so far during Resolve,
// alongside an index of which applied permissions belong to each namespace.
//
// Resolve needs to answer "which currently-applied permissions are in
// namespace X" twice per non-negator permission it processes (for `@clear`
// and for the wildcard-overrides-negator rule). Scanning every applied
// permission to answer that, as the naive implementation did, makes Resolve
// O(n^2) in the total number of permissions across all positions. The index
// here answers it directly, making Resolve O(n). This only changes how that
// question is answered, not the sequence of Set/Delete calls against the
// underlying ordered map, so the resolved output (including its order) is
// unchanged.
type permAccumulator struct {
	perms       *orderedmap.OrderedMap[Permission, int32]
	byNamespace map[string]map[Permission]struct{}
}

func newPermAccumulator() *permAccumulator {
	return &permAccumulator{
		perms:       orderedmap.New[Permission, int32](),
		byNamespace: map[string]map[Permission]struct{}{},
	}
}

func (a *permAccumulator) Get(p Permission) (int32, bool) {
	return a.perms.Get(p)
}

func (a *permAccumulator) Set(p Permission, index int32) {
	if _, existed := a.perms.Get(p); !existed {
		if a.byNamespace[p.Namespace] == nil {
			a.byNamespace[p.Namespace] = map[Permission]struct{}{}
		}
		a.byNamespace[p.Namespace][p] = struct{}{}
	}
	a.perms.Set(p, index)
}

func (a *permAccumulator) Delete(p Permission) {
	delete(a.byNamespace[p.Namespace], p)
	a.perms.Delete(p)
}

func (a *permAccumulator) ClearAll() {
	a.perms = orderedmap.New[Permission, int32]()
	a.byNamespace = map[string]map[Permission]struct{}{}
}

// NamespaceMembers returns the permissions currently applied in namespace.
func (a *permAccumulator) NamespaceMembers(namespace string) []Permission {
	members := a.byNamespace[namespace]
	out := make([]Permission, 0, len(members))
	for p := range members {
		out = append(out, p)
	}
	return out
}

func (a *permAccumulator) Keys() []Permission {
	keys := []Permission{}
	for key := a.perms.Oldest(); key != nil; key = key.Next() {
		keys = append(keys, key.Key)
	}
	return keys
}

func (s StaffPermissions) Resolve() []Permission {
	acc := newPermAccumulator()

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
					acc.ClearAll()
				} else {
					// Clear all perms with this namespace
					for _, key := range acc.NamespaceMembers(perm.Namespace) {
						acc.Delete(key)
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
				if _, ok := acc.Get(nonNegated); ok {
					// Remove old permission
					acc.Delete(nonNegated)
					// Add the negator
					acc.Set(perm, pos.Index)
				} else {
					if _, ok := acc.Get(perm); ok {
						// Case 3: The negator is already applied, so we can ignore it
						continue
					}
					// Then we can freely add the negator
					acc.Set(perm, pos.Index)
				}
			} else {
				// Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
				if perm.Perm == "*" {
					// Remove negators in this namespace. As the permissions
					// are sorted, we can just check if a negator is in the
					// accumulator.
					for _, key := range acc.NamespaceMembers(perm.Namespace) {
						if !key.Negator {
							continue // This special case only applies to negators
						}
						acc.Delete(key)
					}
				}
				// If its not a negator, first check if there's a negator
				negated := Permission{
					Namespace: perm.Namespace,
					Perm:      perm.Perm,
					Negator:   true,
				}
				if _, ok := acc.Get(negated); ok {
					// Remove the negator
					acc.Delete(negated)
					// Add the permission
					acc.Set(perm, pos.Index)
				} else {
					if _, ok := acc.Get(perm); ok {
						// Case 3: The permission is already applied, so we can ignore it
						continue
					}
					// Then we can freely add the permission
					acc.Set(perm, pos.Index)
				}
			}
		}
	}

	appliedPerms := acc.Keys()

	if testing.Testing() {
		fmt.Println("Applied perms: ", appliedPerms)
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
				hasNegator = true
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

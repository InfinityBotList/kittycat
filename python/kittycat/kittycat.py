"""
Python SDK for kittycat
"""
from typing import Union
global _testmode
_testmode = False
def _set_test_mode(b: bool):
    global _testmode
    _testmode = True

class PartialStaffPosition:
    id: str
    """The id of the position"""

    index: int
    """The index of the position. Lower means higher in the list of hierarchy"""

    perms: list["Permission"]
    """The preset permissions of this position"""

    """A PartialStaffPosition is a partial representation of a staff position"""
    def __init__(self, id: str, index: int, perms: list[Union["Permission", str]], trusted: bool = False):
        self.id = id
        """The id of the position"""

        self.index = index
        """The index of the permission. Lower means higher in the list of hierarchy"""

        self.perms = []

        if not trusted:
            for perm in perms:
                if isinstance(perm, str):
                    self.perms.append(Permission.from_str(perm))
                else:
                    self.perms.append(perm)
        else:
            self.perms = perms
    
class Permission:
    """
    A deconstructed permission

    This allows converting a permission (e.g. rpc.test) into a Permission struct
    """
    namespace: str
    perm: str
    negator: bool

    def __init__(self, namespace: str, perm: str, negator: bool):
        self.namespace = namespace
        self.perm = perm
        self.negator = negator
    
    @staticmethod
    def from_str(perm: str):
        negator = perm.startswith('~')
        perm_split = perm.split('.', 2)
        if len(perm_split) < 2:
            perm_split = ["global", perm_split[0]]
        
        namespace = perm_split[0].strip('~')
        perm = perm_split[1]
        return Permission(namespace, perm, negator)
    
    @staticmethod
    def from_str_list(perms: list[str]):
        return [Permission.from_str(perm) for perm in perms]

    def __str__(self):
        return f"{'~' if self.negator else ''}{self.namespace}.{self.perm}"

    def __eq__(self, other):
        if isinstance(other, str):
            return str(self) == other
        elif isinstance(other, Permission):
            return self.namespace == other.namespace and self.perm == other.perm and self.negator == other.negator
        else:
            return False
    
    def __hash__(self):
        return hash(str(self))

class StaffPermissions:
    """
    A set of permissions for a staff member

    This is a list of permissions that the user has
    """
    user_positions: list[PartialStaffPosition]
    perm_overrides: list[Permission]

    def __init__(self, user_positions: list[PartialStaffPosition], perm_overrides: list[Permission]):
        self.user_positions = user_positions
        self.perm_overrides = perm_overrides

    def resolve(self) -> list[Permission]:
        applied_perms_val: dict[Permission, int] = {}
        user_positions = self.user_positions.copy()
        user_positions.append(PartialStaffPosition("perm_overrides", 0, self.perm_overrides.copy()))
        
        # Sort the positions by index in descending order
        user_positions.sort(key=lambda x: x.index, reverse=True)

        for pos in user_positions:
            for perm in pos.perms:
                if perm.perm == "@clear":
                    if perm.namespace == "global":
                        # Clear all perms
                        applied_perms_val.clear()
                    else:
                        # Clear all perms with this namespace
                        to_remove = []
                        for key in applied_perms_val:
                            if key.namespace == perm.namespace:
                                to_remove.append(key)
                        
                        # RUST OPT: Remove here to avoid immutable borrow
                        for key in to_remove:
                            del applied_perms_val[key]
                    continue

                if perm.negator:
                    # Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
                    negated = Permission(perm.namespace, perm.perm, False)
                    if negated in applied_perms_val:
                        # Remove old permission
                        del applied_perms_val[negated]
                        # Add the negator
                        applied_perms_val[perm] = pos.index
                    else:
                        if perm in applied_perms_val:
                            # Case 3: The negator is already applied, so we can ignore it
                            continue
                            
                        # Then we can freely add the negator
                        applied_perms_val[perm] = pos.index
                else:
                    # Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
                    if perm.perm == "*":
                        # If the * element is from a permission of lower index, then we can ignore this negator
                        to_remove: list[Permission] = []
                        for key in applied_perms_val:
                            if not key.negator:
                                continue # This special case only applies to negators

                            # Same namespaces
                            if key.namespace == perm.namespace: 
                                # Then we can ignore this negator
                                to_remove.append(key)

                        # RUST OPT: Remove here to avoid immutable borrow
                        for key in to_remove:
                            del applied_perms_val[key]
                    
                    # If its not a negator, first check if there's a negator
                    negated = Permission(perm.namespace, perm.perm, True)
                    if negated in applied_perms_val:
                        # Remove the negator
                        del applied_perms_val[negated]
                        # Add the permission
                        applied_perms_val[perm] = pos.index
                    else:
                        # Case 3: The permission is already applied, so we can ignore it
                        if perm in applied_perms_val:
                            continue
                        
                        # Then we can freely add the permission
                        applied_perms_val[perm] = pos.index

        applied_perms = list(applied_perms_val.keys())

        if _testmode:
            print(f"Applied perms: {applied_perms} with hashmap: {applied_perms_val}");

        return applied_perms

def has_perm(perms: list[Permission], perm: Permission) -> bool:
    """
    Check if the user has a permission given a set of user permissions and a permission to check for
    
    This assumes a resolved set of permissions
    """

    has_perm = None
    has_negator = False
    for user_perm in perms:
        if not user_perm.negator and user_perm.namespace == "global" and user_perm.perm == "*":
            # Special case
            return True
        
        if (user_perm.namespace == perm.namespace or user_perm.namespace == "global") and (user_perm.perm == "*" or user_perm.perm == perm.perm):
            has_perm = user_perm
            if user_perm.negator:
                has_negator = True # While we can optimize here by returning false, we may want to add more negation systems in the future

    return has_perm is not None and not has_negator

def has_perm_str(perms: list[str], perm: str) -> bool:
    return has_perm(Permission.from_str_list(perms), Permission.from_str(perm))

# Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position
def check_patch_changes(manager_perms: list[Permission], current_perms: list[Permission], new_perms: list[Permission]) -> None:
    """Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position"""

    # Take the symmetric_difference between current_perms and new_perms
    hset_1 = set(current_perms)
    hset_2 = set(new_perms)
    changed = list(hset_2.symmetric_difference(hset_1))
    for perm in changed:
        resolved_perm = perm

        if perm.negator:
            # Strip the ~ from namespace to check it
            resolved_perm.negator = False
        
        if not has_perm(manager_perms, resolved_perm):
            # Check if the user has the permission
            raise Exception(f"You do not have permission to add this permission: {resolved_perm}")
        
        if perm.perm == "*":            
            # Ensure that new_perms has *at least* negators that manager_perms has within the namespace
            for perms in manager_perms:
                if not perms.negator:
                    continue # Only check negators
                                
                if perms.namespace == perm.namespace and perms not in new_perms:
                    raise Exception(f"You do not have permission to add wildcard permission {perm} with negators due to lack of negator {perms}")
                
def check_patch_changes_str(manager_perms: list[str], current_perms: list[str], new_perms: list[str]) -> None:
    """Helper method to check_patch_changes with string lists"""
    return check_patch_changes(Permission.from_str_list(manager_perms), Permission.from_str_list(current_perms), Permission.from_str_list(new_perms))
use std::hash::{Hash, Hasher};

use indexmap::IndexMap;

/// A permission is defined as the following structure
///
/// namespace.permission
///
/// If a user has the * permission, they will have all permissions in that namespace
/// If namespace is global then only the permission is checked. E.g. global.view allows using the view permission in all namespaces
///
/// If a permission has no namespace, it will be assumed to be global
///
/// If a permission has ~ in the beginning of its namespace, it is a negator permission that removes that specific permission from the user
///
/// Negators work to negate a specific permission *excluding the global.* permission* (for now, until this gets a bit more refined to not need a global.* special case)
///
/// Anything past the <namespace>.<permission> may be ignored or indexed at the discretion of the implementation and is *undefined behaviour*
///
/// # Permission Overrides
/// 
/// Permission overrides are a special case of permissions that are used to override permissions for a specific user. 
/// They use the same structure and follow the same rules as a normal permission, but are stored separately from the normal permissions.
///
/// # Special Cases
/// 
/// - Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
/// 
/// # Clearing Permissions
/// 
/// In some cases, it may desired to start from a fresh slate of permissions. To do this, add a '@clear' permission to the namespace. All permissions after this on that namespace will be cleared
/// 
/// TODO: Use enums for storing permissions instead of strings by serializing and deserializing them to strings when needed

//

#[derive(Clone, Debug)]
/// A PartialStaffPosition is a partial representation of a staff position
/// for the purposes of permission resolution
pub struct PartialStaffPosition {
    /// The id of the position
    pub id: String,
    /// The index of the permission. Lower means higher in the list of hierarchy
    pub index: i32,
    /// The preset permissions of this position
    pub perms: Vec<String>,
}

impl Hash for PartialStaffPosition {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.id.hash(state);
    }
}

/// A set of permissions for a staff member
/// 
/// This is a list of permissions that the user has
pub struct StaffPermissions {
    pub user_positions: Vec<PartialStaffPosition>,
    pub perm_overrides: Vec<String>,
}

impl StaffPermissions {
    pub fn resolve(&self) -> Vec<String> {
        let mut applied_perms_val = IndexMap::new();

        // Add the permission overrides as index 0
        for perm in self.perm_overrides.iter() {
            applied_perms_val.insert(perm.clone(), 0);
        }

        // Sort the positions by index in descending order
        let mut user_positions = self.user_positions.clone();

        user_positions.sort_by(|a, b| b.index.cmp(&a.index));

        for pos in user_positions.iter() {
            for perm in pos.perms.iter() {
                if perm.ends_with(".@clear") {
                    // Split permission by namespace
                    let mut perm_split = perm.split('.').collect::<Vec<&str>>();

                    if perm_split.len() < 2 {
                        // Then assume its a global permission on the namespace
                        perm_split = vec!["global", "@clear"];
                    }

                    let perm_namespace = perm_split[0];

                    if perm_namespace == "global" {
                        // Clear all perms
                        applied_perms_val.clear();
                        continue;
                    }

                    // Clear all perms with this namespace
                    let mut to_remove = Vec::new();
                    for (key, _) in applied_perms_val.iter() {
                        let mut key_split = key.split('.').collect::<Vec<&str>>();

                        if key_split.len() < 2 {
                            // Then assume its a global permission on the namespace
                            key_split = vec!["global", "*"];
                        }

                        let key_namespace = key_split[0];

                        if key_namespace == perm_namespace {
                            to_remove.push(key.clone());
                        }
                    }

                    // Remove here to avoid immutable borrow
                    for key in to_remove {
                        applied_perms_val.remove(&key);
                    }

                    continue;
                }

                if perm.starts_with('~') {
                    // Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
                    if applied_perms_val.get(perm.trim_start_matches('~')).is_some() {
                        // Remove old permission
                        applied_perms_val.remove(perm.trim_start_matches('~'));

                        // Add the negator
                        applied_perms_val.insert(perm.clone(), pos.index);
                    } else {
                        if applied_perms_val.get(perm).is_some() {
                            // Case 3: The negator is already applied, so we can ignore it
                            continue;
                        }

                        // Then we can freely add the negator
                        applied_perms_val.insert(perm.clone(), pos.index);            
                    }
                } else {
                    // Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
                    if perm.ends_with(".*") {
                        // Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap
                        let perm_split = perm.split('.').collect::<Vec<&str>>();
                        let perm_namespace = perm_split[0];

                        // If the * element is from a permission of lower index, then we can ignore this negator
                        let mut to_remove = Vec::new();
                        for (key, _) in applied_perms_val.iter() {
                            if !key.starts_with('~') {
                                continue; // This special case only applies to negators
                            }

                            let key_namespace = key.split('.').collect::<Vec<&str>>()[0].trim_start_matches('~');
                            // Same namespaces
                            if key_namespace == perm_namespace {
                                // Then we can ignore this negator
                                to_remove.push(key.clone());
                            }
                        }

                        // Remove here to avoid immutable borrow
                        for key in to_remove {
                            applied_perms_val.remove(&key);
                        }
                    }

                    // If its not a negator, first check if there's a negator
                    if applied_perms_val.get(&format!("~{}", perm)).is_some() {
                        // Remove the negator
                        applied_perms_val.remove(&format!("~{}", perm));
                        // Add the permission
                        applied_perms_val.insert(perm.clone(), pos.index);
                    } else {
                        if applied_perms_val.get(perm).is_some() {
                            // Case 3: The permission is already applied, so we can ignore it
                            continue;
                        }

                        // Then we can freely add the permission
                        applied_perms_val.insert(perm.clone(), pos.index);            
                    }
                }
            }
        }
        
        let applied_perms = applied_perms_val.keys().cloned().collect::<Vec<String>>();

        if cfg!(test) {
            println!("Applied perms: {:?} with hashmap: {:?}", applied_perms, applied_perms_val);
        }

        applied_perms
    }
}

/// Check if the user has a permission given a set of user permissions and a permission to check for
/// 
/// This assumes a resolved set of permissions
pub fn has_perm(perms: &Vec<String>, perm: &str) -> bool {
    let mut perm_split = perm.split('.').collect::<Vec<&str>>();

    if perm_split.len() < 2 {
        // Then assume its a global permission on the namespace
        perm_split = vec![perm, "*"];
    }

    let perm_namespace = perm_split[0];
    let perm_name = perm_split[1];

    let mut has_perm = None;
    let mut has_negator = false;
    for user_perm in perms {
        if user_perm == "global.*" {
            // Special case
            return true;
        }

        let mut user_perm_split = user_perm.split('.').collect::<Vec<&str>>();

        if user_perm_split.len() < 2 {
            // Then assume its a global permission
            user_perm_split = vec![user_perm, "*"];
        }

        let mut user_perm_namespace = user_perm_split[0];
        let user_perm_name = user_perm_split[1];

        if user_perm.starts_with('~') {
            // Strip the ~ from namespace to check it
            user_perm_namespace = user_perm_namespace.trim_start_matches('~');
        }

        if (user_perm_namespace == perm_namespace
            || user_perm_namespace == "global"
            || perm_namespace == "global")
            && (user_perm_name == "*" || user_perm_name == perm_name)
        {
            // We have to check for all negator
            has_perm = Some(user_perm_split);

            if user_perm.starts_with('~') {
                has_negator = true; // While we can optimize here by returning false, we may want to add more negation systems in the future
            }
        }
    }

    has_perm.is_some() && !has_negator
}

/// Builds a permission string from a namespace and permission
pub fn build(namespace: &str, perm: &str) -> String {
    format!("{}.{}", namespace, perm)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_has_perm() {
        assert!(has_perm(&vec!["global.*".to_string()], "test")); // global.* implies test[.*]
        assert!(has_perm(&vec!["global.test".to_string()], "rpc.test")); // global.test implies rpc.test
        assert!(!has_perm(
            &vec!["global.test".to_string()],
            "rpc.view_bot_queue"
        )); // global.test does not imply rpc.view_bot_queue as global = rpc, test != view_bot_queue
        assert!(has_perm(
            &vec!["global.*".to_string()],
            "rpc.view_bot_queue"
        )); // global.* implies rpc.view_bot_queue
        assert!(has_perm(&vec!["rpc.*".to_string()], "rpc.ViewBotQueue")); // rpc.* implies rpc.view_bot_queue
        assert!(!has_perm(
            &vec!["rpc.BotClaim".to_string()],
            "rpc.ViewBotQueue"
        )); // rpc.BotClaim does not implies rpc.ViewBotQueue as BotClaim != ViewBotQueue
        assert!(!has_perm(&vec!["apps.*".to_string()], "rpc.ViewBotQueue")); // apps.* does not imply rpc.ViewBotQueue, apps != rpc
        assert!(!has_perm(&vec!["apps.*".to_string()], "rpc.*")); // apps.* does not imply rpc.*, apps != rpc despite the global permission
        assert!(!has_perm(&vec!["apps.test".to_string()], "rpc.test")); // apps.test does not imply rpc.test, apps != rpc despite the permissions being the same

        // Negator tests
        assert!(has_perm(&vec!["apps.*".to_string()], "apps.test")); // apps.* implies apps.test
        assert!(!has_perm(&vec!["~apps.*".to_string()], "apps.test")); // ~apps.* does not imply apps.test as it is negated
        assert!(!has_perm(
            &vec!["apps.*".to_string(), "~apps.test".to_string()],
            "apps.test"
        )); // apps.* does not imply apps.test due to negator ~apps.test
        assert!(!has_perm(
            &vec!["~apps.test".to_string(), "apps.*".to_string()],
            "apps.test"
        )); // apps.* does not imply apps.test due to negator ~apps.test. Same as above with different order of perms to test for ordering
        assert!(has_perm(&vec!["apps.test".to_string()], "apps.test")); // ~apps.* does not imply apps.test as it is negated
        assert!(has_perm(
            &vec!["apps.test".to_string(), "apps.*".to_string()],
            "apps.test"
        )); // More tests
        assert!(has_perm(
            &vec!["~apps.test".to_string(), "global.*".to_string()],
            "apps.test"
        )); // Test for global.* handling as a wildcard 'return true'
    }

    #[test]
    fn test_resolve_perms() {
        // Test for basic resolution of overrides
        assert!(
            StaffPermissions {
                user_positions: Vec::new(),
                perm_overrides: vec!["rpc.test".to_string()]
            }
            .resolve()
            == vec!["rpc.test".to_string()]
        );

        // Test for basic resolution of single position
        assert!(
            StaffPermissions {
                user_positions: vec![(
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.test".to_string()]
                    }
                )],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["rpc.test".to_string()]
        );

        // Test for basic resolution of multiple positions
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.test".to_string()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["rpc.test2".to_string()]
                    }
                ],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["rpc.test2".to_string(), "rpc.test".to_string()]
        );

        // Test for basic resolution of multiple positions with negators
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.test".to_string(), "rpc.test2".to_string()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test".to_string(), "~rpc.test3".to_string()]
                    }
                ],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["~rpc.test3".to_string(), "rpc.test".to_string(), "rpc.test2".to_string()]
        );

        // Same as above but testing negator ordering
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".to_string(), "rpc.test2".to_string()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".to_string(), "rpc.test".to_string()] // The rpc.test here should not be set in final output due to negator in index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["~rpc.test3".to_string(), "~rpc.test".to_string(), "rpc.test2".to_string()]
        );

        // Now mix everything together
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".to_string(), "rpc.test2".to_string(), "rpc.test3".to_string()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".to_string(), "~rpc.test2".to_string()] // The rpc.test2 negator is overriden by index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec!["rpc.test".to_string()]
            }
            .resolve()
            == vec!["~rpc.test".to_string(), "rpc.test2".to_string(), "rpc.test3".to_string()]
        );

        // @clear
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".to_string(), "rpc.test2".to_string()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["global.@clear".to_string(), "~rpc.test".to_string(), "rpc.test2".to_string()] // global.@clear should clear all permissions before it
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".to_string(), "~rpc.test2".to_string()] // The rpc.test2 negator is overriden by index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec!["~rpc.test".to_string(), "~rpc.test3".to_string(), "rpc.test2".to_string()]
            }
            .resolve()
            == vec!["~rpc.test".to_string(), "rpc.test2".to_string()]
        );

        // Special case of * with negators
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.*".to_string()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".to_string(), "~rpc.test2".to_string()] // The rpc negators should be overriden by index 1 rpc.*
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["rpc.*".to_string()]
        );

        // Ensure special case does not apply when index is higher (2 > 1 in the below)
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test3".to_string(), "~rpc.test2".to_string()] // The rpc negators should be overriden by index 1 rpc.*
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 2,
                        perms: vec!["rpc.*".to_string()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                ],
                perm_overrides: vec![]
            }
            .resolve()
            == vec!["rpc.*".to_string(), "~rpc.test3".to_string(), "~rpc.test2".to_string()]
        );
    }
}

use indexmap::IndexMap;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Hash, PartialEq, Eq, Serialize, Deserialize)]
/// A PartialStaffPosition is a partial representation of a staff position
/// for the purposes of permission resolution
pub struct PartialStaffPosition {
    /// The id of the position
    pub id: String,
    /// The index of the permission. Lower means higher in the list of hierarchy
    pub index: i32,
    /// The preset permissions of this position
    pub perms: Vec<Permission>,
}

/// A set of permissions for a staff member
///
/// This is a list of permissions that the user has
#[derive(Clone, Serialize, Deserialize)]
pub struct StaffPermissions {
    pub user_positions: Vec<PartialStaffPosition>,
    pub perm_overrides: Vec<Permission>,
}

/// A deconstructed permission
/// This allows converting a permission (e.g. rpc.test) into a Permission struct
#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Permission {
    pub namespace: String,
    pub perm: String,
    pub negator: bool,
}

impl Permission {
    pub fn from_string(perm: &str) -> Self {
        let mut perm_split = perm.splitn(2, '.').collect::<Vec<&str>>();

        if perm_split.len() < 2 {
            // Then assume its a global permission on the namespace
            perm_split = vec!["global", perm];
        }

        let negator = perm.starts_with('~');
        let perm_namespace = perm_split[0].trim_start_matches('~');
        let perm_name = perm_split[1];

        Permission {
            namespace: perm_namespace.to_string(),
            perm: perm_name.to_string(),
            negator,
        }
    }
}

impl std::fmt::Display for Permission {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.negator {
            write!(f, "~")?;
        }

        write!(f, "{}.{}", self.namespace, self.perm)
    }
}

impl From<&str> for Permission {
    fn from(perm: &str) -> Self {
        Permission::from_string(perm)
    }
}

impl From<String> for Permission {
    fn from(perm: String) -> Self {
        Permission::from_string(&perm)
    }
}

impl From<&String> for Permission {
    fn from(perm: &String) -> Self {
        Permission::from_string(perm)
    }
}

impl PartialEq<String> for Permission {
    fn eq(&self, other: &String) -> bool {
        self == &Permission::from_string(other)
    }
}

impl StaffPermissions {
    pub fn resolve(&self) -> Vec<Permission> {
        let mut applied_perms_val: IndexMap<Permission, i32> = IndexMap::new();

        // Sort the positions by index in descending order
        let mut user_positions = self.user_positions.clone();

        // Add the permission overrides as index 0
        user_positions.push(PartialStaffPosition {
            id: "perm_overrides".to_string(),
            index: 0,
            perms: self.perm_overrides.clone(),
        });

        user_positions.sort_by(|a, b| b.index.cmp(&a.index));

        for pos in user_positions.iter() {
            for perm in pos.perms.iter() {
                if perm.perm == "@clear" {
                    if perm.namespace == "global" {
                        // Clear all permissions
                        applied_perms_val.clear();
                    } else {
                        // Clear all perms with this namespace
                        let mut to_remove = Vec::new();
                        for (key, _) in applied_perms_val.iter() {
                            if key.namespace == perm.namespace {
                                to_remove.push(key.clone());
                            }
                        }

                        // Remove here to avoid immutable borrow
                        for key in to_remove {
                            applied_perms_val.remove(&key);
                        }
                    }
                    continue;
                }

                if perm.negator {
                    // Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
                    let non_negated = Permission {
                        negator: false,
                        ..perm.clone()
                    };

                    if applied_perms_val.get(&non_negated).is_some() {
                        // Remove old permission
                        applied_perms_val.remove(&non_negated);

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
                    if perm.perm == "*" {
                        // Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap

                        // If the * element is from a permission of lower index, then we can ignore this negator
                        let mut to_remove = Vec::new();
                        for (key, _) in applied_perms_val.iter() {
                            if !key.negator {
                                continue; // This special case only applies to negators
                            }

                            // Same namespaces
                            if key.namespace == perm.namespace {
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
                    let negated = Permission {
                        negator: true,
                        ..perm.clone()
                    };

                    if applied_perms_val.get(&negated).is_some() {
                        // Remove the negator
                        applied_perms_val.remove(&negated);
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

        let applied_perms = applied_perms_val
            .keys()
            .cloned()
            .collect::<Vec<Permission>>();

        if cfg!(test) {
            println!(
                "Applied perms: {} with hashmap: {:?}",
                applied_perms
                    .iter()
                    .map(|x| x.to_string())
                    .collect::<Vec<String>>()
                    .join(", "),
                applied_perms_val
            );
        }

        applied_perms
    }
}

/// Check if the user has a permission given a set of user permissions and a permission to check for
///
/// This assumes a resolved set of permissions
pub fn has_perm(perms: &[Permission], perm: &Permission) -> bool {
    let mut has_perm = None;
    let mut has_negator = false;
    for user_perm in perms {
        if !user_perm.negator && user_perm.namespace == "global" && user_perm.perm == "*" {
            // Special case
            return true;
        }

        if (user_perm.namespace == perm.namespace || user_perm.namespace == "global")
            && (user_perm.perm == "*" || user_perm.perm == perm.perm)
        {
            // We have to check for all negator
            has_perm = Some(user_perm);

            if user_perm.negator {
                has_negator = true;
            }
        }
    }

    has_perm.is_some() && !has_negator
}

/// Same thing as has_perm, but with strings
pub fn has_perm_str(perms: &[String], perm: &str) -> bool {
    let perm = Permission::from_string(perm);
    let perms = perms
        .iter()
        .map(|x| Permission::from_string(x))
        .collect::<Vec<Permission>>();
    has_perm(&perms, &perm)
}

/// Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position
pub fn check_patch_changes(
    manager_perms: &[Permission],
    current_perms: &[Permission],
    new_perms: &[Permission],
) -> Result<(), crate::Error> {
    // Take the symmetric_difference between current_perms and new_perms
    let hset_1 = current_perms
        .iter()
        .collect::<std::collections::HashSet<&Permission>>();
    let hset_2 = new_perms
        .iter()
        .collect::<std::collections::HashSet<&Permission>>();
    let changed = hset_2
        .symmetric_difference(&hset_1)
        .cloned()
        .collect::<Vec<&Permission>>();
    for perm in changed {
        let mut resolved_perm = perm.clone();

        if perm.negator {
            // Strip the namespace to check it
            resolved_perm.negator = false;
        }

        // Check if the user has the permission
        if !has_perm(manager_perms, &resolved_perm) {
            return Err(format!(
                "You do not have permission to add this permission: {}",
                resolved_perm
            )
            .into());
        }

        if perm.perm == "*" {
            // Ensure that new_perms has *at least* negators that manager_perms has within the namespace
            for perms in manager_perms {
                if !perms.negator {
                    continue; // Only check negators
                }

                if perms.namespace == perm.namespace {
                    // Then we have a negator in the same namespace
                    if !new_perms.contains(perms) {
                        return Err(format!("You do not have permission to add wildcard permission {} with negators due to lack of negator {}", perm, perms).into());
                    }
                }
            }
        }
    }

    Ok(())
}

pub fn check_patch_changes_str(
    manager_perms: &[String],
    current_perms: &[String],
    new_perms: &[String],
) -> Result<(), crate::Error> {
    let manager_perms = manager_perms
        .iter()
        .map(|x| Permission::from_string(x))
        .collect::<Vec<Permission>>();
    let current_perms = current_perms
        .iter()
        .map(|x| Permission::from_string(x))
        .collect::<Vec<Permission>>();
    let new_perms = new_perms
        .iter()
        .map(|x| Permission::from_string(x))
        .collect::<Vec<Permission>>();
    check_patch_changes(&manager_perms, &current_perms, &new_perms)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_permission_ops() {
        let mut val: IndexMap<Permission, i32> = IndexMap::new();

        // Add the permission overrides as index 0
        val.insert("rpc.test".into(), 1);
        val.insert("~rpc.test".into(), 1);

        assert!(val.get(&Permission::from_string("rpc.test")).is_some());
        assert!(val.get(&Permission::from_string("rpc.test2")).is_none());

        val.remove(&Permission::from_string("rpc.test"));

        assert!(val.get(&Permission::from_string("rpc.test")).is_none());
        assert!(val.get(&Permission::from_string("~rpc.test")).is_some());

        let p = Permission::from_string("~rpc.testC");

        assert!(p.negator);
        assert!(p.namespace == "rpc");
        assert!(p.perm == "testC");
    }

    #[test]
    fn test_has_perm() {
        assert!(has_perm_str(&["global.*".to_string()], "test")); // global.* implies test[.*]
        assert!(!has_perm_str(&["rpc.*".to_string()], "global.*")); // rpc.* does not imply global.* as rpc != global
        assert!(has_perm_str(&["global.test".to_string()], "rpc.test")); // global.test implies rpc.test
        assert!(!has_perm_str(
            &["global.test".to_string()],
            "rpc.view_bot_queue"
        )); // global.test does not imply rpc.view_bot_queue as global = rpc, test != view_bot_queue
        assert!(has_perm_str(
            &["global.*".to_string()],
            "rpc.view_bot_queue"
        )); // global.* implies rpc.view_bot_queue
        assert!(has_perm_str(&["rpc.*".to_string()], "rpc.ViewBotQueue")); // rpc.* implies rpc.view_bot_queue
        assert!(!has_perm_str(
            &["rpc.BotClaim".to_string()],
            "rpc.ViewBotQueue"
        )); // rpc.BotClaim does not implies rpc.ViewBotQueue as BotClaim != ViewBotQueue
        assert!(!has_perm_str(&["apps.*".to_string()], "rpc.ViewBotQueue")); // apps.* does not imply rpc.ViewBotQueue, apps != rpc
        assert!(!has_perm_str(&["apps.*".to_string()], "rpc.*")); // apps.* does not imply rpc.*, apps != rpc despite the global permission
        assert!(!has_perm_str(&["apps.test".to_string()], "rpc.test")); // apps.test does not imply rpc.test, apps != rpc despite the permissions being the same

        // Negator tests
        assert!(has_perm_str(&["apps.*".to_string()], "apps.test")); // apps.* implies apps.test
        assert!(!has_perm_str(&["~apps.*".to_string()], "apps.test")); // ~apps.* does not imply apps.test as it is negated
        assert!(!has_perm_str(
            &["apps.*".to_string(), "~apps.test".to_string()],
            "apps.test"
        )); // apps.* does not imply apps.test due to negator ~apps.test
        assert!(!has_perm_str(
            &["~apps.test".to_string(), "apps.*".to_string()],
            "apps.test"
        )); // apps.* does not imply apps.test due to negator ~apps.test. Same as above with different order of perms to test for ordering
        assert!(has_perm_str(&["apps.test".to_string()], "apps.test")); // ~apps.* does not imply apps.test as it is negated
        assert!(has_perm_str(
            &["apps.test".to_string(), "apps.*".to_string()],
            "apps.test"
        )); // More tests
        assert!(has_perm_str(
            &["~apps.test".to_string(), "global.*".to_string()],
            "apps.test"
        )); // Test for global.* handling as a wildcard 'return true'
    }

    #[test]
    fn test_resolve_perms() {
        // Test for basic resolution of overrides
        assert!(
            StaffPermissions {
                user_positions: Vec::new(),
                perm_overrides: vec!["rpc.test".into()]
            }
            .resolve()
                == vec!["rpc.test".to_string()]
        );

        // Test for basic resolution of single position
        assert!(
            StaffPermissions {
                user_positions: vec![
                    (PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.test".into()]
                    })
                ],
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
                        perms: vec!["rpc.test".into()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["rpc.test2".into()]
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
                        perms: vec!["rpc.test".into(), "rpc.test2".into()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test".into(), "~rpc.test3".into()]
                    }
                ],
                perm_overrides: vec![]
            }
            .resolve()
                == vec![
                    "~rpc.test3".to_string(),
                    "rpc.test".to_string(),
                    "rpc.test2".to_string()
                ]
        );

        // Same as above but testing negator ordering
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".into(), "rpc.test2".into()]
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".into(), "rpc.test".into()] // The rpc.test here should not be set in final output due to negator in index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec![]
            }
            .resolve()
                == vec![
                    "~rpc.test3".to_string(),
                    "~rpc.test".to_string(),
                    "rpc.test2".to_string()
                ]
        );

        // Now mix everything together
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".into(), "rpc.test2".into(), "rpc.test3".into()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test2".into(),
                        index: 2,
                        perms: vec!["~rpc.test3".into(), "~rpc.test2".into()] // The rpc.test2 negator is overriden by index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec!["rpc.test".into()]
            }
            .resolve()
                == vec![
                    "rpc.test2".to_string(),
                    "rpc.test3".to_string(),
                    "rpc.test".to_string(),
                ]
        );

        // @clear
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["~rpc.test".into(), "rpc.test2".into()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec![
                            "global.@clear".into(),
                            "~rpc.test".into(),
                            "rpc.test2".into()
                        ] // global.@clear should clear all permissions before it
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".into(), "~rpc.test2".into()] // The rpc.test2 negator is overriden by index 1
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec!["~rpc.test".into(), "rpc.test2".into(), "rpc.test3".into()]
            }
            .resolve()
                == vec![
                    "~rpc.test".to_string(),
                    "rpc.test2".to_string(),
                    "rpc.test3".to_string()
                ]
        );

        // Special case of * with negators
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 1,
                        perms: vec!["rpc.*".into()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                    PartialStaffPosition {
                        id: "test2".to_string(),
                        index: 2,
                        perms: vec!["~rpc.test3".into(), "~rpc.test2".into()] // The rpc negators should be overriden by index 1 rpc.*
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
                        perms: vec!["~rpc.test3".into(), "~rpc.test2".into()] // The rpc negators should be overriden by index 1 rpc.*
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                    PartialStaffPosition {
                        id: "test".to_string(),
                        index: 2,
                        perms: vec!["rpc.*".into()] // The rpc.test here is overriden by index 0 (perm_overrides)
                    },
                ],
                perm_overrides: vec![]
            }
            .resolve()
                == vec![
                    "rpc.*".to_string(),
                    "~rpc.test3".to_string(),
                    "~rpc.test2".to_string()
                ]
        );

        // Some common cases
        // Ensure special case does not apply when index is higher (2 > 1 in the below)
        assert!(
            StaffPermissions {
                user_positions: vec![
                    PartialStaffPosition {
                        id: "reviewer".to_string(),
                        index: 1,
                        perms: vec!["rpc.Claim".into()] // The rpc negators should be overriden by index 1 rpc.*
                    }, // Note that indexing here works in reverse, so the higher index is actually lower in the hierarchy
                ],
                perm_overrides: vec!["~rpc.Claim".into()]
            }
            .resolve()
                == vec!["~rpc.Claim".to_string()]
        );
    }

    #[test]
    fn test_check_patch_changes() {
        // global.* should always be allowed to change perms
        assert!(check_patch_changes(
            &["global.*".into()],
            &["rpc.test".into()],
            &["rpc.test".into(), "rpc.test2".into()]
        )
        .is_ok());

        // Users should not be able to remove perms they dont have
        assert!(check_patch_changes(
            &["rpc.*".into()],
            &["global.*".into()],
            &["rpc.test".into(), "rpc.test2".into()]
        )
        .is_err());

        // Basic real world test
        assert!(check_patch_changes(
            &["rpc.*".into()],
            &["rpc.test".into()],
            &["rpc.test".into(), "rpc.test2".into()]
        )
        .is_ok());

        // If adding '*' permissions, target must have all negators of user
        assert!(check_patch_changes(
            &["~rpc.test".into(), "rpc.*".into()],
            &["rpc.foobar".into()],
            &["rpc.*".into()]
        )
        .is_err());
        assert!(check_patch_changes(
            &["~rpc.test".into(), "rpc.*".into()],
            &["~rpc.test".into()],
            &["rpc.*".into()]
        )
        .is_err());

        // This is OK though
        assert!(check_patch_changes(
            &["~rpc.test".into(), "rpc.*".into()],
            &["~rpc.test".into()],
            &["rpc.*".into(), "~rpc.test".into(), "~rpc.test2".into()]
        )
        .is_ok());

        // But this isnt as rpc.test negator isnt in new
        assert!(check_patch_changes(
            &["~rpc.test".into(), "rpc.*".into()],
            &["~rpc.test".into()],
            &["rpc.*".into(), "~rpc.test2".into(), "~rpc.test2".into()]
        )
        .is_err());
    }
}

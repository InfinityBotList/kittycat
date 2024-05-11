// Test related
export let testMode: boolean = false;
export function setTestMode(f: boolean) {
	testMode = f;
}

// A deconstructed permission
// This allows converting a permission (e.g. rpc.test) into a Permission
export class Permission {
	public namespace: string;
	public perm: string;
	public negator: boolean;

    constructor(namespace: string, perm: string, negator: boolean) {
        this.namespace = namespace;
        this.perm = perm;
        this.negator = negator;
    }

	static from(perm: string): Permission {
		let perm_split = perm.split(".", 2);
		if (perm_split.length < 2) {
			// Then assume its a global permission on the namespace
			perm_split = ["global", perm];
		}

        let negator = perm.startsWith("~")
        let namespace = perm_split[0].slice(negator ? 1 : 0)
        let permission = perm_split[1]

        return new Permission(namespace, permission, negator);
	}

    static fromList(perm_list: string[]): Permission[] {
        let perms: Permission[] = []
        for (let perm of perm_list) {
            perms.push(Permission.from(perm))
        }
        return perms
    }

	toString(): string {
		return `${this.negator ? "~" : ""}${this.namespace}.${this.perm}`;
	}

	toObject() {
		return {
			namespace: this.namespace,
			perm: this.perm,
			negator: this.negator,
		};
	}
}

export class PartialStaffPosition {
	id: string;
	index: number;
	perms: Permission[];

	constructor(id: string, index: number, perms: Permission[]) {
		this.id = id;
		this.index = index;
		this.perms = perms;

		if (!Number.isInteger(index)) {
			throw new Error("Index must be an integer");
		}
	}
}

// JS Maps use reference equality
//
// To avoid issues from it, define a custom map that stores the permissions internally as a string
class PermissionMap {
    private map: Map<string, number>;

    constructor() {
        this.map = new Map();
    }

    get(perm: Permission) {
        return this.map.get(perm.toString());
    }

    set(perm: Permission, index: number) {
        this.map.set(perm.toString(), index);
    }

    delete(perm: Permission) {
        this.map.delete(perm.toString());
    }

    has(perm: Permission) {
        return this.map.has(perm.toString());
    }

    keys() {
        let keys: Permission[] = [];
        for (const key of this.map.keys()) {
            keys.push(Permission.from(key));
        }
        return keys;
    }

    clear() {
        this.map.clear();
    }
    
    // Iterator using *[Symbol.iterator]()
    *[Symbol.iterator](): IterableIterator<[Permission, number]> {
        for (const [key, value] of this.map) {
            yield [Permission.from(key), value];
        }
    }

    toString() {
        let a = []
        for (const [key, value] of this) {
            a.push(`${key} -> ${value}`)
        }
        return a.join(", ")
    }
}

/**
 * A class that represents the permissions of a user
 */
export class StaffPermissions {
	user_positions: PartialStaffPosition[];
	perm_overrides: Permission[];

	constructor(
		user_positions: PartialStaffPosition[],
		perm_overrides: Permission[],
	) {
		this.user_positions = user_positions;
		this.perm_overrides = perm_overrides;
	}

    /**
     * Resolves the permissions of the user given their positions and permission overrides
     * 
     * @returns The resolved permissions of the user
     */
	resolve(): Permission[] {
		const applied_perms_val = new PermissionMap();
		let user_positions = [...this.user_positions];

		// Add the permission overrides as index 0
		user_positions.push({
			id: "perm_overrides",
			index: 0,
			perms: [...this.perm_overrides],
		});

		// Sort the positions by index in descending order
		user_positions.sort((a, b) => b.index - a.index);
		for (const pos of user_positions) {
			for (const perm of pos.perms) {
				if (perm.perm == "@clear") {
                    if(perm.namespace == "global") {
                        // Clear all permissions
                        applied_perms_val.clear();
                    } else {
                        // Clear all perms with this namespace
                        const to_remove: Permission[] = [];
                        for (const [key, _] of applied_perms_val) {
                            if (key.namespace === perm.namespace) {
                                to_remove.push(key);
                            }
                        }

                        // Remove here
                        for (const key of to_remove) {
                            applied_perms_val.delete(key);
                        }
                    }
					continue;
				}

				if (perm.negator) {
					// Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
					let unnegated_perm = new Permission(perm.namespace, perm.perm, false);
					if (applied_perms_val.has(unnegated_perm)) {
						// Remove old permission
						applied_perms_val.delete(unnegated_perm);

						// Add the negator
						applied_perms_val.set(perm, pos.index);
					} else {
						if (applied_perms_val.has(perm)) {
							// Case 3: The negator is already applied, so we can ignore it
							continue;
						}

						// Then we can freely add the negator
						applied_perms_val.set(perm, pos.index);
					}
				} else {
					// Special case: If a * element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator
					if (perm.perm == "*") {
						// Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap
                        
						// If the * element is from a permission of lower index, then we can ignore this negator
						const to_remove: Permission[] = [];
						for (const [key, _] of applied_perms_val) {
							if (!key.negator) {
								continue; // This special case only applies to negators
							}
							// Same namespaces
							if (key.namespace === perm.namespace) {
								// Then we can ignore this negator
								to_remove.push(key);
							}
						}

						// Remove here
						for (const key of to_remove) {
							applied_perms_val.delete(key);
						}
					}

					// If its not a negator, first check if there's a negator
                    let negated = new Permission(perm.namespace, perm.perm, true);
					if (applied_perms_val.has(negated)) {
						// Remove the negator
						applied_perms_val.delete(negated);
						// Add the permission
						applied_perms_val.set(perm, pos.index);
					} else {
						if (applied_perms_val.has(perm)) {
							// Case 3: The permission is already applied, so we can ignore it
							continue;
						}

						// Then we can freely add the permission
						applied_perms_val.set(perm, pos.index);
					}
				}
			}
		}
		const applied_perms = applied_perms_val.keys();

		if (testMode) {
			console.log(
				`Applied perms: ${applied_perms} with hashmap: ${applied_perms_val}`,
			);
		}

		return applied_perms;
	}
}

/**
 * Check if the user has a permission given a set of user permissions and a permission to check for
 * 
 * @param perms The resolved permissions of the user
 * @param perm The permission to check for
 * @returns 
 */
export function hasPerm(perms: Permission[], perm: Permission): boolean {
	let has_perm = null;
	let has_negator = false;
	for (const user_perm of perms) {
		if (!user_perm.negator && user_perm.namespace == "global" && user_perm.perm === "*") {
			// Special case
			return true;
		}

		if (
			(user_perm.namespace === perm.namespace ||
				user_perm.namespace === "global") &&
			(user_perm.perm === "*" || user_perm.perm === perm.perm)
		) {
			// We have to check for all negator
			has_perm = user_perm;

			if (user_perm.negator) {
				has_negator = true; // While we can optimize here by returning false, we may want to add more negation systems in the future
			}
		}
	}
	return !!has_perm && !has_negator;
}

/**
 * Same as hasPerm, but takes in string arrays instead of Permission arrays
 */
export function hasPermString(perms: string[], perm: string): boolean {
    return hasPerm(Permission.fromList(perms), Permission.from(perm));
}

/**
 * Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position
 * 
 * @param managerPerms The permissions of the manager
 * @param currentPerms The current permissions of the user (must be resolved)
 * @param newPerms The new permissions of the user (must be resolved)
 */
export function checkPatchChanges(
	managerPerms: Permission[],
	currentPerms: Permission[],
	newPerms: Permission[],
): void {
    let manager_perms = managerPerms.map((perm) => perm.toString()) // Convert to string to avoid reference equality issues
    let current_perms = currentPerms.map((perm) => perm.toString())
    let new_perms = newPerms.map((perm) => perm.toString())

	// Take the symmetric_difference between current_perms and new_perms
	const diff1 = [
		...new Set([...current_perms].filter((x) => !new_perms.includes(x))),
	];
	const diff2 = [
		...new Set([...new_perms].filter((x) => !current_perms.includes(x))),
	];
	const changed = [...diff1, ...diff2];

	for (const _perm of changed) {
        let perm = Permission.from(_perm)

		let resolved_perm = perm;

		if (perm.negator) {
			// Strip the ~ from namespace to check it
			resolved_perm.negator = false;
		}

		// Check if the user has the permission
		if (!hasPerm(managerPerms, resolved_perm)) {
			throw new Error(
				`You do not have permission to add this permission: ${resolved_perm}`,
			);
		}

		if (perm.perm == "*") {
			// Ensure that new_perms has *at least* negators that manager_perms has within the namespace
			for (const perms of managerPerms) {
				if (!perms.negator) {
					continue; // Only check negators
				}

				if (
					perms.namespace === perm.namespace &&
					!new_perms.includes(perms.toString())
				) {
					// Then we have a negator in the same namespace
					throw new Error(
						`You do not have permission to add wildcard permission ${perm} with negators due to lack of negator ${perms}`,
					);
				}
			}
		}
	}
}

/**
 * Same as checkPatchChanges, but takes in string arrays instead of Permission arrays
 */
export function checkPatchChangesString(
    managerPerms: string[],
    currentPerms: string[],
    newPerms: string[],
): void {
    checkPatchChanges(Permission.fromList(managerPerms), Permission.fromList(currentPerms), Permission.fromList(newPerms));
}

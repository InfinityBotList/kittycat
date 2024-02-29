// Test related
export let testMode: boolean = false;
export function setTestMode(f: boolean) {
    testMode = f
}

export class PartialStaffPosition {
    id: string;
    index: number;
    perms: string[];

    constructor(id: string, index: number, perms: string[]) {
	this.id = id
	this.index = index
	this.perms = perms

	if(!Number.isInteger(index)) {
	    throw new Error("Index must be an integer")
	}
    }
}

export class StaffPermissions {
    user_positions: PartialStaffPosition[];
    perm_overrides: string[];
    
    constructor(user_positions: PartialStaffPosition[], perm_overrides: string[]) {
	this.user_positions = user_positions
	this.perm_overrides = perm_overrides
    }

    resolve(): string[] {
        const applied_perms_val: Map<string, number> = new Map();
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
                if (perm.endsWith(".@clear")) {
                    let perm_split = perm.split('.');
                    if (perm_split.length < 2) {
			// Then assume its a global permission on the namespace
                        perm_split = ["global", "@clear"];
                    }

                    const perm_namespace = perm_split[0];
                    if (perm_namespace === "global") {
			// Clear all perms
                        applied_perms_val.clear();
                        continue;
                    }

		    // Clear all perms with this namespace
                    const to_remove: string[] = [];
                    for (const [key, _] of applied_perms_val) {
                        let key_split = key.split('.');
                        if (key_split.length < 2) {
			    // Then assume its a global permission on the namespace
                            key_split = ["global", "*"];
                        }

                        const key_namespace = key_split[0];
                        if (key_namespace === perm_namespace) {
                            to_remove.push(key);
                        }
                    }
                    
		    // Remove here
		    for (const key of to_remove) {
                        applied_perms_val.delete(key);
                    }
                    continue;
                }

                if (perm.startsWith('~')) {
                    // Check what gave the permission. We *know* its sorted so we don't need to do anything but remove if it exists
		    let unnegated_perm = perm.slice(1)
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
                    if (perm.endsWith(".*")) {
			// Remove negators. As the permissions are sorted, we can just check if a negator is in the hashmap
                        const perm_split = perm.split('.');
                        const perm_namespace = perm_split[0];
                        
			// If the * element is from a permission of lower index, then we can ignore this negator
			const to_remove: string[] = [];
                        for (const [key, _] of applied_perms_val) {
                            if (!key.startsWith('~')) {
                                continue; // This special case only applies to negators
                            }
                            const key_namespace = key.split('.')[0].slice(1);
				
			    // Same namespaces
                            if (key_namespace === perm_namespace) {
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
                    if (applied_perms_val.has(`~${perm}`)) {
			// Remove the negator
                        applied_perms_val.delete(`~${perm}`);
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
        const applied_perms = Array.from(applied_perms_val.keys());
        
        if (testMode) {
            console.log(`Applied perms: ${applied_perms} with hashmap: ${applied_perms_val}`);
        }
        
        return applied_perms;
    }
}

// Check if the user has a permission given a set of user permissions and a permission to check for
// 
// This assumes a resolved set of permissions
export function hasPerm(perms: string[], perm: string): boolean {
    let perm_split = perm.split('.');
    if (perm_split.length < 2) {
        // Then assume its a global permission on the namespace
        perm_split = [perm, "*"];
    }
    
    const perm_namespace = perm_split[0];
    const perm_name = perm_split[1];
    let has_perm = null;
    let has_negator = false;
    for (const user_perm of perms) {
        if (user_perm === "global.*") {
	    // Special case
            return true;
        }
        let user_perm_split = user_perm.split('.');
        
	if (user_perm_split.length < 2) {
	    // Then assume its a global permission
            user_perm_split = [user_perm, "*"];
        }
        
	let user_perm_namespace = user_perm_split[0];
        const user_perm_name = user_perm_split[1];
       
	 if (user_perm.startsWith('~')) {
	    // Strip the ~ from namespace to check it
            user_perm_namespace = user_perm_namespace.slice(1);
        }

        if ((user_perm_namespace === perm_namespace || user_perm_namespace === "global") && (user_perm_name === "*" || user_perm_name === perm_name)) {
	    // We have to check for all negator
            has_perm = user_perm_split;

            if (user_perm.startsWith('~')) {
                has_negator = true; // While we can optimize here by returning false, we may want to add more negation systems in the future
            }
        }
    }
    return !!has_perm && !has_negator;
}

// Builds a permission string from a namespace and permission
export function build(namespace: string, perm: string): string {
    return `${namespace}.${perm}`;
}

// Checks whether or not a resolved set of permissions allows the addition or removal of a permission to a position
export function checkPatchChanges(manager_perms: string[], current_perms: string[], new_perms: string[]): void {
    // Take the symmetric_difference between current_perms and new_perms
    const diff1 = [...new Set([...current_perms].filter(x => !new_perms.includes(x)))];
    const diff2 = [...new Set([...new_perms].filter(x => !current_perms.includes(x)))];
    const changed = [...diff1, ...diff2];

    for (const perm of changed) {
        let resolved_perm = perm;

        if (perm.startsWith('~')) {
	    // Strip the ~ from namespace to check it
            resolved_perm = resolved_perm.slice(1);
        }

	// Check if the user has the permission
        if (!hasPerm(manager_perms, resolved_perm)) {
            throw new Error(`You do not have permission to add this permission: ${resolved_perm}`);
        }

        if (perm.endsWith(".*")) {
            const perm_split = perm.split('.').filter(Boolean);
            const perm_namespace = perm_split[0]; // SAFETY: split is guaranteed to have at least 1 element

	    // Ensure that new_perms has *at least* negators that manager_perms has within the namespace
            for (const perms of manager_perms) {
                if (!perms.startsWith('~')) {
                    continue; // Only check negators
                }

                const perms_split = perms.split('.');
                const perms_namespace = perms_split[0].slice(1);

                if (perms_namespace === perm_namespace && !new_perms.includes(perms)) {
		    // Then we have a negator in the same namespace
                    throw new Error(`You do not have permission to add wildcard permission ${perm} with negators due to lack of negator ${perms}`);
                }
            }
        }
    }
}

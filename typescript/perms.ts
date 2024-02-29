/**
 * Checks if a user has a permission
 * @param {string[]} perms - An array of permissions
 * @param {string} perm - The permission to check
 * @returns {boolean} Whether the user has the permission or not
 */
export const hasPerm = (perms: string[], perm: string): boolean => {
	let perm_split = perm.split(".");
	if (perm_split.length < 2) perm_split = [perm, "*"];

	const perm_namespace = perm_split[0];
	const perm_name = perm_split[1];

	let has_perm: string[] | null = null;
	let has_negator: boolean = false;
	for (const user_perm of perms) {
		if (user_perm === "global.*") return true;

		let user_perm_split = user_perm.split(".");

		if (user_perm_split.length < 2) user_perm_split = [user_perm, "*"];

		let user_perm_namespace = user_perm_split[0];
		const user_perm_name = user_perm_split[1];

		if (user_perm.startsWith("~"))
			user_perm_namespace = user_perm_namespace.substring(1);

		if (
			(user_perm_namespace === perm_namespace ||
				user_perm_namespace === "global") &&
			(user_perm_name === "*" || user_perm_name === perm_name)
		) {
			has_perm = user_perm_split;

			if (user_perm.startsWith("~")) has_negator = true;
		}
	}

	return has_perm !== null && !has_negator;
};

/**
 * Builds a permission
 * @param {string} namespace - The permission's namespace
 * @param {string} perm - The permission's name
 * @returns {string} The built permission
 */
export const build = (namespace: string, perm: string): string => {
	return `${namespace}.${perm}`;
};

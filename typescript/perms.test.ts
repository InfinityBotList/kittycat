import { hasPerm } from "./perms.js";

test("should return true if perm is global.*", () => {
	const perms = ["global.*"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(true);
});

test("should return true if perm is global and perms contains global.*", () => {
	const perms = ["global.*", "test"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(true);
});

test("should return false if perm is not in perms", () => {
	const perms = ["test"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(false);
});

test("should return false if perm is in perms with a negator", () => {
	const perms = ["~global.test"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(false);
});

test("should return true if perm is in perms without a negator", () => {
	const perms = ["global.test"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(true);
});

test("should return true if perm is in perms with a wildcard", () => {
	const perms = ["global.*"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(true);
});

test("should return true if perm is in perms with a wildcard and a negator", () => {
	const perms = ["~global.*"];
	const perm = "global.test";
	const result = hasPerm(perms, perm);
	expect(result).toBe(true);
});

import {
	setTestMode,
	hasPermString,
	checkPatchChanges,
	StaffPermissions,
	PartialStaffPosition,
	Permission
} from "./perms";

setTestMode(true);

console.assert = function (condition?: boolean, ...data: any[]) {
	if (!condition) {
		throw new Error("Assertion failed");
	}
};

function test_has_perm(): void {
	console.assert(hasPermString(["global.*"], "test") === true);
	console.assert(hasPermString(["rpc.*"], "global.*") === false);
	console.assert(hasPermString(["global.test"], "rpc.test") === true);
	console.assert(hasPermString(["global.test"], "rpc.view_bot_queue") === false);
	console.assert(hasPermString(["global.*"], "rpc.view_bot_queue") === true);
	console.assert(hasPermString(["rpc.*"], "rpc.ViewBotQueue") === true);
	console.assert(hasPermString(["rpc.BotClaim"], "rpc.ViewBotQueue") === false);
	console.assert(hasPermString(["apps.*"], "rpc.ViewBotQueue") === false);
	console.assert(hasPermString(["apps.*"], "rpc.*") === false);
	console.assert(hasPermString(["apps.test"], "rpc.test") === false);
	console.assert(hasPermString(["apps.*"], "apps.test") === true);
	console.assert(hasPermString(["~apps.*"], "apps.test") === false);
	console.assert(hasPermString(["apps.*", "~apps.test"], "apps.test") === false);
	console.assert(hasPermString(["~apps.test", "apps.*"], "apps.test") === false);
	console.assert(hasPermString(["apps.test"], "apps.test") === true);
	console.assert(hasPermString(["apps.test", "apps.*"], "apps.test") === true);
	console.assert(hasPermString(["~apps.test", "global.*"], "apps.test") === true);
	console.log("hasPermString tests passed");
}

function test_resolve_perms(): void {
	const arraysEqual = <T>(a1: T[], a2: T[]) => {
		return JSON.stringify(a1) == JSON.stringify(a2);
	};

	console.assert(
		arraysEqual(new StaffPermissions([], [Permission.from("rpc.test")]).resolve(), [
			Permission.from("rpc.test"),
		]),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[new PartialStaffPosition("test", 1, [Permission.from("rpc.test")])],
				[],
			).resolve(),
			[Permission.from("rpc.test")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test", 1, [Permission.from("rpc.test")]),
					new PartialStaffPosition("test2", 2, [Permission.from("rpc.test2")]),
				],
				[],
			).resolve(),
			[Permission.from("rpc.test2"), Permission.from("rpc.test")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test", 1, [
						Permission.from("rpc.test"),
						Permission.from("rpc.test2"),
					]),
					new PartialStaffPosition("test2", 2, [
						Permission.from("~rpc.test"),
						Permission.from("~rpc.test3"),
					]),
				],
				[],
			).resolve(),
			[Permission.from("~rpc.test3"), Permission.from("rpc.test"), Permission.from("rpc.test2")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test", 1, [
						Permission.from("~rpc.test"),
						Permission.from("rpc.test2"),
					]),
					new PartialStaffPosition("test2", 2, [
						Permission.from("~rpc.test3"),
						Permission.from("rpc.test"),
					]),
				],
				[],
			).resolve(),
			[Permission.from("~rpc.test3"), Permission.from("~rpc.test"), Permission.from("rpc.test2")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test", 1, [
						Permission.from("~rpc.test"),
						Permission.from("rpc.test2"),
						Permission.from("rpc.test3"),
					]),
					new PartialStaffPosition("test2", 2, [
						Permission.from("~rpc.test3"),
						Permission.from("~rpc.test2"),
					]),
				],
				[Permission.from("rpc.test")],
			).resolve(),
			[Permission.from("rpc.test2"), Permission.from("rpc.test3"), Permission.from("rpc.test")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test", 1, [
						Permission.from("~rpc.test"),
						Permission.from("rpc.test2"),
					]),
					new PartialStaffPosition("test", 1, [
						Permission.from("global.@clear"),
						Permission.from("~rpc.test"),
						Permission.from("rpc.test2"),
					]),
					new PartialStaffPosition("test2", 2, [
						Permission.from("~rpc.test3"),
						Permission.from("~rpc.test2"),
					]),
				],
				[Permission.from("~rpc.test"), Permission.from("rpc.test2"), Permission.from("rpc.test3")],
			).resolve(),
			[Permission.from("~rpc.test"), Permission.from("rpc.test2"), Permission.from("rpc.test3")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[new PartialStaffPosition("test", 1, [Permission.from("rpc.*")])],
				[],
			).resolve(),
			[Permission.from("rpc.*")],
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[
					new PartialStaffPosition("test2", 1, [
						Permission.from("~rpc.test3"),
						Permission.from("~rpc.test2"),
					]),
					new PartialStaffPosition("test", 2, [Permission.from("rpc.*")]),
				],
				[],
			).resolve(),
			Permission.fromList(["rpc.*", "~rpc.test3", "~rpc.test2"]),
		),
	);
	console.assert(
		arraysEqual(
			new StaffPermissions(
				[new PartialStaffPosition("reviewer", 1, Permission.fromList(["rpc.Claim"]))],
				Permission.fromList(["~rpc.Claim"]),
			).resolve(),
			Permission.fromList(["~rpc.Claim"]),
		),
	);
	console.log("resolve_perms tests passed");
}

function checkPatchChangesAndReturn(
	manager_perms: string[],
	current_perms: string[],
	new_perms: string[],
) {
	try {
		checkPatchChanges(Permission.fromList(manager_perms), Permission.fromList(current_perms), Permission.fromList(new_perms));
		return true;
	} catch (err) {
		return false;
	}
}

function test_check_patch_changes(): void {
	console.assert(
		checkPatchChangesAndReturn(
			["global.*"],
			["rpc.test"],
			["rpc.test", "rpc.test2"],
		) === true,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["rpc.*"],
			["global.*"],
			["rpc.test", "rpc.test2"],
		) === false,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["rpc.*"],
			["rpc.test"],
			["rpc.test", "rpc.test2"],
		) === true,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["~rpc.test", "rpc.*"],
			["rpc.foobar"],
			["rpc.*"],
		) === false,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["~rpc.test", "rpc.*"],
			["~rpc.test"],
			["rpc.*"],
		) === false,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["~rpc.test", "rpc.*"],
			["~rpc.test"],
			["rpc.*", "~rpc.test", "~rpc.test2"],
		) === true,
	);
	console.assert(
		checkPatchChangesAndReturn(
			["~rpc.test", "rpc.*"],
			["~rpc.test"],
			["rpc.*", "~rpc.test2", "~rpc.test2"],
		) === false,
	);
	console.log("checkPatchChanges tests passed");
}

[test_has_perm, test_resolve_perms, test_check_patch_changes].forEach((test) =>
	test(),
);

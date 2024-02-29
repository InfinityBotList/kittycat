import { setTestMode, hasPerm, checkPatchChanges, StaffPermissions, PartialStaffPosition } from "./perms";

setTestMode(true)

console.assert = function(condition?: boolean, ...data: any[]) {
	if(!condition) {
		throw new Error("Assertion failed")
	}
}

function test_has_perm(): void {
    console.assert(hasPerm(["global.*"], "test") === true);
    console.assert(hasPerm(["rpc.*"], "global.*") === false);
    console.assert(hasPerm(["global.test"], "rpc.test") === true);
    console.assert(hasPerm(["global.test"], "rpc.view_bot_queue") === false);
    console.assert(hasPerm(["global.*"], "rpc.view_bot_queue") === true);
    console.assert(hasPerm(["rpc.*"], "rpc.ViewBotQueue") === true);
    console.assert(hasPerm(["rpc.BotClaim"], "rpc.ViewBotQueue") === false);
    console.assert(hasPerm(["apps.*"], "rpc.ViewBotQueue") === false);
    console.assert(hasPerm(["apps.*"], "rpc.*") === false);
    console.assert(hasPerm(["apps.test"], "rpc.test") === false);
    console.assert(hasPerm(["apps.*"], "apps.test") === true);
    console.assert(hasPerm(["~apps.*"], "apps.test") === false);
    console.assert(hasPerm(["apps.*", "~apps.test"], "apps.test") === false);
    console.assert(hasPerm(["~apps.test", "apps.*"], "apps.test") === false);
    console.assert(hasPerm(["apps.test"], "apps.test") === true);
    console.assert(hasPerm(["apps.test", "apps.*"], "apps.test") === true);
    console.assert(hasPerm(["~apps.test", "global.*"], "apps.test") === true);
    console.log("hasPerm tests passed");
}

function test_resolve_perms(): void {
    const arraysEqual = (a1: any[], a2: any[]) => {
        return JSON.stringify(a1)==JSON.stringify(a2);
    }

    console.assert(arraysEqual(new StaffPermissions(
        [], 
        ["rpc.test"]
    ).resolve(), ["rpc.test"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("test", 1, ["rpc.test"])
        ], 
        []
    ).resolve(), ["rpc.test"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("test", 1, ["rpc.test"]), 
            new PartialStaffPosition("test2", 2, ["rpc.test2"])
        ],
        []
    ).resolve(), ["rpc.test2", "rpc.test"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("test", 1, ["rpc.test", "rpc.test2"]),
            new PartialStaffPosition("test2", 2, ["~rpc.test", "~rpc.test3"])
        ], 
        []
    ).resolve(), ["~rpc.test3", "rpc.test", "rpc.test2"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("test", 1, ["~rpc.test", "rpc.test2"]),
            new PartialStaffPosition("test2", 2, ["~rpc.test3", "rpc.test"])
        ], 
        []
    ).resolve(), ["~rpc.test3", "~rpc.test", "rpc.test2"]));
    console.assert(arraysEqual(new StaffPermissions([new PartialStaffPosition("test", 1, ["~rpc.test", "rpc.test2", "rpc.test3"]), new PartialStaffPosition("test2", 2, ["~rpc.test3", "~rpc.test2"])], ["rpc.test"]).resolve(), ["rpc.test2", "rpc.test3", "rpc.test"]));
    console.assert(arraysEqual(new StaffPermissions([new PartialStaffPosition("test", 1, ["~rpc.test", "rpc.test2"]), new PartialStaffPosition("test", 1, ["global.@clear", "~rpc.test", "rpc.test2"]), new PartialStaffPosition("test2", 2, ["~rpc.test3", "~rpc.test2"])], ["~rpc.test", "rpc.test2", "rpc.test3"]).resolve(), ["~rpc.test", "rpc.test2", "rpc.test3"]));
    console.assert(arraysEqual(new StaffPermissions([new PartialStaffPosition("test", 1, ["rpc.*"])], []).resolve(), ["rpc.*"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("test2", 1, ["~rpc.test3", "~rpc.test2"]),
            new PartialStaffPosition("test", 2, ["rpc.*"])
        ],
        []
    ).resolve(), ["rpc.*", "~rpc.test3", "~rpc.test2"]));
    console.assert(arraysEqual(new StaffPermissions(
        [
            new PartialStaffPosition("reviewer", 1, ["rpc.Claim"])
        ], 
        ["~rpc.Claim"]
    ).resolve(), ["~rpc.Claim"]));
    console.log("resolve_perms tests passed");
}

function checkPatchChangesAndReturn(manager_perms: string[], current_perms: string[], new_perms: string[]) {
	try {
		checkPatchChanges(manager_perms, current_perms, new_perms)
		return true
	} catch (err) {
		return false
	}
}

function test_check_patch_changes(): void {
    console.assert(checkPatchChangesAndReturn(["global.*"], ["rpc.test"], ["rpc.test", "rpc.test2"]) === true);
    console.assert(checkPatchChangesAndReturn(["rpc.*"], ["global.*"], ["rpc.test", "rpc.test2"]) === false);
    console.assert(checkPatchChangesAndReturn(["rpc.*"], ["rpc.test"], ["rpc.test", "rpc.test2"]) === true);
    console.assert(checkPatchChangesAndReturn(["~rpc.test", "rpc.*"], ["rpc.foobar"], ["rpc.*"]) === false);
    console.assert(checkPatchChangesAndReturn(["~rpc.test", "rpc.*"], ["~rpc.test"], ["rpc.*"]) === false);
    console.assert(checkPatchChangesAndReturn(["~rpc.test", "rpc.*"], ["~rpc.test"], ["rpc.*", "~rpc.test", "~rpc.test2"]) === true);
    console.assert(checkPatchChangesAndReturn(["~rpc.test", "rpc.*"], ["~rpc.test"], ["rpc.*", "~rpc.test2", "~rpc.test2"]) === false);
    console.log("checkPatchChanges tests passed");
}

[test_has_perm, test_resolve_perms, test_check_patch_changes].forEach(test => test());

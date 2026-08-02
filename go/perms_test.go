package perms

import "testing"

func TestHasPerm(t *testing.T) {
	if !HasPerm(PFSS([]string{"global.*"}), PFS("test")) {
		t.Error("Expected true, got false")
	}
	if HasPerm(PFSS([]string{"rpc.*"}), PFS("global.*")) {
		t.Error("Expected false, got true")
	}
	if !HasPerm(PFSS([]string{"global.test"}), PFS("rpc.test")) {
		t.Error("Expected true, got false")
	}
	if HasPerm(PFSS([]string{"global.test"}), PFS("rpc.view_bot_queue")) {
		t.Error("Expected false, got true")
	}
	if !HasPerm(PFSS([]string{"global.*"}), PFS("rpc.view_bot_queue")) {
		t.Error("Expected true, got false")
	}
	if !HasPerm(PFSS([]string{"rpc.*"}), PFS("rpc.ViewBotQueue")) {
		t.Error("Expected true, got false")
	}
	if HasPerm(PFSS([]string{"rpc.BotClaim"}), PFS("rpc.ViewBotQueue")) {
		t.Error("Expected false, got true")
	}
	if HasPerm(PFSS([]string{"apps.*"}), PFS("rpc.ViewBotQueue")) {
		t.Error("Expected false, got true")
	}
	if HasPerm(PFSS([]string{"apps.*"}), PFS("rpc.*")) {
		t.Error("Expected false, got true")
	}
	if HasPerm(PFSS([]string{"apps.test"}), PFS("rpc.test")) {
		t.Error("Expected false, got true")
	}
	if !HasPerm(PFSS([]string{"apps.*"}), PFS("apps.test")) {
		t.Error("Expected true, got false")
	}
	if HasPerm(PFSS([]string{"~apps.*"}), PFS("apps.test")) {
		t.Error("Expected false, got true")
	}
	if HasPerm(PFSS([]string{"apps.*", "~apps.test"}), PFS("apps.test")) {
		t.Error("Expected false, got true")
	}
	if HasPerm(PFSS([]string{"~apps.test", "apps.*"}), PFS("apps.test")) {
		t.Error("Expected false, got true")
	}
	if !HasPerm(PFSS([]string{"apps.test"}), PFS("apps.test")) {
		t.Error("Expected true, got false")
	}
	if !HasPerm(PFSS([]string{"apps.test", "apps.*"}), PFS("apps.test")) {
		t.Error("Expected true, got false")
	}
	if !HasPerm(PFSS([]string{"~apps.test", "global.*"}), PFS("apps.test")) {
		t.Error("Expected true, got false")
	}
}

func TestResolvePerms(t *testing.T) {
	// Test for basic resolution of overrides
	expected := []Permission{PFS("rpc.test")}
	result := StaffPermissions{
		UserPositions: []PartialStaffPosition{},
		PermOverrides: []Permission{PFS("rpc.test")},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Test for basic resolution of single position
	expected = []Permission{PFS("rpc.test")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("rpc.test")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Test for basic resolution of multiple positions
	expected = []Permission{PFS("rpc.test2"), PFS("rpc.test")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("rpc.test")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("rpc.test2")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Test for basic resolution of multiple positions with negators
	expected = []Permission{PFS("~rpc.test3"), PFS("rpc.test"), PFS("rpc.test2")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("rpc.test"), PFS("rpc.test2")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("~rpc.test"), PFS("~rpc.test3")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Same as above but testing negator ordering
	expected = []Permission{PFS("~rpc.test3"), PFS("~rpc.test"), PFS("rpc.test2")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("~rpc.test"), PFS("rpc.test2")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("~rpc.test3"), PFS("rpc.test")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Now mix everything together
	expected = []Permission{PFS("rpc.test2"), PFS("rpc.test3"), PFS("rpc.test")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("~rpc.test"), PFS("rpc.test2"), PFS("rpc.test3")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("~rpc.test3"), PFS("~rpc.test2")},
			},
		},
		PermOverrides: []Permission{PFS("rpc.test")},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// @clear
	expected = []Permission{PFS("~rpc.test"), PFS("rpc.test2"), PFS("rpc.test3")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("~rpc.test"), PFS("rpc.test2")},
			},
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("global.@clear"), PFS("~rpc.test"), PFS("rpc.test2")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("~rpc.test3"), PFS("~rpc.test2")},
			},
		},
		PermOverrides: []Permission{PFS("~rpc.test"), PFS("rpc.test2"), PFS("rpc.test3")},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Special case of * with negators
	expected = []Permission{PFS("rpc.*")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test",
				Index: 1,
				Perms: []Permission{PFS("rpc.*")},
			},
			{
				ID:    "test2",
				Index: 2,
				Perms: []Permission{PFS("~rpc.test3"), PFS("~rpc.test2")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Ensure special case does not apply when index is higher (2 > 1 in the below)
	expected = []Permission{PFS("rpc.*"), PFS("~rpc.test3"), PFS("~rpc.test2")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "test2",
				Index: 1,
				Perms: []Permission{PFS("~rpc.test3"), PFS("~rpc.test2")},
			},
			{
				ID:    "test",
				Index: 2,
				Perms: []Permission{PFS("rpc.*")},
			},
		},
		PermOverrides: []Permission{},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}

	// Some common cases
	// Ensure special case does not apply when index is higher (2 > 1 in the below)
	expected = []Permission{PFS("~rpc.Claim")}
	result = StaffPermissions{
		UserPositions: []PartialStaffPosition{
			{
				ID:    "reviewer",
				Index: 1,
				Perms: []Permission{PFS("rpc.Claim")},
			},
		},
		PermOverrides: []Permission{PFS("~rpc.Claim")},
	}.Resolve()
	if !equal(result, expected) {
		t.Errorf("Expected %v, got %v", expected, result)
	}
}

func TestCheckPatchChanges(t *testing.T) {
	err := CheckPatchChanges([]Permission{PFS("global.*")}, []Permission{PFS("rpc.test")}, PFSS([]string{"rpc.test", "rpc.test2"}))
	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}

	err = CheckPatchChanges([]Permission{PFS("rpc.*")}, []Permission{PFS("global.*")}, PFSS([]string{"rpc.test", "rpc.test2"}))
	if err == nil {
		t.Error("Expected error, got nil")
	}

	err = CheckPatchChanges([]Permission{PFS("rpc.*")}, []Permission{PFS("rpc.test")}, PFSS([]string{"rpc.test", "rpc.test2"}))
	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}

	// If adding '*' permissions, target must have all negators of user
	err = CheckPatchChanges([]Permission{PFS("~rpc.test"), PFS("rpc.*")}, []Permission{PFS("rpc.foobar")}, []Permission{PFS("rpc.*")})
	if err == nil {
		t.Error("Expected error, got nil")
	}

	err = CheckPatchChanges([]Permission{PFS("~rpc.test"), PFS("rpc.*")}, []Permission{PFS("~rpc.test")}, []Permission{PFS("rpc.*")})
	if err == nil {
		t.Error("Expected error, got nil")
	}

	err = CheckPatchChanges([]Permission{PFS("~rpc.test"), PFS("rpc.*")}, []Permission{PFS("~rpc.test")}, PFSS([]string{"rpc.*", "~rpc.test", "~rpc.test2"}))
	if err != nil {
		t.Errorf("Expected no error, got %v", err)
	}

	err = CheckPatchChanges([]Permission{PFS("~rpc.test"), PFS("rpc.*")}, []Permission{PFS("~rpc.test")}, PFSS([]string{"rpc.*", "~rpc.test2", "~rpc.test2"}))
	if err == nil {
		t.Error("Expected error, got nil")
	}
}

func equal[T comparable](a, b []T) bool {
	if len(a) != len(b) {
		return false
	}
	for i, v := range a {
		if v != b[i] {
			return false
		}
	}
	return true
}

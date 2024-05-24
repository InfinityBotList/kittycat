#include "perms.c"

int deconstruct_permission__test(char *perm)
{
    struct kittycat_string *perm_str = new_string(perm, strlen(perm));
    struct Permission *p = permission_from_str(perm_str);

    if (p == NULL)
    {
        string_free(perm_str);
        printf("ERROR: Permission is NULL\n");
        return 1;
    }
    else
    {
        char *fp = permission_to_str(p);
        printf("Namespace: %s\n", p->namespace->str);
        printf("Perm: %s\n", p->perm->str);
        printf("Negator: %s\n", p->negator ? "true" : "false");
        printf("Final permission: %s\n", fp);
        free(fp);
    }

    permission_free(p);
    string_free(perm_str);

    return 0;
}

bool has_perm_test_impl(char **str, char *perm, size_t len)
{
    struct PermissionList *perms = new_permission_list();

    for (size_t i = 0; i < len; i++)
    {
        struct kittycat_string *perm_str = new_string(str[i], strlen(str[i]));
        struct Permission *p = permission_from_str(perm_str);
        string_free(perm_str);

        permission_list_add(perms, p);
    }

    struct kittycat_string *perm_str = new_string(perm, strlen(perm));
    struct Permission *p = permission_from_str(perm_str);
    struct kittycat_string *permlist_joined = permission_list_join(perms, ", ");

    printf("Checking permission: %s against [%s] (len=%zu)\n", perm_str->str, permlist_joined->str, perms->len);

    bool res = has_perm(perms, p);

    string_free(permlist_joined);
    string_free(perm_str);
    permission_free(p);
    permission_list_free(perms);

    return res;
}

int has_perm__test()
{
    if (!has_perm_test_impl((char *[]){"global.*"}, "test", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"rpc.*"}, "global.*", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"global.test"}, "rpc.test", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"global.test"}, "rpc.view_bot_queue", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"global.*"}, "rpc.view_bot_queue", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"rpc.*"}, "rpc.ViewBotQueue", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"rpc.BotClaim"}, "rpc.ViewBotQueue", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"apps.*"}, "rpc.ViewBotQueue", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"apps.*"}, "rpc.*", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"apps.test"}, "rpc.test", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"apps.*"}, "apps.test", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"~apps.*"}, "apps.test", 1))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"apps.*", "~apps.test"}, "apps.test", 2))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (has_perm_test_impl((char *[]){"~apps.test", "apps.*"}, "apps.test", 2))
    {
        printf("Expected false, got true\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"apps.test"}, "apps.test", 1))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"apps.test", "apps.*"}, "apps.test", 2))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    if (!has_perm_test_impl((char *[]){"~apps.test", "global.*"}, "apps.test", 2))
    {
        printf("Expected true, got false\n");
        return 1;
    }

    return 0;
}

bool sp_resolve_test_impl(struct StaffPermissions *sp, struct PermissionList *expected_perms, struct __OrderedPermissionMap *opm)
{
    struct PermissionList *perms = __staff_permissions_resolve(sp, opm);

    struct kittycat_string *expected_perms_str = permission_list_join(expected_perms, ", ");
    struct kittycat_string *perms_str = permission_list_join(perms, ", ");
    bool res = permission_lists_equal(perms, expected_perms);

    printf("Expected: [%s], got [%s], isEqual=%s\n", expected_perms_str->str, perms_str->str, res ? "true" : "false");

    permission_list_free(perms);
    permission_list_free(expected_perms);
    string_free(expected_perms_str);
    string_free(perms_str);
    staff_permissions_free(sp);
    __ordered_permission_map_clear(opm); // Clear the OPM for reuse

    return res;
}

// TODO: Finish porting all tests
int sp_resolve__test()
{
    /*
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
    */

    struct __OrderedPermissionMap *opm = __new_ordered_permission_map();
    struct kittycat_string *rpcTest = new_string("rpc.test", 8);
    struct kittycat_string *rpcTest2 = new_string("rpc.test2", 9);
    struct kittycat_string *rpcTest3 = new_string("rpc.test3", 9);
    struct kittycat_string *rpcClaim = new_string("rpc.Claim", 9);
    struct kittycat_string *rpcStar = new_string("rpc.*", 5);
    struct kittycat_string *NrpcTest = new_string("~rpc.test", 9);
    struct kittycat_string *NrpcTest2 = new_string("~rpc.test2", 10);
    struct kittycat_string *NrpcTest3 = new_string("~rpc.test3", 10);
    struct kittycat_string *NrpcClaim = new_string("~rpc.Claim", 10);
    struct kittycat_string *NrpcStar = new_string("~rpc.*", 6);
    struct kittycat_string *global_clear = new_string("global.@clear", 13);

    // Test for basic resolution of overrides
    struct PermissionList *expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcTest),
        },
        1);
    struct StaffPermissions *sp = new_staff_permissions();
    permission_list_add(sp->perm_overrides, permission_from_str(rpcTest));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of single position
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcTest),
        },
        1);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcTest)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of multiple positions
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcTest2),
            permission_from_str(rpcTest),
        },
        2);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcTest)}, 1)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcTest2)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of multiple positions with negators
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(NrpcTest3),
            permission_from_str(rpcTest),
            permission_from_str(rpcTest2),
        },
        3);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcTest), permission_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest), permission_from_str(NrpcTest3)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Same as above but testing negator ordering
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(NrpcTest3),
            permission_from_str(NrpcTest),
            permission_from_str(rpcTest2),
        },
        3);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest), permission_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest3), permission_from_str(rpcTest)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Now mix everything together
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcTest2),
            permission_from_str(rpcTest3),
            permission_from_str(rpcTest),
        },
        3);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest), permission_from_str(rpcTest2), permission_from_str(rpcTest3)}, 3)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest3), permission_from_str(NrpcTest2)}, 2)));
    permission_list_add(sp->perm_overrides, permission_from_str(rpcTest));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // @clear
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(NrpcTest),
            permission_from_str(rpcTest2),
            permission_from_str(rpcTest3),
        },
        3);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest), permission_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(global_clear), permission_from_str(NrpcTest), permission_from_str(rpcTest2)}, 3)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest3), permission_from_str(NrpcTest2)}, 2)));
    permission_list_add(sp->perm_overrides, permission_from_str(NrpcTest));
    permission_list_add(sp->perm_overrides, permission_from_str(rpcTest2));
    permission_list_add(sp->perm_overrides, permission_from_str(rpcTest3));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Special case of * with negators
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcStar),
        },
        1);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcStar)}, 1)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest3), permission_from_str(NrpcTest2)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Ensure special case does not apply when index is higher (2 > 1 in the below)
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(rpcStar),
            permission_from_str(NrpcTest3),
            permission_from_str(NrpcTest2),
        },
        3);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test2", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(NrpcTest3), permission_from_str(NrpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("test", 2, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcStar)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Some common cases
    // Ensure special case does not apply when index is higher (2 > 1 in the below)
    expected = new_permission_list_with_perms(
        (struct Permission *[]){
            permission_from_str(NrpcClaim),
        },
        1);

    sp = new_staff_permissions();
    partial_staff_position_list_add(sp->user_positions, new_partial_staff_position("reviewer", 1, new_permission_list_with_perms((struct Permission *[]){permission_from_str(rpcClaim)}, 1)));
    permission_list_add(sp->perm_overrides, permission_from_str(NrpcClaim));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Free memory
    string_free(rpcTest);
    string_free(rpcTest2);
    string_free(rpcTest3);
    string_free(rpcClaim);
    string_free(rpcStar);
    string_free(NrpcTest);
    string_free(NrpcTest2);
    string_free(NrpcTest3);
    string_free(NrpcClaim);
    string_free(NrpcStar);
    string_free(global_clear);
    __ordered_permission_map_free(opm);

    return 0;
}

int main()
{
    printf("Running tests: %s...\n", ":)");

    char *checks[] = {"bar", "global.bar", "bot.foo", "~bot.foo"};

    for (int i = 0; i < 4; i++)
    {
        deconstruct_permission__test(checks[i]);
    }

    int rc = has_perm__test();
    if (rc)
    {
        return rc;
    }

    rc = sp_resolve__test();
    if (rc)
    {
        return rc;
    }

    printf("All tests passed :)\n");

    return 0;
}

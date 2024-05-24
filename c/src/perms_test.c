#include "perms.c"

#ifdef DEBUG_PRINTF
#define printff(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define printf(fmt, ...)
#endif

#if defined(DECONSTRUCT_kittycat_permission_CHECKS)
int deconstruct_kittycat_permission__test(char *perm)
{
    struct kittycat_string *perm_str = new_string(perm, strlen(perm));
    struct KittycatPermission *p = kittycat_permission_new_from_str(perm_str);

    if (p == NULL)
    {
        string_free(perm_str);
        printf("ERROR: KittycatPermission is NULL\n");
        return 1;
    }
    else
    {
        char *fp = kittycat_permission_to_str(p);
        printf("Namespace: %s\n", p->namespace->str);
        printf("Perm: %s\n", p->perm->str);
        printf("Negator: %s\n", p->negator ? "true" : "false");
        printf("Final KittycatPermission: %s\n", fp);
        free(fp);
    }

    kittycat_permission_free(p);
    string_free(perm_str);

    return 0;
}
#endif

bool has_perm_test_impl(char **str, char *perm, size_t len)
{
    struct KittycatPermissionList *perms = kittycat_permission_list_new();

    for (size_t i = 0; i < len; i++)
    {
        struct kittycat_string *perm_str = new_string(str[i], strlen(str[i]));
        struct KittycatPermission *p = kittycat_permission_new_from_str(perm_str);
        string_free(perm_str);

        kittycat_permission_list_add(perms, p);
    }

    struct kittycat_string *perm_str = new_string(perm, strlen(perm));
    struct KittycatPermission *p = kittycat_permission_new_from_str(perm_str);
    struct kittycat_string *permlist_joined = kittycat_permission_list_join(perms, ", ");

    printf("Checking KittycatPermission: %s against [%s] (len=%zu)\n", perm_str->str, permlist_joined->str, perms->len);

    bool res = kittycat_has_perm(perms, p);

    string_free(permlist_joined);
    string_free(perm_str);
    kittycat_permission_free(p);
    kittycat_permission_list_free(perms);

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

bool sp_resolve_test_impl(struct StaffKittycatPermissions *sp, struct KittycatPermissionList *expected_perms, struct __KittycatOrderedPermissionMap *opm)
{
    struct KittycatPermissionList *perms = __kittycat_staff_permissions_resolve(sp, opm);

    struct kittycat_string *expected_perms_str = kittycat_permission_list_join(expected_perms, ", ");
    struct kittycat_string *perms_str = kittycat_permission_list_join(perms, ", ");
    bool res = kittycat_permission_lists_equal(perms, expected_perms);

    printf("Expected: [%s], got [%s], isEqual=%s\n", expected_perms_str->str, perms_str->str, res ? "true" : "false");

    kittycat_permission_list_free(perms);
    kittycat_permission_list_free(expected_perms);
    string_free(expected_perms_str);
    string_free(perms_str);
    kittycat_staff_permissions_free(sp);
    __kittycat_ordered_permission_map_clear(opm); // Clear the OPM for reuse

    return res;
}

int sp_resolve__test()
{
    struct __KittycatOrderedPermissionMap *opm = __kittycat_ordered_permission_map_new();
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
    struct KittycatPermissionList *expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcTest),
        },
        1);
    struct StaffKittycatPermissions *sp = kittycat_staff_permissions_new();
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(rpcTest));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of single position
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcTest),
        },
        1);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcTest)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of multiple positions
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcTest2),
            kittycat_permission_new_from_str(rpcTest),
        },
        2);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcTest)}, 1)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcTest2)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Test for basic resolution of multiple positions with negators
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(NrpcTest3),
            kittycat_permission_new_from_str(rpcTest),
            kittycat_permission_new_from_str(rpcTest2),
        },
        3);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcTest), kittycat_permission_new_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest), kittycat_permission_new_from_str(NrpcTest3)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Same as above but testing negator ordering
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(NrpcTest3),
            kittycat_permission_new_from_str(NrpcTest),
            kittycat_permission_new_from_str(rpcTest2),
        },
        3);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest), kittycat_permission_new_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest3), kittycat_permission_new_from_str(rpcTest)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Now mix everything together
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcTest2),
            kittycat_permission_new_from_str(rpcTest3),
            kittycat_permission_new_from_str(rpcTest),
        },
        3);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest), kittycat_permission_new_from_str(rpcTest2), kittycat_permission_new_from_str(rpcTest3)}, 3)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest3), kittycat_permission_new_from_str(NrpcTest2)}, 2)));
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(rpcTest));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // @clear
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(NrpcTest),
            kittycat_permission_new_from_str(rpcTest2),
            kittycat_permission_new_from_str(rpcTest3),
        },
        3);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest), kittycat_permission_new_from_str(rpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(global_clear), kittycat_permission_new_from_str(NrpcTest), kittycat_permission_new_from_str(rpcTest2)}, 3)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest3), kittycat_permission_new_from_str(NrpcTest2)}, 2)));
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(NrpcTest));
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(rpcTest2));
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(rpcTest3));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Special case of * with negators
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcStar),
        },
        1);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcStar)}, 1)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest3), kittycat_permission_new_from_str(NrpcTest2)}, 2)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Ensure special case does not apply when index is higher (2 > 1 in the below)
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(rpcStar),
            kittycat_permission_new_from_str(NrpcTest3),
            kittycat_permission_new_from_str(NrpcTest2),
        },
        3);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test2", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(NrpcTest3), kittycat_permission_new_from_str(NrpcTest2)}, 2)));
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("test", 2, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcStar)}, 1)));

    if (!sp_resolve_test_impl(sp, expected, opm))
    {
        return 1;
    }

    // Some common cases
    // Ensure special case does not apply when index is higher (2 > 1 in the below)
    expected = kittycat_permission_list_new_with_perms(
        (struct KittycatPermission *[]){
            kittycat_permission_new_from_str(NrpcClaim),
        },
        1);

    sp = kittycat_staff_permissions_new();
    partial_staff_position_list_add(sp->user_positions, partial_staff_position_new("reviewer", 1, kittycat_permission_list_new_with_perms((struct KittycatPermission *[]){kittycat_permission_new_from_str(rpcClaim)}, 1)));
    kittycat_permission_list_add(sp->perm_overrides, kittycat_permission_new_from_str(NrpcClaim));

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
    __kittycat_ordered_permission_map_free(opm);

    return 0;
}

int main()
{
    printf("Running tests: %s...\n", ":)");

#if defined(DECONSTRUCT_kittycat_permission_CHECKS)
    char *checks[] = {"bar", "global.bar", "bot.foo", "~bot.foo"};

    for (int i = 0; i < 4; i++)
    {
        if (deconstruct_kittycat_permission__test(checks[i]))
        {
            return 1;
        }
    }
#endif

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

    // Print "All tests passed" to stdout
    fprintf(stdout, "All tests passed\n");

    return 0;
}

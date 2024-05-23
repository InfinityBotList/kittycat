#include "perms.c"

int check_permchecking(char *perm)
{
    struct string *perm_str = string_from_char(perm);
    struct Permission *p = permission_from_str(perm_str);
    string_free(perm_str);

    if (p == NULL)
    {
        printf("ERROR: Permission is NULL\n");
        return 1;
    }
    else
    {
        char *fp = permission_to_str(p);
        printf("Namespace: %s\n", p->namespace->str);
        printf("Perm: %s\n", p->perm->str);
        printf("Negator: %d\n", p->negator);
        printf("Final permission: %s\n", fp);
        free(fp);
    }

    permission_free(p);

    return 0;
}

bool has_perm_test_impl(char **str, char *perm, size_t len)
{
    struct PermissionList *perms = new_permission_list();

    for (int i = 0; i < len; i++)
    {
        struct string *perm_str = string_from_char(str[i]);
        struct Permission *p = permission_from_str(perm_str);
        string_free(perm_str);

        permission_list_add(perms, p);
    }

    struct string *perm_str = string_from_char(perm);
    struct Permission *p = permission_from_str(perm_str);
    struct string *permlist_joined = permission_list_join(perms, ", ");

    printf("Checking permission: %s against [%s] (len=%zu)\n", perm_str->str, permlist_joined->str, perms->len);

    bool res = has_perm(perms, p);

    string_free(permlist_joined);
    string_free(perm_str);
    permission_free(p);
    permission_list_free(perms);

    return res;
}

int main()
{
    printf("Running tests: %s...\n", ":)");

    char *checks[] = {"bar", "global.bar", "bot.foo", "~bot.foo"};

    for (int i = 0; i < 4; i++)
    {
        check_permchecking(checks[i]);
    }

    /*
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
    */

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

    printf("All tests passed :)\n");

    return 0;
}
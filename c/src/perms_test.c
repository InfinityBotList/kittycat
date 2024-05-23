#include "perms.c"

int check(char *perm)
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
        struct string *fp = permission_to_str(p);
        printf("Namespace: %s\n", p->namespace->str);
        printf("Perm: %s\n", p->perm->str);
        printf("Negator: %d\n", p->negator);
        printf("Final permission: %s\n", fp->str);
        string_free(fp);
    }

    permission_free(p);

    return 0;
}

int main()
{
    printf("Running tests: %s...\n", ":)");

    char *checks[] = {"bar", "global.bar", "bot.foo", "~bot.foo"};

    for (int i = 0; i < 4; i++)
    {
        check(checks[i]);
    }

    return 0;
}
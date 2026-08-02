# Kittycat TS

Typescript library for the kittycat permission system

## Supported Operations

-   `hasPerm`
-   `StaffPermissions#resolve`
-   `checkPatchChanges`

## Structure

A permission is defined as the following structure

`namespace.permission`

If a user has the \* permission, they will have all permissions in that namespace
If namespace is global then only the permission is checked. E.g. global.view allows using the view permission in all namespaces

If a permission has no namespace, it will be assumed to be global

If a permission has ~ in the beginning of its namespace, it is a negator permission that removes that specific permission from the user

Negators work to negate a specific permission _excluding the global._ permission* (for now, until this gets a bit more refined to not need a global.* special case)

Anything past the <namespace>.<permission> may be ignored or indexed at the discretion of the implementation and is _undefined behaviour_

# Permission Overrides

Permission overrides are a special case of permissions that are used to override permissions for a specific user.
They use the same structure and follow the same rules as a normal permission, but are stored separately from the normal permissions.

# Special Cases

-   Special case: If a \* element exists for a smaller index, then the negator must be ignored. E.g. manager has ~rpc.PremiumAdd but head_manager has no such negator

# Clearing Permissions

In some cases, it may desired to start from a fresh slate of permissions. To do this, add a '@clear' permission to the namespace. All permissions after this on that namespace will be cleared

TODO: Use enums for storing permissions instead of strings by serializing and deserializing them to strings when needed

# Permission Management

A permission can be added/managed by a user to a position if the following applies:

-   The user must _have_ the permission themselves. If the permission is a negator, then the user must have the 'base' permission (permission without the negator operator)
-   If the permission is `*`, then the user has no negators in that namespace that the target perm set would not also have

Note on point 2: this means that if a user is trying to add/remove `rpc.*` but also has `~rpc.test`, then they cannot add `rpc.*` unless the target user also has `~rpc.test`

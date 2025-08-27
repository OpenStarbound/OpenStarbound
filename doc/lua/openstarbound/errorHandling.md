# Error Handling

Error handling scripts are defined in the `_metadata` of a mod.
They get access to `root` and `sb`

- itemError
- objectError
- npcError
- playerError
- monsterError
- vehicleError

When an error occurs loading from disk, the `error(diskStore, e)` is invoked in each mod's scripts in reverse priority order until the error is entirely handled, or it isn't.
This means that a mod with a high priority, such as opensb_base with a priority of -9999 will execute its error handling scripts *last* (as its scripts are now responsible for generic items)

`diskStore` being the stored data to be modified and returned to potentially handle the error. It will only try to reload if the output has actually been changed.

`e` being the string of the exception that occured.

If none of the scripts handled the load errors fully, then the error will be thrown as normal.

function rebuild(diskStore, e)
    sb.logError("Failed to deserialize object '%s'. %s", sb.printJson(diskStore), e)
    local parameters = diskStore.parameters
    local name = diskStore.name
    diskStore.name = "perfectlygenericitem"
    diskStore.parameters = {
        genericItemStorage = {
            name = name,
            parameters = parameters
        },
        shortdescription = name,
        description = "Reinstall the parent mod to return this item to normal"
    }

    return diskStore
end

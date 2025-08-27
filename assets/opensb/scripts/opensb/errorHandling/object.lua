-- invoked when an Object loaded from disk fails to deserialize properly

-- we're catching any object error and converting them to generic objects
function error(diskStore, e)
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

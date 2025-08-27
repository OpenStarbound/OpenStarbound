
-- invoked when an item fails to instantiate, we can return a fallback item to create

-- we're catching any item error and converting them to the generic item format that the engine will try to re-instantiate later
function error(descriptor, e)
    if descriptor.name == "perfectlygenericitem" then
        sb.logError("Could not re-instantiate item '%s'. %s", sb.printJson(descriptor), e)
        return descriptor
    else
        sb.logError("Could not instantiate item '%s'. %s", sb.printJson(descriptor), e)
        local newDescriptor = {
            name = "perfectlygenericitem",
            parameters = {
                genericItemStorage = descriptor,
                shortdescription = descriptor.name,
                description = "Reinstall the parent mod to return this item to normal"
            }
        }
        return newDescriptor
    end
end

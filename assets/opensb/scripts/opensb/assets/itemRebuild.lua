
-- invoked when an item fails to instantiate, we must return a fallback item to create
function rebuild(descriptor, e)
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

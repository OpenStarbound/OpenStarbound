-- invoked when a Monster loaded from disk fails to deserialize properly
-- return null if script did not handle the error

function error(diskStore, e)

    return diskStore
end

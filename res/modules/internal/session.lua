local entries = {}
local this = {}

function this.get_entry(name)
    local entry = entries[name]
    if entry == nil then
        entry = {}
        entries[name] = entry
    end
    return entry
end

function this.reset_entry(name)
    entries[name] = nil
end

return this

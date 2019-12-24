local NOTCARE <const> = {}
local INDEX <const> = {1,2}

local function copytable(t)
    local r = {}
    for k, v in pairs(t) do
        r[k] = v
    end
    return r
end

local function compile_pattern(pattern)
    local res = {}
    for k, v in pairs(pattern) do
        res[#res+1] = k
        res[#res+1] = v
    end
    if #res == 0 then
        return true
    end
    return res
end

local function addmb(lookup, event_pattern, mb, pattern)
    event_pattern[mb] = copytable(pattern)
    for _, k in ipairs(INDEX) do
        local v = pattern[k]
        if not v then
            if lookup[NOTCARE] then
                lookup[NOTCARE][mb] = compile_pattern(pattern)
            else
                lookup[NOTCARE] = {[mb] = compile_pattern(pattern)}
            end
            return
        end
        pattern[k] = nil
        if not lookup[v] then
            lookup[v] = {}
        end
        lookup = lookup[v]
    end
    lookup[mb] = compile_pattern(pattern)
end

local function delmb(lookup, event_pattern, mb)
    local pattern = event_pattern[mb]
    for _, k in ipairs(INDEX) do
        local v = pattern[k]
        if not v then
            lookup[NOTCARE][mb] = nil
            return
        end
        lookup = lookup[v]
    end
    event_pattern[mb] = nil
    lookup[mb] = nil
end

local function filter(message, lst)
    local res = {}
    for mb, compiled in pairs(lst) do
        res[mb] = true
        if compiled ~= true then
            for i = 1, #compiled, 2 do
                local k, v = compiled[i], compiled[i+1]
                if message[k] ~= v then
                    res[mb] = nil
                    break
                end
            end
        end
    end
    return res
end

local function findmb(lookup, message)
    for _, k in ipairs(INDEX) do
        local v = message[k]
        if not v or not lookup[v] then
            return lookup[NOTCARE] and filter(message, lookup[NOTCARE]) or {}
        end
        lookup = lookup[v]
    end
    return lookup and filter(message, lookup) or {}
end

local mailbox = {}
mailbox.__index = mailbox

function mailbox:each()
    return function ()
        local msg = self[1]
        if msg then
            table.remove(self, 1)
            return msg
        end
    end
end

function mailbox:unpack()
    return function ()
        local msg = self[1]
        if msg then
            table.remove(self, 1)
            return table.unpack(msg)
        end
    end
end

local world = {}

function world:init()
    self._event_lookup = {}
    self._event_pattern = {}
end

function world:sub(pattern)
    local mb = setmetatable({}, mailbox)
    addmb(self._event_lookup, self._event_pattern, mb, pattern)
    return mb
end

function world:unsub(mb)
    delmb(self._event_lookup, self._event_pattern, mb)
end

function world:pub(message)
    for mb in pairs(findmb(self._event_lookup, message)) do
        mb[#mb+1] = message
    end
end

return world

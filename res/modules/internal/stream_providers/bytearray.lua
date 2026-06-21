local io_stream = require "core:io_stream"

local lib = { }

local buffers = { }
local positions = { }

local nextDescriptor = 0

local function openDescriptor(buffer)
    nextDescriptor = nextDescriptor + 1

    buffers[nextDescriptor] = buffer
    positions[nextDescriptor] = 1

    return nextDescriptor
end

local function requireDescriptor(descriptor)
    if not buffers[descriptor] then
        error("unknown descriptor")
    end
end

function lib.read(descriptor, length)
    requireDescriptor(descriptor)

    local buf = buffers[descriptor]
    local bufLength = #buf
    local pos = positions[descriptor]

    local toRead = math.min(bufLength - pos + 1, length)

    if toRead <= 0 then
        return Bytearray()
    end

    local segment = buf:slice(pos, toRead)

    positions[descriptor] = pos + toRead

    return segment
end

function lib.write(descriptor, data)
    requireDescriptor(descriptor)

    local buf = buffers[descriptor]
    local pos = positions[descriptor]

    local bufLength = #buf
    local dataLength = #data

    local endPos = pos + dataLength - 1

    -- size ensuring
    if endPos > bufLength then
        for i = bufLength + 1, endPos do
            buf[i] = 0
        end
    end

    for i = 1, dataLength do
        buf[i + pos - 1] = data[i]
    end

    positions[descriptor] = pos + dataLength
end

function lib.seek(descriptor, mode, offset)
    requireDescriptor(descriptor)

    local buf = buffers[descriptor]
    local bufLength = #buf

    local base

    if mode == 'b' then
        base = 1
    elseif mode == 'c' then
        base = positions[descriptor]
    elseif mode == 'e' then
        base = bufLength + 1
    else error('invalid seek mode') end

    local newPos = base + offset

    if newPos < 1 then
        error('failed to seek stream')
    end

    positions[descriptor] = newPos
end

function lib.tell(descriptor)
    requireDescriptor(descriptor)

    return positions[descriptor]
end

function lib.flush(descriptor)
    requireDescriptor(descriptor)
end

function lib.available(descriptor)
    requireDescriptor(descriptor)

    local buf = buffers[descriptor]
    local pos = positions[descriptor]

    return math.max(#buf - pos + 1, 0)
end

function lib.is_alive(descriptor)
    return buffers[descriptor] ~= nil
end

function lib.close(descriptor)
    requireDescriptor(descriptor)

    buffers[descriptor] = nil
    positions[descriptor] = nil
end

return function(buffer, binaryMode)
    if binaryMode == nil then
        binaryMode = true
    end

    return io_stream.new(
        openDescriptor(buffer),
        binaryMode,
        lib
    )
end
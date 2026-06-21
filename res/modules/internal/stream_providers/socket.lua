local io_stream = require "core:io_stream"

local lib = { }

local sockets = { }

local nextDescriptor = 0

local function openDescriptor(socket)
    nextDescriptor = nextDescriptor + 1

    sockets[nextDescriptor] = socket

    return nextDescriptor
end

local function requireDescriptor(descriptor)
    if not sockets[descriptor] then
        error("unknown descriptor")
    end
end

function lib.read(descriptor, length)
    requireDescriptor(descriptor)

    return sockets[descriptor]:recv(length)
end

function lib.write(descriptor, data)
    requireDescriptor(descriptor)

    sockets[descriptor]:send(data)
end

function lib.seek(descriptor, mode, offset)
    error("cannot seek socket")
end

function lib.tell(descriptor)
    error("cannot tell socket")
end

function lib.flush(descriptor)
    requireDescriptor(descriptor)
end

function lib.available(descriptor)
    requireDescriptor(descriptor)

    return sockets[descriptor]:available()
end

function lib.is_alive(descriptor)
    local socket = sockets[descriptor]

    return socket ~= nil and socket:is_alive()
end

function lib.close(descriptor)
    requireDescriptor(descriptor)

    sockets[descriptor]:close()
    sockets[descriptor] = nil
end

return function(socket, binaryMode)
    if binaryMode == nil then
        binaryMode = true
    end

    return io_stream.new(
        openDescriptor(socket),
        binaryMode,
        lib
    )
end
local arr = Bytearray()
assert(#arr == 0)

for i=1,10 do
    arr[i] = 10 - i
    assert(#arr == i)
    assert(arr[i] == 10 - i)
end

for i, v in ipairs(arr) do
    assert(v == 10 - i)
end

Bytearray.remove(arr, 2)
assert(#arr == 9)

Bytearray.insert(arr, {5, 3, 6})

assert(#arr == 12)
Bytearray.insert(arr, 2, 8)
assert(#arr == 13)
for i=1,10 do
    assert(arr[i] == 10 - i)
end
print(#arr, arr:get_capacity())
arr:trim()
assert(#arr == arr:get_capacity())

arr = Bytearray({0, 2, 7, 1, 16, 75, 25})
assert(arr[6] == 75)

arr:insert(2, {5, 6})

assert(arr[#arr] == 25)
assert(arr[2] == 5)
assert(arr[3] == 6)

-- =============================

local arr2 = Bytearray({254, 255, 255, 255, 255, 255, 255, 255})
--assert(U64view(arr2)[1] == 2 ^ 64 - 2)
assert(I64view(arr2)[1] == -2)
assert(U32view(arr2)[1] == 2 ^ 32 - 2)
assert(I32view(arr2)[1] == -2)
assert(U16view(arr2)[1] == 2 ^ 16 - 2)
assert(I16view(arr2)[1] == -2)
assert(I8view(arr2)[1] == -2)

assert(I8view(arr2)[2] == -1)
assert(I32view(arr2)[2] == -1)

local arr3 = Bytearray({0xFF, 0x7F})
local arr3i16 = I16view(arr3)
local arr3u16 = U16view(arr3)
assert(arr3i16[1] == 32767)

arr3u16[1] = arr3u16[1] + 1
assert(arr3i16[1] == -32768)
assert(arr3u16[1] == 32768)

local arr4 = Bytearray({0x9A, 0x99, 0x59, 0x40}) -- approx. equal 3.4 in hexadecimal representation of single-precision float number.
local arr4flt = FLTview(arr4)
local arr4u32 = U32view(arr4)
assert(arr4flt[1] == 3.4000000953674316)
assert(arr4u32[1] == 0x4059999A)

arr4flt[1] = arr4flt[1] + 0.1
assert(arr4flt[1] == 3.5)
assert(arr4u32[1] == 0x40600000) -- hexadecimal representation of 3.5 single-precision float number.

-- =============================================

function barrtostr(barr)
    local outstr = ""
    for i = 1, #barr do
        local lastfmt = ""
        if i ~= #barr then lastfmt = "|" end
        outstr = outstr .. barr[i] .. lastfmt
    end
    return outstr
end

local arr5 = Bytearray({0, 5, 34, 87, 21, 0, 210})
assert(barrtostr(arr5) == "0|5|34|87|21|0|210")
arr5:move(2, 3, 4)
assert(barrtostr(arr5) == "0|5|5|34|87|21|210")

local arr6 = Bytearray({1, 2, 3, 4, 6, 7, 8})
assert(barrtostr(arr6) == "1|2|3|4|6|7|8")
arr5:copy(4, arr6, 2, 3)
assert(barrtostr(arr6) == "1|34|87|21|6|7|8")

arr5:fill(nil, nil, 192)
assert(barrtostr(arr5) == "192|192|192|192|192|192|192")
arr5:fill(nil, 3, 233)
assert(barrtostr(arr5) == "233|233|233|233|233|233|233")
arr5:fill(2, nil, 254)
assert(barrtostr(arr5) == "254|254|254|254|254|254|254")
arr5:fill(2, 2, 66)
assert(barrtostr(arr5) == "254|66|66|254|254|254|254")
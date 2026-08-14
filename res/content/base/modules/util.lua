local util = {}

local DROP_FORCE = 8
local DROP_INIT_VEL = {0, 3, 0}
local DROP_MAX_ITEM_DIR_SHIFT_DEGREES = 65

local function calculate_item_drop_dir(pid)
    local yaw, pitch = player.get_rot(pid)
    local vp = gui.get_viewport()
    local mpos = table.map(input.get_mouse_pos(), function(i, v)
        return v / vp[i] - 0.5
    end)
    if not hud.is_inventory_open() and not hud.is_player_inventory_open() then
        mpos = {0, 0}
    end
    pitch = math.clamp(pitch - mpos[2]*DROP_MAX_ITEM_DIR_SHIFT_DEGREES*2, -90, 90)
    yaw = math.rad(yaw + 90 -mpos[1]*DROP_MAX_ITEM_DIR_SHIFT_DEGREES*2)
    pitch = math.rad(pitch)
    return vec3.normalize({
       math.cos(yaw) * math.cos(pitch),
       math.sin(pitch),
      -math.sin(yaw) * math.cos(pitch)
     })
end

---@param mode integer 0 - whole stack, 1 - single item, 2 - dupe whole stack
function util.drop_from_slot(invid, slot, mode)
    if invid == 0 then
        return
    end
    local pid = hud.get_player()
    if mode == 2 and not player.is_infinite_items(pid) then
        return
    end
    local itemid, itemcount = inventory.get(invid, slot)
    if itemid == 0 then
        return
    end

    local drop_itemcount = 1
    if mode == 0 then
        drop_itemcount = itemcount
    end
    if mode == 2 then
        drop_itemcount = item.stack_size(itemid)
    else
        inventory.set(invid, slot, itemid, itemcount - drop_itemcount)
    end

    local data = inventory.get_all_data(invid, slot)
    local pvel = { player.get_vel(pid) }
    local ppos = vec3.add({ player.get_pos(pid) }, { 0, 0.7, 0 })
    local dir = calculate_item_drop_dir(pid)
    local throw_force = vec3.mul(dir, DROP_FORCE)
    local drop = util.drop(ppos, itemid, drop_itemcount, data, 1.5)
    if not drop then
        return
    end
    local velocity = vec3.add(throw_force, vec3.add(pvel, DROP_INIT_VEL))
    drop.rigidbody:set_vel(velocity)
end

function util.drop(ppos, itemid, count, data, pickup_delay)
    if itemid == 0 or not itemid then
        return nil
    end
    return entities.spawn("base:drop", ppos, {base__drop={
        id=itemid,
        count=count,
        data=data,
        pickup_delay=pickup_delay
    }})
end

function util.calc_loot(loot_table)
    local results = {}
    for _, loot in ipairs(loot_table) do
        local chance = loot.chance or 1
        local count = loot.count or 1

        local roll = math.random()

        if roll < chance then
            if loot.min and loot.max then
                count = math.random(loot.min, loot.max)
            end
            if count == 0 then
                goto continue
            end
            table.insert(results, {item=item.index(loot.item), count=count})
        end
        ::continue::
    end
    return results
end

function util.block_loot(blockid)
    local lootscheme = block.properties[blockid]["base:loot"]
    if lootscheme then
        return util.calc_loot(lootscheme)
    end
    return {{item=block.get_picking_item(blockid), count=1}}
end

return util

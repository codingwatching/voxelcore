local base_util = require "util"

local DROP_FORCE = 8
local DROP_INIT_VEL = {0, 3, 0}

---@param mode integer 0 - whole stack, 1 - single item, 2 - dupe whole stack
function drop_from_slot(invid, slot, mode)
    local pid = hud.get_player()
    if mode == 2 and !player.is_infinite_items(pid) then
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
    local throw_force = vec3.mul(player.get_dir(pid), DROP_FORCE)
    local drop = base_util.drop(ppos, itemid, drop_itemcount, data, 1.5)
    if not drop then
        return
    end
    local velocity = vec3.add(throw_force, vec3.add(pvel, DROP_INIT_VEL))
    drop.rigidbody:set_vel(velocity)
end

function on_inventory_clicked_outside(exc_invid, exc_slot, mode)
    drop_from_slot(exc_invid, exc_slot, mode)
end

function on_hud_open()
    input.add_callback("player.drop", function()
        if hud.is_paused() or hud.is_inventory_open() then
            return
        end
        local pid = hud.get_player()
        local invid, slot = player.get_inventory(pid)
        local mode = 1
        if input.is_pressed("key:left-ctrl") or input.is_pressed("key:right-ctrl") then
          mode = 0
        end
        drop_from_slot(invid, slot, mode)
    end)
    rules.create("do-loot-non-player", true)
end

local show_weapon_collections = config.items.show_weapon_collections
local show_protector_collections = config.items.show_protector_collections
local show_equipped = config.items.show_equipped

local cjson = require('cjson')

local address_table = nil

local last_running = false
local progress_str = ''
local equip_address = 0
local inventory_address = 0
local inventory_size = 0
local storage_address = 0
local storage_size = 0

local categories = {}
local weapons = {}
local weapons_by_id = {}
local protectors = {}
local protectors_by_id = {}
local arrows = {}
local arrows_by_id = {}
local accessories = {}
local accessories_by_id = {}

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/weapons.json'))) do
  local index = #categories + 1
  categories[index] = v.category
  for _, v2 in ipairs(v.items) do
    local windex = #weapons + 1
    weapons[windex] = { id = v2.id, name = v2.name, category = index }
    weapons_by_id[v2.id] = windex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/protectors.json'))) do
  local pindex = #protectors + 1
  if type(v.id) == 'table' then
    protectors[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      protectors_by_id[v2] = pindex
    end
  else
    protectors[pindex] = { id = v.id, name = v.name }
    protectors_by_id[v.id] = pindex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/arrows.json'))) do
  local pindex = #arrows + 1
  if type(v.id) == 'table' then
    arrows[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      arrows_by_id[v2] = pindex
    end
  else
    arrows[pindex] = { id = v.id, name = v.name }
    arrows_by_id[v.id] = pindex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/accessories.json'))) do
  local pindex = #accessories + 1
  if type(v.id) == 'table' then
    accessories[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      accessories_by_id[v2] = pindex
    end
  else
    accessories[pindex] = { id = v.id, name = v.name }
    accessories_by_id[v.id] = pindex
  end
end

local function update_addresses()
  -- [[GameDataMan]+8]+418
  addr = process.read_u64(address_table.game_data_man_addr)
  if addr == 0 then return end
  addr = process.read_u64(addr + 8)
  if addr == 0 then return end
  equip_address = addr + 0x328

  if show_weapon_collections or show_protector_collections then
    inventory_address = process.read_u64(addr + 0x418)
    if inventory_address == 0 then
      inventory_size = 0
      return
    end
    inventory_size = process.read_u32(addr + 0x420)

    addr = process.read_u64(addr + 0x8B0)
    if addr == 0 then return end
    storage_address = process.read_u64(addr + 0x10)
    if storage_address == 0 then
      storage_size = 0
      return
    end
    storage_size = process.read_u32(addr + 0x18)
  end
end

local function get_weapon_name(id)
  if id < 0 then return '无' end
  local ridx = weapons_by_id[id // 10000 * 10000]
  if ridx == nil then return '无' end
  return weapons[ridx].name
end

local function get_protector_name(id)
  if id < 0 then return '无' end
  local ridx = protectors_by_id[id]
  if ridx == nil then return '无' end
  return protectors[ridx].name
end

local function get_arrow_name(id)
  if id < 0 then return '无' end
  local ridx = arrows_by_id[id]
  if ridx == nil then return '无' end
  return arrows[ridx].name
end

local function get_accessory_name(id)
  if id < 0 then return '无' end
  local ridx = accessories_by_id[id]
  if ridx == nil then return '无' end
  return accessories[ridx].name
end

local function calculate_items_count(wcount, pcount, addr_start, item_count, max_count)
  local addr = addr_start + 4
  local total = 0
  for idx = 0, max_count-1 do
    local id = process.read_u32(addr)
    if id ~= 0 and id ~= 0xFFFFFFFF then
      local type = id >> 28
      id = id & 0xFFFFFFF
      if type == 0 then
        if wcount ~= nil then
          local ridx = weapons_by_id[id // 10000 * 10000]
          if ridx ~= nil then
            wcount[ridx] = (wcount[ridx] or 0) + 1
          end
        end
      elseif type == 1 then
        if pcount ~= nil then
          local ridx = protectors_by_id[id]
          if ridx ~= nil then
            pcount[ridx] = (pcount[ridx] or 0) + 1
          end
        end
      end
      total = total + 1
      if total >= item_count then
        break
      end
    end
    addr = addr + 0x14
  end
end

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      f = io.open(config.output_folder .. 'items.txt', 'w')
      f:close()
    end
    return
  end
  if not last_running then
    last_running = true
    address_table = process.get_address_table()
  end
  update_addresses()
  local progstr = ''
  if show_equipped then
    local arm_style = process.read_u32(equip_address)
    local lhand_slot = process.read_u32(equip_address + 0x04) + 1
    local rhand_slot = process.read_u32(equip_address + 0x08) + 1

    local lw = {
      get_weapon_name(process.read_u32(equip_address + 0x74)),
      get_weapon_name(process.read_u32(equip_address + 0x7C)),
      get_weapon_name(process.read_u32(equip_address + 0x84))
    }
    local rw = {
      get_weapon_name(process.read_u32(equip_address + 0x78)),
      get_weapon_name(process.read_u32(equip_address + 0x80)),
      get_weapon_name(process.read_u32(equip_address + 0x88))
    }

--[[
    local arrow = {
      get_arrow_name(process.read_u32(equip_address + 0x8C)),
      get_arrow_name(process.read_u32(equip_address + 0x94))
    }
    local bolt = {
      get_arrow_name(process.read_u32(equip_address + 0x90)),
      get_arrow_name(process.read_u32(equip_address + 0x98))
    }
]]
    local helmet = get_protector_name(process.read_u32(equip_address + 0xA4))
    local armor = get_protector_name(process.read_u32(equip_address + 0xA8))
    local gauntlet = get_protector_name(process.read_u32(equip_address + 0xAC))
    local leggings = get_protector_name(process.read_u32(equip_address + 0xB0))

    local acc_1 = get_accessory_name(process.read_u32(equip_address + 0xB8))
    local acc_2 = get_accessory_name(process.read_u32(equip_address + 0xBC))
    local acc_3 = get_accessory_name(process.read_u32(equip_address + 0xC0))
    local acc_4 = get_accessory_name(process.read_u32(equip_address + 0xC4))

    if lhand_slot > 0 and lhand_slot < 4 then
      if arm_style == 2 then
        lw[lhand_slot] = '[[' .. lw[lhand_slot] .. ']]'
      else
        lw[lhand_slot] = '[' .. lw[lhand_slot] .. ']'
      end
    end
    if rhand_slot > 0 and rhand_slot < 4 then
      if arm_style == 3 then
        rw[rhand_slot] = '[[' .. rw[rhand_slot] .. ']]'
      else
        rw[rhand_slot] = '[' .. rw[rhand_slot] .. ']'
      end
    end

    progstr = string.format('右手:\n　%s\n　%s\n　%s\n左手:\n　%s\n　%s\n　%s\n防具:\n　%s\n　%s\n　%s\n　%s\n护符:\n　%s\n　%s\n　%s\n　%s', rw[1], rw[2], rw[3], lw[1], lw[2], lw[3], helmet, armor, gauntlet, leggings, acc_1, acc_2, acc_3, acc_4)
  end
  if show_weapon_collections or show_protector_collections then
    local wcount = nil
    local pcount = nil
    if show_weapon_collections then
      wcount = {}
    end
    if show_protector_collections then
      pcount = {}
    end
    if inventory_address ~= 0 then
      calculate_items_count(wcount, pcount, inventory_address, inventory_size, 2688)
    end
    if storage_address ~= 0 then
      calculate_items_count(wcount, pcount, storage_address, storage_size, 1920)
    end
    local weapon_count = 0
    local protector_count = 0
    if show_weapon_collections then
      for k, v in pairs(wcount) do
        weapon_count = weapon_count + 1
      end
    end
    if show_protector_collections then
      for k, v in pairs(pcount) do
        protector_count = protector_count + 1
      end
    end
    if show_weapon_collections then
      local wstr = ''
      local show_count = 0
      for i, v in ipairs(weapons) do
        if wcount[i] == nil then
          wstr = wstr .. string.format('\n　%s', v.name)
          show_count = show_count + 1
          if show_count == 10 then break end
        end
      end
      if #progstr > 0 then
        progstr = progstr .. '\n'
      end
      if #wstr > 0 then
        if weapon_count + 10 < #weapons then
          progstr = progstr .. string.format('武器收集进度: %d/%d\n未收集武器(仅显示10个):', weapon_count, #weapons) .. wstr
        else
          progstr = progstr .. string.format('武器收集进度: %d/%d\n未收集武器:', weapon_count, #weapons) .. wstr
        end
      else
        progstr = progstr .. string.format('武器收集进度: %d/%d', weapon_count, #weapons)
      end
    end
    if show_protector_collections then
      local pstr = ''
      show_count = 0
      for i, v in ipairs(protectors) do
        if pcount[i] == nil then
          pstr = pstr .. string.format('\n　%s', v.name)
          show_count = show_count + 1
          if show_count == 10 then break end
        end
      end
      if #progstr > 0 then
        progstr = progstr .. '\n'
      end
      if #pstr > 0 then
        if protector_count + 10 < #protectors then
          progstr = progstr .. string.format('防具收集进度: %d/%d\n未收集防具(仅显示10个):', protector_count, #protectors) .. pstr
        else
          progstr = progstr .. string.format('防具收集进度: %d/%d\n未收集防具:', protector_count, #protectors) .. pstr
        end
      else
        progstr = progstr .. string.format('防具收集进度: %d/%d', protector_count, #protectors)
      end
    end
  end

  if progstr ~= progress_str then
    progress_str = progstr
    f = io.open(config.output_folder .. 'items.txt', 'w')
    f:write(progstr)
    f:close()
  end
end

return {
  update = update
}

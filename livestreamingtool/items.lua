local show_weapons_collections = config.items.show_weapons_collections
local show_protectors_collections = config.items.show_protectors_collections
local show_accessories_collections = config.items.show_accessories_collections
local show_sorceries_collections = config.items.show_sorceries_collections
local show_incantations_collections = config.items.show_incantations_collections
local show_ashesofwar_collections = config.items.show_ashesofwar_collections
local show_spirits_collections = config.items.show_spirits_collections
local any_collections = show_weapons_collections or show_protectors_collections or show_accessories_collections or show_sorceries_collections or show_incantations_collections or show_ashesofwar_collections or show_spirits_collections
local show_equipped = config.items.show_equipped
local check_dlc_items = config.items.check_dlc_items

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
local accessories = {}
local accessories_by_id = {}
local sorceries = {}
local sorceries_by_id = {}
local incantations = {}
local incantations_by_id = {}
local ashesofwar = {}
local ashesofwar_by_id = {}
local spirits = {}
local spirits_by_id = {}

local check_list = {{},{},{},{},{},{},{},{},{},{}}
if show_weapons_collections then
  check_list[1][#check_list[1] + 1] = {{}, function(id) return weapons_by_id[id // 10000 * 10000] end, weapons, '武器'}
end
if show_protectors_collections then
  check_list[2][#check_list[2] + 1] = {{}, function(id) return protectors_by_id[id] end, protectors, '防具'}
end
if show_accessories_collections then
  check_list[3][#check_list[3] + 1] = {{}, function(id) return accessories_by_id[id] end, accessories, '护符'}
end
if show_sorceries_collections then
  check_list[5][#check_list[5] + 1] = {{}, function(id) return sorceries_by_id[id] end, sorceries, '魔法'}
end
if show_incantations_collections then
  check_list[5][#check_list[5] + 1] = {{}, function(id) return incantations_by_id[id] end, incantations, '祷告'}
end
if show_ashesofwar_collections then
  check_list[9][#check_list[9] + 1] = {{}, function(id) return ashesofwar_by_id[id] end, ashesofwar, '战灰'}
end
if show_spirits_collections then
  check_list[5][#check_list[5] + 1] = {{}, function(id) return spirits_by_id[id] end, spirits, '骨灰'}
end

local suffix
if check_dlc_items then
  suffix = '_dlc1.json'
else
  suffix = '.json'
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/weapons'..suffix))) do
  local index = #categories + 1
  categories[index] = v.category
  for _, v2 in ipairs(v.items) do
    local windex = #weapons + 1
    weapons[windex] = { id = v2.id, name = v2.name, category = index }
    weapons_by_id[v2.id] = windex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/protectors'..suffix))) do
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

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/accessories'..suffix))) do
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

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/sorceries'..suffix))) do
  local pindex = #sorceries + 1
  if type(v.id) == 'table' then
    sorceries[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      sorceries_by_id[v2] = pindex
    end
  else
    sorceries[pindex] = { id = v.id, name = v.name }
    sorceries_by_id[v.id] = pindex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/incantations'..suffix))) do
  local pindex = #incantations + 1
  if type(v.id) == 'table' then
    incantations[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      incantations_by_id[v2] = pindex
    end
  else
    incantations[pindex] = { id = v.id, name = v.name }
    incantations_by_id[v.id] = pindex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/ashesofwar'..suffix))) do
  local pindex = #ashesofwar + 1
  if type(v.id) == 'table' then
    ashesofwar[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      ashesofwar_by_id[v2] = pindex
    end
  else
    ashesofwar[pindex] = { id = v.id, name = v.name }
    ashesofwar_by_id[v.id] = pindex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/spirits'..suffix))) do
  local pindex = #spirits + 1
  if type(v.id) == 'table' then
    spirits[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      spirits_by_id[v2] = pindex
    end
  else
    spirits[pindex] = { id = v.id, name = v.name }
    spirits_by_id[v.id] = pindex
  end
end

local function update_addresses()
  -- [[GameDataMan]+8]+418
  addr = process.read_u64(address_table.game_data_man_addr)
  if addr == 0 then return end
  addr = process.read_u64(addr + 8)
  if addr == 0 then return end
  equip_address = addr + 0x328

  if any_collections then
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

local function get_accessory_name(id)
  if id < 0 then return '无' end
  local ridx = accessories_by_id[id]
  if ridx == nil then return '无' end
  return accessories[ridx].name
end

local function calculate_items_count(addr_start, item_count, max_count)
  local addr = addr_start + 4
  local total = 0
  for idx = 0, max_count-1 do
    local id = process.read_u32(addr)
    if id ~= 0 and id ~= -1 then
      local type = (id >> 28) & 0x0F
      id = id & 0xFFFFFFF
      if type < 9 then
        for _, v in ipairs(check_list[type + 1]) do
          local ridx = v[2](id)
          if ridx ~= nil then
            v[1][ridx] = (v[1][ridx] or 0) + 1
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
  if any_collections then
    for _, v in ipairs(check_list) do
      for _, v2 in ipairs(v) do
        v2[1] = {}
      end
    end
    if inventory_address ~= 0 then
      calculate_items_count(inventory_address, inventory_size, 2688)
    end
    if storage_address ~= 0 then
      calculate_items_count(storage_address, storage_size, 1920)
    end
    for _, v in ipairs(check_list) do
      for _, v2 in ipairs(v) do
        local cnt = 0
        for _, _ in pairs(v2[1]) do
          cnt = cnt + 1
        end
        local str = ''
        local show_count = 0
        for i, v3 in ipairs(v2[3]) do
          if v2[1][i] == nil then
            str = str .. string.format('\n　%s', v3.name)
            show_count = show_count + 1
            if show_count == 10 then break end
          end
        end
        if #progstr > 0 then
          progstr = progstr .. '\n'
        end
        if #str > 0 then
          if cnt + 10 < #v2[3] then
            progstr = progstr .. string.format('%s收集进度: %d/%d\n未收集%s(显示前10个):', v2[4], cnt, #v2[3], v2[4]) .. str
          else
            progstr = progstr .. string.format('%s收集进度: %d/%d\n未收集%s:', v2[4], cnt, #v2[3], v2[4]) .. str
          end
        else
          progstr = progstr .. string.format('%s收集进度: %d/%d', v2[4], cnt, #v2[3])
        end
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

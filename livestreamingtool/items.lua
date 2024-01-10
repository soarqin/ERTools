local cjson = require('cjson')

local address_table = nil

local last_running = false
local progress_str = ''
local inventory_address = 0
local inventory_size = 0
local storage_address = 0
local storage_size = 0

local categories = {}
local weapons = {}
local weapons_by_id = {}
local protectors = {}
local protectors_by_id = {}

for _, v in ipairs(cjson.decode(util.read_file('data/weapons.json'))) do
  local index = #categories + 1
  categories[index] = v.category
  for _, v2 in ipairs(v.items) do
    local windex = #weapons + 1
    weapons[windex] = { id = v2.id, name = v2.name, category = index }
    weapons_by_id[v2.id] = windex
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/protectors.json'))) do
  local pindex = #protectors + 1
  if type(v.id) == "table" then
    protectors[pindex] = { id = v.id[1], name = v.name }
    for _, v2 in ipairs(v.id) do
      protectors_by_id[v2] = pindex
    end
  else
    protectors[pindex] = { id = v.id, name = v.name }
    protectors_by_id[v.id] = pindex
  end
end

local function update_inventory_address()
  -- [[GameDataMan]+8]+418
  local addrstr = process.read_memory(address_table.game_data_man_addr, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 8, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0x420, 8)
  if addrstr == nil then return nil end
  local sz1, sz2 = string.unpack('=I4I4', addrstr)
  inventory_size = sz1 + sz2
  addrstr = process.read_memory(addr + 0x418, 8)
  if addrstr == nil then
    inventory_size = 0
    return nil
  end
  inventory_address = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0x8B0, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)

  addrstr = process.read_memory(addr + 0x18, 4)
  if addrstr == nil then return nil end
  storage_size = string.unpack('=I4', addrstr)
  addrstr = process.read_memory(addr + 0x10, 8)
  if addrstr == nil then
    storage_size = 0
    return nil
  end
  storage_address = string.unpack('=I8', addrstr)
end

local function calculate_items_count(wcount, pcount, addr_start, item_count)
  local addr = addr_start + 4
  for idx = 0, item_count-1 do
    local addrstr = process.read_memory(addr, 4)
    if addrstr == nil then break end
    local id = string.unpack('=I4', addrstr)
    local type = id >> 28
    id = id & 0xFFFFFFF
    if type == 0 then
      local ridx = weapons_by_id[id // 10000 * 10000]
      if ridx ~= nil then
        wcount[ridx] = (wcount[ridx] or 0) + 1
      end
    elseif type == 1 then
      local ridx = protectors_by_id[id]
      if ridx ~= nil then
        pcount[ridx] = (pcount[ridx] or 0) + 1
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
  update_inventory_address()
  local pcount = {}
  local wcount = {}
  if inventory_address ~= 0 then
    calculate_items_count(wcount, pcount, inventory_address, inventory_size)
  end
  if storage_address ~= 0 then
    calculate_items_count(wcount, pcount, storage_address, storage_size)
  end
  local weapon_count = 0
  local protector_count = 0
  for k, v in pairs(wcount) do
    weapon_count = weapon_count + 1
  end
  for k, v in pairs(pcount) do
    protector_count = protector_count + 1
  end
  local wstr = ''
  local show_count = 0
  for i, v in ipairs(weapons) do
    if wcount[i] == nil then
      wstr = wstr .. string.format("\n　%s", v.name)
      show_count = show_count + 1
      if show_count == 10 then break end
    end
  end
  if #wstr > 0 then
    wstr = string.format('武器收集进度: %d/%d\n未收集武器(仅显示10个):', weapon_count, #weapons) .. wstr
  else
    wstr = string.format('武器收集进度: %d/%d', weapon_count, #weapons)
  end
  local pstr = ''
  show_count = 0
  for i, v in ipairs(protectors) do
    if pcount[i] == nil then
      pstr = pstr .. string.format("\n　%s", v.name)
      show_count = show_count + 1
      if show_count == 10 then break end
    end
  end
  if #pstr > 0 then
    wstr = wstr .. string.format('\n防具收集进度: %d/%d\n未收集防具(仅显示10个):', protector_count, #protectors) .. pstr
  else
    wstr = wstr .. string.format('\n防具收集进度: %d/%d', protector_count, #protectors)
  end

  if wstr ~= progress_str then
    progress_str = wstr
    f = io.open(config.output_folder .. 'items.txt', 'w')
    f:write(wstr)
    f:close()
  end
--  print(string.format("%X %d", inventory_address, inventory_size))
--  print(string.format("%X %d", storage_address, storage_size))
end

return {
  update = update
}

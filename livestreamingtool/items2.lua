local category_name = config.items2.category

local cjson = require('cjson')

local address_table = nil
local game_version = 0

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

local check_list = {{},{},{},{},{},{},{},{},{},{}}
check_list[1][#check_list[1] + 1] = {{}, function(id) return weapons_by_id[id // 10000 * 10000] end, weapons, '武器'}

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/weapons.json'))) do
  if category_name == v.category then
    local index = #categories + 1
    categories[index] = v.category
    for _, v2 in ipairs(v.items) do
      local windex = #weapons + 1
      weapons[windex] = { id = v2.id, name = v2.name, category = index }
      weapons_by_id[v2.id] = windex
    end
  end
end

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/weapons_dlc1.json'))) do
  if category_name == v.category then
    local index = #categories + 1
    categories[index] = 'DLC'
    for _, v2 in ipairs(v.items) do
      local windex = #weapons + 1
      weapons[windex] = { id = v2.id, name = v2.name, category = index }
      weapons_by_id[v2.id] = windex
    end
  end
end

local function update_addresses()
  -- [[GameDataMan]+8]+418
  addr = process.read_u64(address_table.game_data_man_addr)
  if addr == 0 then return end
  addr = process.read_u64(addr + 8)
  if addr == 0 then return end
  equip_address = addr + 0x328

  if game_version < 0x20200 then
    addr = process.read_u64(addr + 0x5B8)
  else
    addr = process.read_u64(addr + 0x5D0)
  end
  if addr ~= 0 then
    inventory_address = process.read_u64(addr + 0x10)
    if inventory_address == 0 then
      inventory_size = 0
      return
    end
    inventory_size = process.read_u32(addr + 0x18)
  end

  if game_version < 0x10200 then
    addr = process.read_u64(addr + 0x8B0)
  else
    addr = process.read_u64(addr + 0x8D0)
  end
  if addr == 0 then return end
  storage_address = process.read_u64(addr + 0x10)
  if storage_address == 0 then
    storage_size = 0
    return
  end
  storage_size = process.read_u32(addr + 0x18)
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
  local step
  if game_version < 0x20200 then
    step = 0x14
  else
    step = 0x18
  end
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
    addr = addr + step
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
    game_version = process.game_version()
  end
  update_addresses()
  local progstr = ''
  for _, v in ipairs(check_list) do
    for _, v2 in ipairs(v) do
      v2[1] = {}
    end
  end
  if inventory_address ~= nil and inventory_address ~= 0 then
    calculate_items_count(inventory_address, inventory_size, 2688)
  end
  if storage_address ~= nil and storage_address ~= 0 then
    calculate_items_count(storage_address, storage_size, 1920)
  end
  for _, v in ipairs(check_list) do
    for _, v2 in ipairs(v) do
      local last_cate = 0
      for i, v3 in ipairs(v2[3]) do
        if v3.category ~= last_cate then
          last_cate = v3.category
          progstr = progstr .. '==' .. categories[last_cate] .. '==\n'
        end
        if v2[1][i] == nil then
          progstr = progstr .. string.format('☐ %s\n', v3.name)
        else
          progstr = progstr .. string.format('☑ %s\n', v3.name)
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

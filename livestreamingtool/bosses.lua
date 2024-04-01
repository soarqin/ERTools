local cjson = require('cjson')
local only_rememberance = config.bosses.only_rememberance

-- addresses
local address_table = nil
local event_flag_address = 0

-- boss data
local regions = {}
local bosses = {}
local rememberance_bosses = {}
local rememberance_boss_count = 0
local region_name = {}
local boss_count = 0

for _, v in ipairs(cjson.decode(util.read_file('data/'..config.language..'/bosses.json'))) do
  local index = #bosses + 1
  for _, v2 in ipairs(v.regions) do
    regions[v2] = index
  end
  bosses[index] = v.bosses
  region_name[index] = v['region_name']
end
for _, v in pairs(bosses) do
  boss_count = boss_count + #v
  for _, v2 in pairs(v) do
    local rem = v2.rememberance
    if rem ~= nil and rem > 0 then
      rememberance_bosses[rem] = v2
      rememberance_boss_count = rememberance_boss_count + 1
    end
  end
end

local last_count = 0
local last_region = 0
local last_running = false

local function calc_flag_offset_and_bit(flag_id)
  local addr = process.read_u64(address_table.event_flag_man_addr)
  if addr == 0 then return 0,1 end
  local divisor = process.read_u32(addr + 0x1C)
  local category = flag_id // divisor
  local least_significant_digits = flag_id - category * divisor
  local current_element = process.read_u64(addr + 0x38)
  local current_sub_element = process.read_u64(current_element + 0x08)
  while process.read_u8(current_sub_element + 0x19) == 0 do
    if process.read_u32(current_sub_element + 0x20) < category then
      current_sub_element = process.read_u64(current_sub_element + 0x10)
    else
      current_element = current_sub_element
      current_sub_element = process.read_u64(current_sub_element)
    end
  end
  if current_element == current_sub_element then
    return 0,1
  end
  local mystery_value = process.read_u32(current_element + 0x28) - 1
  local calculated_pointer = 0;
  if mystery_value ~= 0 then
    if mystery_value ~= 1 then
      calculated_pointer = process.read_u64(current_element + 0x30);
    else
      return 0,1
    end
  else
    calculated_pointer = process.read_u32(addr + 0x20) * process.read_u32(current_element + 0x30) + process.read_u64(addr + 0x28)
  end

  if calculated_pointer == 0 then
    return 0,1
  end
  local thing = 7 - (least_significant_digits & 7)
  local another_thing = 1 << thing
  local shifted = least_significant_digits >> 3

  return calculated_pointer + shifted - event_flag_address, another_thing
end

local first = true
local function calculate_all_flag_offsets()
  if not first then
    return
  end
  first = false
  for _, v in pairs(bosses) do
    for _, v2 in pairs(v) do
      v2.offset, v2.bit = calc_flag_offset_and_bit(v2.flag_id)
      if v2.bit == nil then
        print(v2.boss, v2.flag_id, v2.offset, v2.bit)
      end
    end
  end
end

local function update_memory_address()
  local addr = process.read_u64(address_table.event_flag_man_addr)
  if addr == 0 then return end
  event_flag_address = process.read_u64(addr + 0x28)
  calculate_all_flag_offsets()
end

local function read_flag(offset)
  if event_flag_address == 0 then
    return 0
  end
  return process.read_u8(event_flag_address + offset)
end

local function get_map_area()
  local addr = process.read_u64(address_table.field_area_addr)
  if addr == 0 then return 0 end
  return process.read_u32(addr + 0xE4)
end

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      last_count = 0
      event_flag_address = 0
      f = io.open(config.output_folder .. 'bosses.txt', 'w')
      f:close()
    end
    return
  end
  if not last_running then
    last_running = true
    last_count = -1
    address_table = process.get_address_table()
  end
  update_memory_address()
  local count = 0
  if only_rememberance then
    cbosses = {}
    for i, v in ipairs(rememberance_bosses) do
      if (read_flag(v.offset) & v.bit) > 0 then
        count = count + 1
        cbosses[#cbosses + 1] = {'☑', v.boss}
      else
        cbosses[#cbosses + 1] = {'☐', v.boss}
      end
    end
    if count == last_count then return end
    last_count = count
    f = io.open(config.output_folder .. 'bosses.txt', 'w')
    if f == nil then
      return
    end
    f:write(string.format('追忆Boss: %d/%d\n', count, rememberance_boss_count))
    for _, v in pairs(cbosses) do
      f:write(string.format('%s %s\n', v[1], v[2]))
    end
  else
    local rcount = 0
    local region_bosses = {}
    local r = regions[get_map_area() // 1000]
    for i, v in ipairs(bosses) do
      local is_current = i == r
      for _, v2 in pairs(v) do
        if (read_flag(v2.offset) & v2.bit) > 0 then
          count = count + 1
          if is_current then
            rcount = rcount + 1
            region_bosses[#region_bosses + 1] = {'☑', v2.boss, v2.place}
          end
        else
          if is_current then
            region_bosses[#region_bosses + 1] = {'☐', v2.boss, v2.place}
          end
        end
      end
    end
    if count == last_count and r == last_region then return end
    last_count = count
    last_region = r
    f = io.open(config.output_folder .. 'bosses.txt', 'w')
    if f == nil then
      return
    end
    f:write(string.format('全Boss: %d/%d\n', count, boss_count))
    if r ~= nil then
      f:write(string.format('%sBoss: %d/%d\n', region_name[r], rcount, #bosses[r]))
      for _, v in pairs(region_bosses) do
        if #v[3] > 0 then
          f:write(string.format('%s %s - %s\n', v[1], v[2], v[3]))
        else
          f:write(string.format('%s %s\n', v[1], v[2]))
        end
      end
    end
  end
  f:close()
end

return {
  update = update
}

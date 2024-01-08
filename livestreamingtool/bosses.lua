local cjson = require('cjson')

-- Aux functions
local function read_file(path)
    local file = io.open(path, 'rb')
    if not file then return nil end
    local content = file:read('*a')
    file:close()
    return content
end

-- addresses
local address_table = nil
local event_flag_address = 0

-- boss data
local regions = {}
local bosses = {}
local region_name = {}
local boss_count = 0

for k, v in pairs(cjson.decode(read_file('data/bosses.json'))) do
  local index = #bosses + 1
  for _, v2 in pairs(v.regions) do
    regions[v2] = index
  end
  bosses[index] = v.bosses
  region_name[index] = k
end
for _, v in pairs(bosses) do
  for _, v2 in pairs(v) do
    boss_count = boss_count + 1
    v2.offset = tonumber(string.sub(v2.offset, 3), 16)
    v2.bit = 1 << tonumber(v2.bit)
  end
end

local last_count = 0
local last_region = 0
local last_running = false

local function update_memory_address()
  local addrstr = process.read_memory(address_table.event_flag_man_addr, 8)
  if addrstr == nil then return end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0x28, 8)
  if addrstr == nil then return end
  event_flag_address = string.unpack('=I8', addrstr)
end


local function read_flag(offset)
  if event_flag_address == 0 then
    return 0
  end
  local str = process.read_memory(event_flag_address + offset, 1)
  if str == nil then return 0 end
  return string.byte(str)
end

local function get_map_area()
  local addrstr = process.read_memory(address_table.field_area_addr, 8)
  if addrstr == nil then return 0 end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0xE4, 4)
  if addrstr == nil then return 0 end
  local area, _ = string.unpack('=I4', addrstr)
  return area
end

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      event_flag_address = 0
      f = io.open(config.output_folder .. 'bosses.txt', 'w')
      f:close()
    end
    return
  end
  if not last_running then
    last_running = true
    address_table = process.get_address_table()
  end
  update_memory_address()
  local count = 0
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
  f:close()
end

return {
  update = update
}

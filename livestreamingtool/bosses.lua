-- begin of configs
--    Only display rememberance bosses
local only_rememberance = true
-- end of configs

local cjson = require('cjson')

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

for k, v in pairs(cjson.decode(util.read_file('data/bosses.json'))) do
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

local function update_memory_address()
  local addr = process.read_u64(address_table.event_flag_man_addr)
  if addr == 0 then return end
  event_flag_address = process.read_u64(addr + 0x28)
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

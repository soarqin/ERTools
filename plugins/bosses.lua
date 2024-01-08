local cjson = require('cjson')

local function read_file(path)
    local file = io.open(path, 'rb')
    if not file then return nil end
    local content = file:read('*a')
    file:close()
    return content
end

local regions = {}
local bosses = {}
local region_name = {}

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
    v2.offset = tonumber(string.sub(v2.offset, 3), 16)
    v2.bit = 1 << tonumber(v2.bit)
  end
end

local last_count = 0
local last_region = 0
local last_running = false

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      f = io.open("bosses.txt", "w")
      f:close()
    end
    return
  end
  last_running = true
  local count = 0
  local rcount = 0
  local region_bosses = {}
  local r = regions[process.get_map_area() // 1000]
  for i, v in ipairs(bosses) do
    local is_current = i == r
    for _, v2 in pairs(v) do
      if (process.read_flag(v2.offset) & v2.bit) > 0 then
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
  f = io.open("bosses.txt", "w")
  if f == nil then
    return
  end
  f:write(string.format('全Boss: %d/165\n', count))
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

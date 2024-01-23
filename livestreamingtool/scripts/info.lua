-- addresses
local address_table = nil

local last_attr = ''
local last_running = false

local function get_player_attr()
  local addr = process.read_u64(address_table.game_data_man_addr)
  if addr == 0 then return nil end
  deathnum = process.read_u32(addr + 0x94)
  igt = process.read_u32(addr + 0xA0)
  rounds = process.read_u32(addr + 0x120)
  addr = process.read_u64(addr + 8)
  if addr == 0 then return nil end
  addrstr = process.read_memory(addr + 0x3C, 0x38)
  if addrstr == nil then return nil end
  res = {string.unpack('=I4I4I4I4I4I4I4I4I4I4I4I4I4I4', addrstr)}
  res[9] = igt
  res[10] = rounds + 1
  res[11] = deathnum
  return res
end

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      last_attr = ''
      panel_begin("Info")
      panel_end()
    end
    return
  end
  if not last_running then
    last_running = true
    address_table = process.get_address_table()
  end
  local attr = get_player_attr()
  if attr == nil then
    if last_attr ~= nil then
      last_attr = nil
    end
    return
  end
  attr_str = string.format('游戏时间 %02d:%02d\n第%d周目\n卢恩 %d\n累计 %d\n死亡 %d\n等级 %d\n生命 %d\n集中 %d\n耐力 %d\n力气 %d\n灵巧 %d\n智力 %d\n信仰 %d\n感应 %d', attr[9] // 3600000, (attr[9] // 60000) % 60, attr[10], attr[13], attr[14], attr[11], attr[12], attr[1], attr[2], attr[3], attr[4], attr[5], attr[6], attr[7], attr[8])
  if last_attr == attr_str then return end
  last_attr = attr_str
  panel_begin("Info")
  panel_output(attr_str)
  panel_end()
end

panel_create("Info")

return {
  update = update
}

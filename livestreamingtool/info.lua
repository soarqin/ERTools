-- addresses
local address_table = nil

local last_attr = ''
local last_running = false

local function get_player_attr()
  local addrstr = process.read_memory(address_table.game_data_man_addr, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0xA0, 4)
  if addrstr == nil then return nil end
  igt = string.unpack('=I4', addrstr)
  addrstr = process.read_memory(addr + 0x120, 4)
  if addrstr == nil then return nil end
  rounds = string.unpack('=I4', addrstr)
  addrstr = process.read_memory(addr + 8, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process.read_memory(addr + 0x3C, 0x38)
  if addrstr == nil then return nil end
  res = {string.unpack('=I4I4I4I4I4I4I4I4I4I4I4I4I4I4', addrstr)}
  res[9] = igt
  res[10] = rounds + 1
  return res
end

local function update()
  if not process.game_running() then
    if last_running then
      last_running = false
      f = io.open("info.txt", "w")
      f:close()
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
  attr_str = string.format('游戏时间 %02d:%02d\n　周目数 %d\n持有卢恩 %d\n累计获得 %d\n　　等级 %d\n　　生命 %d\n　　集中 %d\n　　耐力 %d\n　　力气 %d\n　　灵巧 %d\n　　智力 %d\n　　信仰 %d\n　　感应 %d', attr[9] // 3600000, (attr[9] // 60000) % 60, attr[10], attr[13], attr[14], attr[12], attr[1], attr[2], attr[3], attr[4], attr[5], attr[6], attr[7], attr[8])
  if last_attr == attr_str then return end
  last_attr = attr_str
  f = io.open("info.txt", "w")
  f:write(attr_str)
  f:close()
end

return {
  update = update
}

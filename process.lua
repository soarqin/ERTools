local memreader = require('memreader')
local process = nil
local event_flag_man_addr = 0
local field_area_addr = 0
local game_data_man_addr = 0
local event_flag_address = 0

local function get_offset_table()
  local ver = process:version().file
  print('  > Version: ' .. ver.major .. '.' .. ver.minor .. '.' .. ver.build)
  if ver.major == 1 and ver.minor == 2 and ver.build == 0 then
    event_flag_man_addr = 0x3c526e8
    field_area_addr = 0x3c53470
    game_data_man_addr = 0x3c481b8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 1 then
    event_flag_man_addr = 0x3c52708
    field_area_addr = 0x3c53490
    game_data_man_addr = 0x3c481d8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 2 then
    event_flag_man_addr = 0x3c52728
    field_area_addr = 0x3c534b0
    game_data_man_addr = 0x3c481f8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 3 then
    event_flag_man_addr = 0x3c55748
    field_area_addr = 0x3c564d0
    game_data_man_addr = 0x3c4b218
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 0 then
    event_flag_man_addr = 0x3c672a8
    field_area_addr = 0x3c68040
    game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 1 then
    event_flag_man_addr = 0x3c672a8
    field_area_addr = 0x3c68040
    game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 2 then
    event_flag_man_addr = 0x3c672a8
    field_area_addr = 0x3c68040
    game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 4 and ver.build == 0 then
    event_flag_man_addr = 0x3c0a538
    field_area_addr = 0x3c0b2c0
    game_data_man_addr = 0x3c00028
  elseif ver.major == 1 and ver.minor == 4 and ver.build == 1 then
    event_flag_man_addr = 0x3c0a538
    field_area_addr = 0x3c0b2c0
    game_data_man_addr = 0x3c00028
  elseif ver.major == 1 and ver.minor == 5 and ver.build == 0 then
    event_flag_man_addr = 0x3c222e8
    field_area_addr = 0x3c23070
    game_data_man_addr = 0x3c17ee8
  elseif ver.major == 1 and ver.minor == 6 and ver.build == 0 then
    event_flag_man_addr = 0x3c33508
    field_area_addr = 0x3c34298
    game_data_man_addr = 0x3c29108
  elseif ver.major == 1 and ver.minor == 7 and ver.build == 0 then
    event_flag_man_addr = 0x3c4dec8
    field_area_addr = 0x3c4ec50
    game_data_man_addr = 0x3c43ac8
  elseif ver.major == 1 and ver.minor == 8 and ver.build == 0 then
    event_flag_man_addr = 0x3cdbdf8
    field_area_addr = 0x3cdcb80
    game_data_man_addr = 0x3cd1948
  elseif ver.major == 1 and ver.minor == 8 and ver.build == 1 then
    event_flag_man_addr = 0x3cdbdf8
    field_area_addr = 0x3cdcb80
    game_data_man_addr = 0x3cd1948
  elseif ver.major == 1 and ver.minor == 9 and ver.build == 0 then
    event_flag_man_addr = 0x3cdf238
    field_area_addr = 0x3cdffc0
    game_data_man_addr = 0x3cd4d88
  elseif ver.major == 1 and ver.minor == 9 and ver.build == 1 then
    event_flag_man_addr = 0x3cdf238
    field_area_addr = 0x3cdffc0
    game_data_man_addr = 0x3cd4d88
  elseif ver.major == 2 and ver.minor == 0 and ver.build == 0 then
    event_flag_man_addr = 0x3cdf238
    field_area_addr = 0x3cdffc0
    game_data_man_addr = 0x3cd4d88
  end
  event_flag_man_addr = process.base + event_flag_man_addr
  field_area_addr = process.base + field_area_addr
  game_data_man_addr = process.base + game_data_man_addr
  print("  > EventFlagMan address: 0x" .. tostring(event_flag_man_addr))
  print("  > FieldArea address: 0x" .. tostring(field_area_addr))
  print("  > GameDataMan address: 0x" .. tostring(game_data_man_addr))
end

local function search_for_game_process()
  for pid, name in memreader.processes() do
    if name == 'eldenring.exe' then
      process = memreader.openprocess(pid)
      if process == nil then
        return
      end
      print('+ Game found')
      get_offset_table()
      break
    end
  end
end

local function update()
  if process == nil then
    search_for_game_process()
    return
  end
  if process:exitcode() ~= nil then
    process = nil
    print('- Game closed')
    return
  end
  local addrstr = process:read(event_flag_man_addr, 8)
  if addrstr == nil then return end
  addr = string.unpack('=I8', addrstr)
  addrstr = process:read(addr + 0x28, 8)
  if addrstr == nil then return end
  event_flag_address = string.unpack('=I8', addrstr)
end

local function game_running()
  return process ~= nil
end

local function read_flag(offset)
  if process == nil or event_flag_address == 0 then
    return 0
  end
  local str = process:read(event_flag_address + offset, 1)
  if str == nil then return 0 end
  return string.byte(str)
end

local function get_map_area()
  local addrstr = process:read(field_area_addr, 8)
  if addrstr == nil then return 0 end
  addr = string.unpack('=I8', addrstr)
  addrstr = process:read(addr + 0xE4, 4)
  if addrstr == nil then return 0 end
  local area, _ = string.unpack('=I4', addrstr)
  return area
end

local function get_player_attr()
  local addrstr = process:read(game_data_man_addr, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process:read(addr + 0xA0, 4)
  if addrstr == nil then return nil end
  igt = string.unpack('=I4', addrstr)
  addrstr = process:read(addr + 0x120, 4)
  if addrstr == nil then return nil end
  rounds = string.unpack('=I4', addrstr)
  addrstr = process:read(addr + 8, 8)
  if addrstr == nil then return nil end
  addr = string.unpack('=I8', addrstr)
  addrstr = process:read(addr + 0x3C, 0x38)
  if addrstr == nil then return nil end
  res = {string.unpack('=I4I4I4I4I4I4I4I4I4I4I4I4I4I4', addrstr)}
  res[9] = igt
  res[10] = rounds + 1
  return res
end

return {
  update = update,
  game_running = game_running,
  read_flag = read_flag,
  get_map_area = get_map_area,
  get_player_attr = get_player_attr
}

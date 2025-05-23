local memreader = require('memreader')
local process = nil

local address_table = {
  event_flag_man_addr = 0,
  field_area_addr = 0,
  game_data_man_addr = 0,
  menu_man_imp = 0,
}

local current_game_version = 0

local function get_offset_table()
  local version = process:version()
  if version == nil then
    return
  end
  local ver = version.file
  current_game_version = (ver.major << 16) | (ver.minor << 8) | ver.build
  print('  > Version: ' .. ver.major .. '.' .. ver.minor .. '.' .. ver.build)
  if current_game_version <= 0x10200 then
    address_table.event_flag_man_addr = 0x3c526e8
    address_table.field_area_addr = 0x3c53470
    address_table.game_data_man_addr = 0x3c481b8
    address_table.menu_man_imp = 0x3c55b30
  elseif current_game_version <= 0x10201 then
    address_table.event_flag_man_addr = 0x3c52708
    address_table.field_area_addr = 0x3c53490
    address_table.game_data_man_addr = 0x3c481d8
    address_table.menu_man_imp = 0x3c55b50
  elseif current_game_version <= 0x10202 then
    address_table.event_flag_man_addr = 0x3c52728
    address_table.field_area_addr = 0x3c534b0
    address_table.game_data_man_addr = 0x3c481f8
    address_table.menu_man_imp = 0x3c55b70
  elseif current_game_version <= 0x10203 then
    address_table.event_flag_man_addr = 0x3c55748
    address_table.field_area_addr = 0x3c564d0
    address_table.game_data_man_addr = 0x3c4b218
    address_table.menu_man_imp = 0x3c58b90
  elseif current_game_version <= 0x10300 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
    address_table.menu_man_imp = 0x3c6a700
  elseif current_game_version <= 0x10301 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
    address_table.menu_man_imp = 0x3c6a700
  elseif current_game_version <= 0x10302 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
    address_table.menu_man_imp = 0x3c6a700
  elseif current_game_version <= 0x10400 then
    address_table.event_flag_man_addr = 0x3c0a538
    address_table.field_area_addr = 0x3c0b2c0
    address_table.game_data_man_addr = 0x3c00028
    address_table.menu_man_imp = 0x3c0d9d0
  elseif current_game_version <= 0x10401 then
    address_table.event_flag_man_addr = 0x3c0a538
    address_table.field_area_addr = 0x3c0b2c0
    address_table.game_data_man_addr = 0x3c00028
    address_table.menu_man_imp = 0x3c0d9d0
  elseif current_game_version <= 0x10500 then
    address_table.event_flag_man_addr = 0x3c222e8
    address_table.field_area_addr = 0x3c23070
    address_table.game_data_man_addr = 0x3c17ee8
    address_table.menu_man_imp = 0x3c25780
  elseif current_game_version <= 0x10600 then
    address_table.event_flag_man_addr = 0x3c33508
    address_table.field_area_addr = 0x3c34298
    address_table.game_data_man_addr = 0x3c29108
    address_table.menu_man_imp = 0x3c369a0
  elseif current_game_version <= 0x10700 then
    address_table.event_flag_man_addr = 0x3c4dec8
    address_table.field_area_addr = 0x3c4ec50
    address_table.game_data_man_addr = 0x3c43ac8
    address_table.menu_man_imp = 0x3c51360
  elseif current_game_version <= 0x10800 then
    address_table.event_flag_man_addr = 0x3cdbdf8
    address_table.field_area_addr = 0x3cdcb80
    address_table.game_data_man_addr = 0x3cd1948
    address_table.menu_man_imp = 0x3cdf140
  elseif current_game_version <= 0x10801 then
    address_table.event_flag_man_addr = 0x3cdbdf8
    address_table.field_area_addr = 0x3cdcb80
    address_table.game_data_man_addr = 0x3cd1948
    address_table.menu_man_imp = 0x3cdf140
  elseif current_game_version <= 0x10900 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
    address_table.menu_man_imp = 0x3ce2580
  elseif current_game_version <= 0x10901 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
    address_table.menu_man_imp = 0x3ce2580
  elseif current_game_version <= 0x20000 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
    address_table.menu_man_imp = 0x3ce2580
  elseif current_game_version <= 0x20001 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
    address_table.menu_man_imp = 0x3ce2580
  elseif current_game_version <= 0x20200 then
    address_table.event_flag_man_addr = 0x3d68448
    address_table.field_area_addr = 0x3d691d8
    address_table.game_data_man_addr = 0x3d5df38
    address_table.menu_man_imp = 0x3d6b7b0
  elseif current_game_version <= 0x20203 then
    address_table.event_flag_man_addr = 0x3d68468
    address_table.field_area_addr = 0x3d691f8
    address_table.game_data_man_addr = 0x3d5df58
    address_table.menu_man_imp = 0x3d6b7d0
  elseif current_game_version <= 0x20300 then
    address_table.event_flag_man_addr = 0x3d68468
    address_table.field_area_addr = 0x3d691f8
    address_table.game_data_man_addr = 0x3d5df58
    address_table.menu_man_imp = 0x3d6b7d0
  elseif current_game_version <= 0x20400 then
    address_table.event_flag_man_addr = 0x3d68448
    address_table.field_area_addr = 0x3d691d8
    address_table.game_data_man_addr = 0x3d5df38
    address_table.menu_man_imp = 0x3d6b7b0
  elseif current_game_version <= 0x20400 then
    address_table.event_flag_man_addr = 0x3d68448
    address_table.field_area_addr = 0x3d691d8
    address_table.game_data_man_addr = 0x3d5df38
    address_table.menu_man_imp = 0x3d6b7b0
  else
    address_table.event_flag_man_addr = 0x3d68448
    address_table.field_area_addr = 0x3d691d8
    address_table.game_data_man_addr = 0x3d5df38
    address_table.menu_man_imp = 0x3d6b7b0
  end
  address_table.event_flag_man_addr = process.base + address_table.event_flag_man_addr
  address_table.field_area_addr = process.base + address_table.field_area_addr
  address_table.game_data_man_addr = process.base + address_table.game_data_man_addr
  print("  > EventFlagMan address: 0x" .. tostring(address_table.event_flag_man_addr))
  print("  > FieldArea address: 0x" .. tostring(address_table.field_area_addr))
  print("  > GameDataMan address: 0x" .. tostring(address_table.game_data_man_addr))
end

local function search_for_game_process()
  for pid, name in memreader.processes() do
    if name == 'eldenring.exe' then
      process = memreader.openprocess(pid)
      if process == nil then
        return
      end
      if process.base == 0 then
        process = nil
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
  if address_table == nil then
    get_offset_table()
  end
  if process:exitcode() ~= nil then
    process = nil
    print('- Game closed')
    return
  end
end

local function game_running()
  return process ~= nil and address_table ~= nil
end

local function game_version()
  return current_game_version
end

local function get_address_table()
  return address_table
end

local function read_memory(addr, size)
  if process == nil then return nil end
  return process:read(addr, size)
end

local function read_u8(addr)
  if process == nil then return 0 end
  return process:readbyte(addr)
end

local function read_u16(addr)
  if process == nil then return 0 end
  return process:readshort(addr)
end

local function read_u32(addr)
  if process == nil then return 0 end
  return process:readint(addr)
end

local function read_u64(addr)
  if process == nil then return 0 end
  return process:readlong(addr)
end

return {
  update = update,
  game_running = game_running,
  game_version = game_version,
  get_address_table = get_address_table,
  read_memory = read_memory,
  read_u8 = read_u8,
  read_u16 = read_u16,
  read_u32 = read_u32,
  read_u64 = read_u64
}

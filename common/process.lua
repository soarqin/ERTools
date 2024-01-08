local memreader = require('memreader')
local process = nil

local address_table = {
  event_flag_man_addr = 0,
  field_area_addr = 0,
  game_data_man_addr = 0
}

local function get_offset_table()
  local ver = process:version().file
  print('  > Version: ' .. ver.major .. '.' .. ver.minor .. '.' .. ver.build)
  if ver.major == 1 and ver.minor == 2 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c526e8
    address_table.field_area_addr = 0x3c53470
    address_table.game_data_man_addr = 0x3c481b8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 1 then
    address_table.event_flag_man_addr = 0x3c52708
    address_table.field_area_addr = 0x3c53490
    address_table.game_data_man_addr = 0x3c481d8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 2 then
    address_table.event_flag_man_addr = 0x3c52728
    address_table.field_area_addr = 0x3c534b0
    address_table.game_data_man_addr = 0x3c481f8
  elseif ver.major == 1 and ver.minor == 2 and ver.build == 3 then
    address_table.event_flag_man_addr = 0x3c55748
    address_table.field_area_addr = 0x3c564d0
    address_table.game_data_man_addr = 0x3c4b218
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 1 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 3 and ver.build == 2 then
    address_table.event_flag_man_addr = 0x3c672a8
    address_table.field_area_addr = 0x3c68040
    address_table.game_data_man_addr = 0x3c5cd78
  elseif ver.major == 1 and ver.minor == 4 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c0a538
    address_table.field_area_addr = 0x3c0b2c0
    address_table.game_data_man_addr = 0x3c00028
  elseif ver.major == 1 and ver.minor == 4 and ver.build == 1 then
    address_table.event_flag_man_addr = 0x3c0a538
    address_table.field_area_addr = 0x3c0b2c0
    address_table.game_data_man_addr = 0x3c00028
  elseif ver.major == 1 and ver.minor == 5 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c222e8
    address_table.field_area_addr = 0x3c23070
    address_table.game_data_man_addr = 0x3c17ee8
  elseif ver.major == 1 and ver.minor == 6 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c33508
    address_table.field_area_addr = 0x3c34298
    address_table.game_data_man_addr = 0x3c29108
  elseif ver.major == 1 and ver.minor == 7 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3c4dec8
    address_table.field_area_addr = 0x3c4ec50
    address_table.game_data_man_addr = 0x3c43ac8
  elseif ver.major == 1 and ver.minor == 8 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3cdbdf8
    address_table.field_area_addr = 0x3cdcb80
    address_table.game_data_man_addr = 0x3cd1948
  elseif ver.major == 1 and ver.minor == 8 and ver.build == 1 then
    address_table.event_flag_man_addr = 0x3cdbdf8
    address_table.field_area_addr = 0x3cdcb80
    address_table.game_data_man_addr = 0x3cd1948
  elseif ver.major == 1 and ver.minor == 9 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
  elseif ver.major == 1 and ver.minor == 9 and ver.build == 1 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
  elseif ver.major == 2 and ver.minor == 0 and ver.build == 0 then
    address_table.event_flag_man_addr = 0x3cdf238
    address_table.field_area_addr = 0x3cdffc0
    address_table.game_data_man_addr = 0x3cd4d88
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
end

local function game_running()
  return process ~= nil
end

local function get_address_table()
  return address_table
end

local function read_memory(addr, size)
  if process == nil then return nil end
  return process:read(addr, size)
end

return {
  update = update,
  game_running = game_running,
  get_address_table = get_address_table,
  read_memory = read_memory
}

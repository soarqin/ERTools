local VERSION = '0.3'

print('Elden Ring LiveStreamingTool v' .. VERSION)

package.cpath = './?.dll;./bin/?.dll;' .. package.cpath

local uv = require('luv')
local lfs = require('lfs')
local current_script = string.gsub(debug.getinfo(1, 'S').short_src, '\\', '/')
local script_filename = string.match(current_script, '[^/]+$')
local script_directory = string.sub(current_script, 1, #current_script - #script_filename)

-- load process and plugins
process = require('common.process')
util = require('common.util')
inifile = require('common.inifile')
config = inifile.parse('livestreamingtool.ini')
lfs.mkdir(config.output_folder)
local plugins = {}
print('+ Loading plugins...')
for plugin_name, enabled in pairs(config.plugins) do
  if enabled then
    print('  > ' .. plugin_name)
    plugins[#plugins + 1] = require(script_directory .. plugin_name)
  end
end
print('- Done.')

-- register signals
uv.new_signal():start('sigint', function(...) os.exit(-1) end)
uv.new_signal():start('sigbreak', function(...) os.exit(-1) end)
uv.new_signal():start('sighup', function(...) os.exit(-1) end)

-- add update timer
uv.new_timer():start(0, config.update_interval, function()
  process.update()
  local addr = process.read_u64(process.get_address_table().game_data_man_addr)
  if addr == 0 then return end
  igt = process.read_u32(addr + 0xA0)
  if igt == 0 then return end
  for _, plugin in pairs(plugins) do
    plugin.update()
  end
end)

-- enter loop
uv.run()

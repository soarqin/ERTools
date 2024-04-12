local current_script = string.gsub(debug.getinfo(1, 'S').short_src, '\\', '/')
local script_filename = string.match(current_script, '[^/]+$')
local script_directory = string.sub(current_script, 1, #current_script - #script_filename)

package.path = script_directory..'?.lua;'..package.path

-- load process and plugins
process = require('common.process')
util = require('common.util')
inifile = require('common.inifile')
config = inifile.parse('livestreamingtool.ini')

local plugins = {}
print('+ Loading plugins...')
for plugin_name, enabled in pairs(config.plugins) do
  if enabled then
    print('  > ' .. plugin_name)
    plugins[#plugins + 1] = require(script_directory .. plugin_name)
  end
end
print('- Done.')

function update()
  process.update()
  for _, plugin in pairs(plugins) do
    plugin.update()
  end
end

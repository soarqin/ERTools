local current_script = string.gsub(debug.getinfo(1, 'S').short_src, '\\', '/')
local script_filename = string.match(current_script, '[^/]+$')
local script_directory = string.sub(current_script, 1, #current_script - #script_filename)

package.path = script_directory..'?.lua;'..package.path

-- load process and plugins
process = require('common.process')
util = require('common.util')
config = require(script_directory .. '_config_')

local plugins = {}
print('+ Loading plugins...')
for file in lfs.dir(script_directory) do
  if file ~= '.' and file ~= '..' and file ~= script_filename and string.sub(file, 1, 1) ~= '_' then
    local filename = script_directory .. file
    local attr = lfs.attributes(filename)
    if attr.mode ~= 'directory' and string.sub(file, -4) == '.lua' then
      print('  > ' .. string.sub(file, 1, -5))
      plugins[#plugins + 1] = require(string.sub(filename, 1, -5))
    end
  end
end
print('- Done.')

function update()
  process.update()
  for _, plugin in pairs(plugins) do
    plugin.update()
  end
end

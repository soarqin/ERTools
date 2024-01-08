package.cpath = './?.dll;' .. package.cpath

local uv = require('luv')
local lfs = require('lfs')
local current_script = string.gsub(debug.getinfo(1, 'S').short_src, '\\', '/')
local script_filename = string.match(current_script, '[^/]+$')
local sciprt_directory = string.sub(current_script, 1, #current_script - #script_filename)

-- load process and plugins
process = require('process')
local plugins = {}
print('+ Loading plugins...')
for file in lfs.dir(sciprt_directory) do
  if file ~= '.' and file ~= '..' and file ~= script_filename then
    local filename = sciprt_directory .. file
    local attr = lfs.attributes(filename)
    if attr.mode ~= 'directory' and string.sub(file, -4) == '.lua' then
    	print('  > ' .. string.sub(file, 1, -5))
    	plugins[#plugins + 1] = require(string.sub(filename, 1, -5))
    end
  end
end
print('- Done.')

-- register signals
uv.new_signal():start('sigint', function(...) os.exit(-1) end)
uv.new_signal():start('sigbreak', function(...) os.exit(-1) end)
uv.new_signal():start('sighup', function(...) os.exit(-1) end)

-- add update timer
uv.new_timer():start(1000, 1000, function()
	process.update()
	for _, plugin in pairs(plugins) do
		plugin.update()
	end
end)

-- enter loop
uv.run()

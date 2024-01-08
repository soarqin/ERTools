package.cpath = './?.dll;' .. package.cpath

local uv = require('luv')
local lfs = require('lfs')

-- load process and plugins
process = require('process')
local plugins = {}
print('+ Loading plugins...')
for file in lfs.dir('plugins') do
  if file ~= '.' and file ~= '..' then
    local f = 'plugins/'..file
    local attr = lfs.attributes(f)
    if attr.mode ~= 'directory' and string.sub(file, -4) == '.lua' then
    	print('  > ' .. string.sub(file, 1, -5))
    	plugins[#plugins + 1] = require(string.sub(f, 1, -5))
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

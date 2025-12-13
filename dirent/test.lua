local dirent = require('dirent')

local result = 1
do
	local dir <close> = assert(dirent.opendir('.'))
	repeat
		local ent, err, code = dir:read()
		if err then
			assert(nil, err, code)
		end
		if not ent then
			break
		end
		if ent.name == 'lua_dirent.c' then
			result = 0
			break
		end
	until false
end
os.exit(result)

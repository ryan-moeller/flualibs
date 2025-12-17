local linker = require('sys.linker')
local module = require('sys.module')
local ucl = require('ucl')

local function iter_klds()
	return function (_, fileid)
		return linker.kldnext(fileid)
	end, nil, 0
end

local function iter_mods(fileid)
	return function (_, modid)
		if modid then
			return module.modfnext(modid)
		end
	end, nil, linker.kldfirstmod(fileid)
end

local klds = {}
for fileid in iter_klds() do
	local kld = assert(linker.kldstat(fileid))
	kld.address = tostring(kld.address):match(' (.*)')
	kld.modules = {}
	for modid in iter_mods(fileid) do
		local mod = assert(module.modstat(modid))
		table.insert(kld.modules, mod)
	end
	table.insert(klds, kld)
end
print(ucl.to_json(klds))

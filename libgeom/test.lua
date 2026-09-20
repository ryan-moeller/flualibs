-- Copyright (c) 2026 Ryan Moeller
-- SPDX-License-Identifier: BSD-2-Clause

local geom = require('geom')
local ucl = require('ucl')

-- Coerce a lightuserdata pointer to a reasonable string representation.
function id(obj)
	return tostring(obj.id):sub(11)
end

-- Break mesh cycles to form a tree that ucl can handle.
gmesh = geom.gettree()
tree = { classes={} }
for gclass in gmesh:class() do
	local class = {
		id = id(gclass),
		name = gclass.name,
		geom = {},
		config = gclass.config,
	}
	tree.classes[#tree.classes + 1] = class
	for ggeom in gclass:geom() do
		local geom = {
			id = id(ggeom),
			class = id(ggeom.class),
			name = ggeom.name,
			rank = ggeom.rank,
			consumer = {},
			provider = {},
			config = ggeom.config,
		}
		class.geom[#class.geom + 1] = geom
		for gconsumer in ggeom:consumer() do
			local consumer = {
				id = id(gconsumer),
				geom = id(gconsumer.geom),
				provider = id(gconsumer.provider),
				mode = gconsumer.mode,
				config = gconsumer.config,
			}
			geom.consumer[#geom.consumer + 1] = consumer
		end
		for gprovider in ggeom:provider() do
			local provider = {
				id = id(gprovider),
				name = gprovider.name,
				geom = id(gprovider.geom),
				consumers = {},
				mode = gprovider.mode,
				mediasize = gprovider.mediasize,
				sectorsize = gprovider.sectorsize,
				stripeoffset = gprovider.stripeoffset,
				stripesize = gprovider.stripesize,
				config = gprovider.config,
			}
			for gconsumer in gprovider:consumers() do
				local i = id(gconsumer)
				provider.consumers[#provider.consumers + 1] = i
			end
			geom.provider[#geom.provider + 1] = provider
		end
	end
end
print(ucl.to_format(tree, 'yaml'))

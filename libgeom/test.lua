-- Copyright (c) 2026 Ryan Moeller
-- SPDX-License-Identifier: BSD-2-Clause

local geom = require('geom')
local ucl = require('ucl')

-- Break mesh cycles to form a tree that ucl can handle.
gmesh = geom.gettree()
tree = { classes={} }
for gclass in gmesh:class() do
	local class = {
		id = gclass.id,
		name = gclass.name,
		geom = {},
		config = gclass.config,
	}
	tree.classes[#tree.classes + 1] = class
	for ggeom in gclass:geom() do
		local geom = {
			id = ggeom.id,
			class = ggeom.class.id,
			name = ggeom.name,
			rank = ggeom.rank,
			consumer = {},
			provider = {},
			config = ggeom.config,
		}
		class.geom[#class.geom + 1] = geom
		for gconsumer in ggeom:consumer() do
			local consumer = {
				id = gconsumer.id,
				geom = gconsumer.geom.id,
				provider = gconsumer.provider.id,
				mode = gconsumer.mode,
				config = gconsumer.config,
			}
			geom.consumer[#geom.consumer + 1] = consumer
		end
		for gprovider in ggeom:provider() do
			local provider = {
				id = gprovider.id,
				name = gprovider.name,
				geom = gprovider.geom.id,
				consumers = {},
				mode = gprovider.mode,
				mediasize = gprovider.mediasize,
				sectorsize = gprovider.sectorsize,
				stripeoffset = gprovider.stripeoffset,
				stripesize = gprovider.stripesize,
				config = gprovider.config,
			}
			geom.provider[#geom.provider + 1] = provider
		end
	end
end
print(ucl.to_format(tree, 'yaml'))

local geom = require('geom')

local gstat = geom.stats_open()
local snap = gstat:snapshot()
print(snap:timestamp())
local ds = snap:next()
print(ds.device_name)

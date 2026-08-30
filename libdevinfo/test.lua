local devinfo = require('devinfo')

local devs <close> = devinfo.snapshot()

for _, rman in ipairs(devs:rmans()) do
	print(rman.start, rman.size, rman.desc)
end

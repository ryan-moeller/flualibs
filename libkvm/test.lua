local kvm = require('kvm')

local kd <close> = assert(kvm.open2())
local ncpus = assert(kd:getncpus())
assert(kd:close())
print(ncpus)

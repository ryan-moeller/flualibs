local grp = require('grp')
local ucl = require('ucl')

local wheel = assert(grp.getgrgid(0))
print(ucl.to_json(wheel))

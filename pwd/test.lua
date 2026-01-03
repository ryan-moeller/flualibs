local pwd = require('pwd')
local ucl = require('ucl')

local root = assert(pwd.getpwuid(0))
print(ucl.to_json(root))

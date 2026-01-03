local pwd = require('pwd')

local root = assert(pwd.getpwuid(0))
print(root.shell)

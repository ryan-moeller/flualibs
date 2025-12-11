local stat = require('stat')
local chflags = require('chflags')

local st = assert(stat.stat('/COPYRIGHT'))
print(assert(chflags.fflagstostr(st.flags)))

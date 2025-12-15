local stat = require('freebsd.sys.stat')

local st = assert(stat.stat('/COPYRIGHT'))
assert(stat.chflags('/COPYRIGHT', stat.UF_ARCHIVE))
print(assert(stat.chflags.fflagstostr(st.flags)))

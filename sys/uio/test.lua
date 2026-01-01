local uio = require('sys.uio')

local bufs = assert(uio.readv(io.stdin, {5,1,5,512}))
assert(#bufs[1] == 5)
assert(#bufs[2] == 1)
assert(#bufs[3] == 5)
assert(#bufs[4] == 512)
print(bufs[1], bufs[3])

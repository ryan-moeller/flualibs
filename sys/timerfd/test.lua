local timerfd = require('sys.timerfd')
local time = require('time')

local tfd <close> = assert(timerfd.create(time.CLOCK_REALTIME))
assert(tfd:settime(0, {value=1,interval=0}))
print('before')
assert(tfd:read() == 1)
print('after')

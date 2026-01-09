local aio = require('aio')

do -- example from the manual
	local f <close> = io.open('/COPYRIGHT')

	local cb = aio.aiocb.shared(f, 0, 16384)
	assert(cb:read())
	assert(cb:suspend())
	local len = assert(cb:_return())
	print(string.sub(cb.buf, 0, len))
end

-- TODO: test :waitcomplete, multiple threads sharing cbs with an event queue

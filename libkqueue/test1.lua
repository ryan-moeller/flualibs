package.cpath = "../fileno/?.so;" .. package.cpath

require('fileno')
local kqueue = require('kqueue')

local kq = kqueue.kqueue()

local changelist = {
	{
		ident = io.stdin:fileno(),
		filter = kqueue.EVFILT_READ,
		flags = kqueue.EV_ADD,
		fflags = 0,
		data = 0,
		udata = coroutine.create(function (event) repeat
			print("READ on stdin: ", ucl.to_json(event))
			local len = event.data
			local data = io.stdin:read(len)
			io.stdout:write(data)
			event = coroutine.yield()
		until false end)
	}
}

local event = kq:kevent(changelist)
repeat
	coroutine.resume(event.udata, event)
	event = kq:kevent()
until false

kq:close()

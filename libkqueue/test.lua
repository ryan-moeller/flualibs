package.cpath = "/home/ryan/libmq/?.so;" .. package.cpath
package.cpath = "/home/ryan/libkqueue/?.so;" .. package.cpath

local mq = require("mq")
local kqueue = require("kqueue")

pcall(function() mq.unlink("/myq") end)

local q1 = mq.open("/q1", mq.O_CREAT | mq.O_RDONLY, 438)
local attr1 = q1:getattr()

local q2 = mq.open("/q2", mq.O_CREAT | mq.O_RDONLY, 438)
local attr2 = q2:getattr()

local kq = kqueue.kqueue()

local changelist = {
	{
		ident = q1:getfd(),
		filter = kqueue.EVFILT_READ,
		flags = kqueue.EV_ADD,
		fflags = 0,
		data = 0,
		udata = coroutine.create(function (event) repeat
			-- check event for error?
			print("READ on q1: ", ucl.to_json(event))
			local msg, prio = q1:receive(attr1.msgsize)
			print("q1: ", msg, " (", prio, ")")
			event = coroutine.yield()
		until false end)
	},
	{
		ident = q2:getfd(),
		filter = kqueue.EVFILT_READ,
		flags = kqueue.EV_ADD,
		fflags = 0,
		data = 0,
		udata = coroutine.create(function (event) repeat
			-- check event for error?
			print("READ on q2: ", ucl.to_json(event))
			print(q2)
			local msg, prio = q2:receive(attr2.msgsize)
			print("q2: ", msg, " (", prio, ")")
			event = coroutine.yield()
		until false end)
	}
}

repeat
	local event = kq:kevent(changelist)
	coroutine.resume(event.udata, event)
until false

kq:close()
q1:close()
q2:close()

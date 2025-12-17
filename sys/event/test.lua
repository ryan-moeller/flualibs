require('stdio')
local event = require('sys.event')
local ucl = require('ucl')

local kq = event.kqueue()
local threads = {}
local changelist

local function insert_change(ident, filter, flags, handler)
	-- Handler may be an existing thread or a function for a new thread.
	local thread = threads[handler] and handler or coroutine.create(handler)
	-- Add a kevent to the changelist.
	changelist = changelist or {}
	table.insert(changelist, {
		-- Only ident, filter, and flags are required.  The rest of the
		-- fields may be nil if not used.
		ident = ident,
		filter = filter,
		flags = flags,
		-- These fields default to 0 if nil.
		fflags = 0,
		data = 0,
		-- This must be a thread (coroutine) if not nil, and it must
		-- outlive the event in the kqueue.  Take care to avoid it being
		-- collected as garbage before the event is deleted!
		udata = thread
	})
	-- Avoid premature garbage collection.
	threads[thread] = true
end

insert_change(
	io.stdin:fileno(),
	event.EVFILT_READ,
	event.EV_ADD,
	function(ev)
		repeat
			print('stdin is readable:', ucl.to_json(ev))
			local len = ev.data
			local data = io.stdin:read(len)
			if not data then
				print('stdin closed')
				break
			end
			print('data:', data)
			ev = coroutine.yield()
		until false
	end
)

-- Main event loop
while next(threads) do
	local ev = assert(kq:kevent(changelist))
	changelist = nil
	local thread = ev.udata
	if thread then
		if (ev.flags & event.EV_ERROR) ~= 0 then
			print('error:', ucl.to_json(ev))
		else
			assert(coroutine.resume(thread, ev))
		end
		if coroutine.status(thread) == 'dead' then
			-- Delete the event for this thread.
			ev.flags = event.EV_DELETE
			ev.udata = nil
			changelist = changelist or {}
			table.insert(changelist, ev)
			-- Now it is safe to GC the thread.
			threads[thread] = nil
			assert(coroutine.close(thread))
		end
	end
end

kq:close()

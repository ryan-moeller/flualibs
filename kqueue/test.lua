require('fileno')
local kqueue = require('kqueue')
local ucl = require('ucl')

local kq = kqueue.kqueue()
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
	kqueue.EVFILT_READ,
	kqueue.EV_ADD,
	function(event)
		repeat
			print('stdin is readable:', ucl.to_json(event))
			local len = event.data
			local data = io.stdin:read(len)
			if not data then
				print('stdin closed')
				break
			end
			print('data:', data)
			event = coroutine.yield()
		until false
	end
)

-- Main event loop
while next(threads) do
	local event = assert(kq:kevent(changelist))
	changelist = nil
	local thread = event.udata
	if thread then
		if (event.flags & kqueue.EV_ERROR) ~= 0 then
			print('error:', ucl.to_json(event))
		else
			assert(coroutine.resume(thread, event))
		end
		if coroutine.status(thread) == 'dead' then
			-- Delete the event for this thread.
			event.flags = kqueue.EV_DELETE
			event.udata = nil
			changelist = changelist or {}
			table.insert(changelist, event)
			-- Now it is safe to GC the thread.
			threads[thread] = nil
			assert(coroutine.close(thread))
		end
	end
end

kq:close()

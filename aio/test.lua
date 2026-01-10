local aio = require('aio')

do -- example from the manual
	local f <close> = assert(io.open('/COPYRIGHT'))

	local cb = aio.aiocb.shared(f, 0, 16384)
	assert(cb:read())
	assert(cb:suspend())
	local len = assert(cb:_return())
	print(string.sub(cb.buf, 0, len))
end

do -- single-threaded :waitcomplete()
	local f <close> = assert(io.open('/COPYRIGHT'))

	local cb = aio.aiocb.shared(f, 0, 16384)
	assert(cb:read())
	local cb1, len = assert(aio.waitcomplete())
	assert(cb1 == cb)
	print(string.sub(cb.buf, 0, len))
end

do -- multiple threads sharing cbs with an event queue
	local event = require('sys.event')
	local pthread = require('pthread')
	local signal = require('signal')
	local ucl = require('ucl')

	local kq <close> = assert(event.kqueue())
	local kqfd = kq:fileno()

	local blocker = assert(pthread.mutex.new())
	assert(blocker:lock())
	local blockerc = blocker:cookie()

	-- Submit many AIO requests concurrently.
	local nthreads = 100
	local threads = {}
	while #threads < nthreads do
		table.insert(threads, assert(pthread.create(function()
			local event = require('sys.event')
			local aio = require('aio')
			local pthread = require('pthread')
			local signal = require('signal')

			local f <close> = assert(io.open('/COPYRIGHT'))

			local cb = aio.aiocb.shared(f, 0, 16384, nil, {
				notify = signal.SIGEV_KEVENT,
				notify_kqueue = kqfd,
				notify_kevent_flags = event.EV_ONESHOT,
			})
			assert(cb:read())

			-- Block the thread so it doesn't close f until safe.
			local blocker = assert(pthread.mutex.retain(blockerc))
			assert(blocker:lock())
			assert(blocker:unlock())
		end)))
	end

	-- Serially print completed requests.
	local completed = 0
	while completed < nthreads do
		local ev = assert(kq:kevent())
		print(ucl.to_json(ev))
		assert(ev.flags == event.EV_EOF | event.EV_ONESHOT)
		local cb = aio.aiocb.retain(ev.cookie)
		local len = assert(cb:_return())
		print(string.sub(cb.buf, 0, len))
		completed = completed + 1
		print('completed:', completed)
	end
	-- Unblock all the threads so they can exit.
	assert(blocker:unlock())
	-- Clean up.
	for _, thread in ipairs(threads) do
		assert(thread:join())
	end
end

do -- :mlock(), :write(), and :fsync()
	local fcntl = require('fcntl')

	local f <close> = assert(io.tmpfile())

	local cb = assert(aio.aiocb.shared(f, 0,
	    "The request knows where it is by knowing where it isn't."))
	assert(cb:mlock())
	assert(cb:suspend())
	assert(cb:_return())
	assert(cb:write())
	assert(cb:suspend())
	assert(cb:_return())
	assert(cb:fsync(fcntl.O_DSYNC))
	assert(cb:suspend())
	assert(cb:_return())

	print(cb.buf)
end

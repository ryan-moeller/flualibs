local pthread = require('pthread')

local function thread_main(ready_cookie, serial_cookie)
    local pthread = require('pthread')

    local threadid = pthread.getthreadid_np()

    local ready = assert(pthread.barrier.retain(ready_cookie))
    assert(ready:wait())

    local serial = assert(pthread.mutex.retain(serial_cookie))
    assert(serial:lock())
    print(('%q ready'):format(threadid))
    assert(serial:unlock())

    return threadid
end

local threads = {}
local nthreads = 8

local ready = assert(pthread.barrier.new(nthreads))
local ready_cookie = ready:cookie()

local serial = assert(pthread.mutex.new())
local serial_cookie = serial:cookie()

for i = 1, nthreads do
    local thread = assert(pthread.create(thread_main, ready_cookie, serial_cookie))
    table.insert(threads, thread)
end

for _, thread in ipairs(threads) do
    local _, threadid = assert(thread:join())
    print(('joined %q'):format(threadid))
end

-- vim: set et sw=4:

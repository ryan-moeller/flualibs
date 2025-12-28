local fcntl = require('fcntl')
local mqueue = require('mqueue')

local flags = fcntl.O_CREAT | fcntl.O_WRONLY
local mode = tonumber('0644', 8)
local q = assert(mqueue.open("/myqueue", flags, mode))

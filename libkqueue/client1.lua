package.cpath = "/home/ryan/libmq/?.so;" .. package.cpath

local mq = require("mq")

local q = mq.open("/q1", mq.O_WRONLY)

for line in io.lines() do
	q:send(line, 0)
end

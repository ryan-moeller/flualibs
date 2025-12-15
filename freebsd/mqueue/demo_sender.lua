mqueue = require('freebsd.mqueue')
ucl = require('ucl')

name = "/mqdemo"
oflags = mqueue.O_WRONLY

q = mqueue.open(name, oflags)

seq = 0
prio = 0
for line in io.lines() do
	msg = ucl.to_format({message=line, seq=seq}, "msgpack")
	q:send(msg, prio)
	seq = seq + 1
end
q:close()

fcntl = require('fcntl')
mqueue = require('mqueue')
ucl = require('ucl')

name = "/mqdemo"
oflags = fcntl.O_WRONLY

q = mqueue.open(name, oflags)

seq = 0
prio = 0
for line in io.lines() do
	msg = ucl.to_format({message=line, seq=seq}, "msgpack")
	q:send(msg, prio)
	seq = seq + 1
end
q:close()

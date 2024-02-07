mq = require('mq')
ucl = require('ucl')

name = "/mqdemo"
oflags = mq.O_WRONLY

q = mq.open(name, oflags)

seq = 0
prio = 0
for line in io.lines() do
	msg = ucl.to_format({message=line, seq=seq}, "msgpack")
	q:send(msg, prio)
	seq = seq + 1
end
q:close()

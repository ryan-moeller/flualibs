mq = require('mq')
ucl = require('ucl')

name = "/mqdemo"
oflags = mq.O_WRONLY

q = mq.open(name, oflags)

msg = ucl.to_format({kill=true}, "msgpack")
prio = 0
q:send(msg, prio)
q:close()

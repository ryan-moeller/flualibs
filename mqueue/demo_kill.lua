mqueue = require('mqueue')
ucl = require('ucl')

name = "/mqdemo"
oflags = mqueue.O_WRONLY

q = mqueue.open(name, oflags)

msg = ucl.to_format({kill=true}, "msgpack")
prio = 0
q:send(msg, prio)
q:close()

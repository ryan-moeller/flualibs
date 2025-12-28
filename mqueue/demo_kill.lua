fcntl = require('fcntl')
mqueue = require('mqueue')
ucl = require('ucl')

name = "/mqdemo"
oflags = fcntl.O_WRONLY

q = mqueue.open(name, oflags)

msg = ucl.to_format({kill=true}, "msgpack")
prio = 0
q:send(msg, prio)
q:close()

mq = require('mq')
ucl = require('ucl')

name = "/mqdemo"
oflags = mq.O_CREAT + mq.O_RDONLY
mode = 438 -- 0666

q = mq.open(name, oflags, mode)

attr = q:getattr()
while true do
	msg, prio = q:receive(attr.msgsize)
	parser = ucl.parser()
	res, err = parser:parse_string(msg, "msgpack")
	if not res then
		error(err)
	end
	obj = parser:get_object()
	print(ucl.to_format(obj, "yaml"))
	if obj.kill then
		break
	end
end
q:close()
mq.unlink(name)

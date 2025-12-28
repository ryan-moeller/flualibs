fcntl = require('fcntl')
mqueue = require('mqueue')
ucl = require('ucl')

name = "/mqdemo"
oflags = fcntl.O_CREAT + fcntl.O_RDONLY
mode = tonumber('0644', 8)

q = mqueue.open(name, oflags, mode)

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
mqueue.unlink(name)

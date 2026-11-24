local socket = require('sys.socket')
local unistd = require('unistd')

-- A connected stream socket has no source address.
do
	local a, b = assert(socket.socketpair(socket.AF_UNIX,
	    socket.SOCK_STREAM, 0))
	assert(unistd.write(a, 'hello'))
	local data, from = assert(socket.recvfrom(b, 4096))
	assert(data == 'hello')
	assert(from == nil)
	assert(unistd.close(a))
	assert(unistd.close(b))
end

-- A datagram socket returns the data and the source address.
do
	local s = assert(socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0))
	local c = assert(socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0))
	assert(socket.bind(s, {
		family = socket.AF_INET,
		data = '\0\0\127\0\0\1\0\0\0\0\0\0\0\0',
	}))
	local name = assert(socket.getsockname(s))
	assert(socket.sendto(c, 'dgram', 0, name))
	local data, from = assert(socket.recvfrom(s, 4096))
	assert(data == 'dgram')
	assert(from.family == socket.AF_INET)
	assert(unistd.close(s))
	assert(unistd.close(c))
end

local socket = require('sys.socket')
local netdb = require('netdb')

local hints = {socktype=socket.SOCK_STREAM}
local addrs = assert(netdb.getaddrinfo('freebsd.org', 'http', hints))
local flags = netdb.NI_NUMERICHOST | netdb.NI_NUMERICSERV
for _, ai in ipairs(addrs) do
	local host, port = assert(netdb.getnameinfo(ai.addr, flags))
	print(('%s:%s'):format(host, port))
end

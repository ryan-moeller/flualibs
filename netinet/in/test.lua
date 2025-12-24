local socket = require('sys.socket')
local netin = require('netinet.in')
local ifcfg = require('ifconfig').open()
local ucl = require('ucl')

local function is_loopback(addr)
	local sa = addr:addr()
	if sa.family == socket.AF_INET then
		return netin.in_loopback(netin.sockaddr_in(sa).addr)
	elseif sa.family == socket.AF_INET6 then
		return netin.in6_is_addr_loopback(netin.sockaddr_in6(sa).addr)
	end
end

local addrs = {}
ifcfg:foreach_iface(function(_, iface)
	ifcfg:foreach_ifaddr(iface, function(_, addr)
		if is_loopback(addr) then
			local info = ifcfg:addr_info(addr)
			info.iface = addr:name()
			table.insert(addrs, info)
		end
	end)
end)
print(ucl.to_json(addrs))

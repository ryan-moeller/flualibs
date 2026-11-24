local ifcfg = assert(require('ifconfig').open())

local function check(addr)
	assert(addr == nil or math.type(addr.family) == 'integer')
end

ifcfg:foreach_iface(function(_, iface)
	ifcfg:foreach_ifaddr(iface, function(_, ifa)
		check(ifa:addr())
		check(ifa:netmask())
		check(ifa:dstaddr())
	end)
end)

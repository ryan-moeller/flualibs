local caph = require('capsicum_helpers')
local casper = require('casper')
local sysctl = require('casper.sysctl')

local capcas = assert(casper.init())
assert(caph.enter())
local capsysctl = assert(capcas:service_open('system.sysctl'))
capcas:close()

for mib in sysctl.sysctl(capsysctl):iter_noskip() do
	print(mib:name(), mib:format(), mib:description())
end

for mib in sysctl.sysctl(capsysctl, 'kstat.zfs'):iter() do
	print(mib:name(), mib:value())
end

capsysctl:close()

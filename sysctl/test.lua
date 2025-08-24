local sysctl = require('sysctl')

for mib in sysctl():iter_noskip() do
	print(mib:name(), mib:format(), mib:description())
end

for mib in sysctl('kstat.zfs'):iter() do
	print(mib:name(), mib:value())
end

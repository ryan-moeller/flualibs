local sysctl = require('sys.sysctl')

for mib in sysctl.sysctl():iter_noskip() do
	print(mib:name(), mib:format(), mib:description())
end

for mib in sysctl.sysctl('kstat.zfs'):iter() do
	print(mib:name(), mib:value())
end

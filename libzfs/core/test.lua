nvpair = require('nvpair')
lzc = require('zfs.core')

lzc.init()
props = lzc.get_props('system')
prop = props:lookup_nvlist('guid')
guid = prop:lookup_uint64('value')
print(("%u"):format(guid))
for _, name, value, type in pairs(props) do
	print(name, value, type)
end
lzc.fini()

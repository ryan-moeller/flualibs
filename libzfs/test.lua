nvpair = require('nvpair')
zfs = require('zfs')

hdl = zfs.init()
zhp = hdl:zpool_open('system')
name = zhp:get_name()
state = zhp:get_state()
print('pool:', name, 'state:', state)
zhp = hdl:zfs_open('storage/zts', zfs.ZFS_TYPE_FILESYSTEM)
props = zhp:get_all_props()
for _, name, value in pairs(props) do
	value, type = value:lookup('value')
	if type == nvpair.DATA_TYPE_UINT64 then
		print(name, ("%u"):format(value))
	elseif type then
		print(name, value)
	end
end
hdl:fini()

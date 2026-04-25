nvpair = require('nvpair')

nvl = nvpair.nvlist()
nvl:add_boolean_value("simple", true)
nvl:add_string("hello", "world")
buffer = nvl:pack()
assert(#buffer > 0)
nvl1 = nvpair.unpack(buffer)
print(nvl1:lookup_boolean_value("simple"))
print(nvl1:lookup_string("hello"))

t = {}
for cookie, name, value, type in pairs(nvl) do
	table.insert(t, name)
	print(cookie, name, value, type)
end
assert(#t == 2)

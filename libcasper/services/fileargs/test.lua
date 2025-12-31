local cap = require('sys.capsicum')
local fcntl = require('fcntl')
local fileargs = require('casper.fileargs')

local args = {...}
local fa = assert(fileargs.init(args, fcntl.O_RDONLY, 0,
    cap.rights.new(cap.READ, cap.FCNTL), fileargs.OPEN))
cap.enter()
for _, arg in ipairs(args) do
	local f = assert(fa:fopen(arg, 'r'))
	print(f:read('a*'))
end

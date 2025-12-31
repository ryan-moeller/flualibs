local cap = require('sys.capsicum')
local caph = require('capsicum_helpers')

assert(caph.enter_casper())
if cap.sandboxed() then
	print('sandboxed')
else
	print('not sandboxed')
end

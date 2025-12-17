local ACL = require('sys.acl')

local mode = tonumber("755", 8) -- octal 755
print(ACL.from_mode(mode):to_text())

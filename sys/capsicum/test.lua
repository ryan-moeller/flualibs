local cap = require('sys.capsicum')

cap.enter()
assert(cap.sandboxed())

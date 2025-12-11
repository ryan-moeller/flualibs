local cap = require('capsicum')

cap.enter()
assert(cap.sandboxed())

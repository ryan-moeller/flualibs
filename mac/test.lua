local mac = require('mac')

mac.prepare('partition/1'):set_proc()

local label = mac.prepare_process_label()
label:get_proc()
print(label:to_text())

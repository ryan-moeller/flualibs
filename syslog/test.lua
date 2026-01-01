local syslog = require('syslog')

syslog.openlog('syslog(3lua) test', syslog.PERROR)
syslog.syslog(syslog.WARNING, 'this is only a test')
syslog.closelog()

syslog.syslog(syslog.NOTICE, 'test over')

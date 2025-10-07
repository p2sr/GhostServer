<!-- markdownlint-disable MD033 -->

# GhostServer: Commands

|Name|Arguments|Description|
|----|---------|-----------|
|help||shows a list of all available commands|
|start|`[port]`|starts the server on the specified port, or the default port if none is specified|
|stop||terminate the server|
|list||list all the currently connected clients|
|countdown|`pre <cmds>`<br/>`post <cmds>`<br/>`start <duration>`|start/modify a countdown|
|kick|`name <name>`<br/>`id <id>`<br/>`ip <ip>`|kick a client|
|ban|`name <name>`<br/>`id <id>`<br/>`ip <ip>`<br/>`list`|ban a client|
|unban|`<ip>`|unban a client|
|accept|`players`<br/>`spectators`<br/>`all`|start accepting connections|
|refuse|`players`<br/>`spectators`<br/>`all`|stop accepting connections|
|say|`<message>`|send a message to all clients|
|whitelist|`on`<br/>`off`<br/>`kick`<br/>`add <name\|ip> <value>`<br/>`remove <name\|ip> <value>`<br/>`list`|manage the whitelist|

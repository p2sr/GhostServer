# GhostServer: Commands

## CLI Commands

|Name|Arguments|Description|
|----|---------|-----------|
|help||shows a list of all available commands|
|quit||terminate the server|
|list||list all the currently connected clients|
|countdown_set|pre-cmd post-cmd duration|set the pre/post cmds and countdown duration|
|countdown||start a countdown|
|disconnect|name|disconnect a client by name|
|disconnect_id|id|disconnect a client by ID|
|disconnect_ip|ip|disconnect a client by IP address|
|ban|name|ban connections from a certain IP by ghost name|
|ban_id|id|ban connections from a certain IP by ghost ID|
|ban_ip|ip|ban connections from a certain IP by IP address|
|accept_players||start accepting connections from players|
|refuse_players||stop accepting connections from players|
|accept_spectators||start accepting connections from spectators|
|refuse_spectators||stop accepting connections from spectators|
|server_msg|message|send all clients a message from the server|
|whitelist_enable||enable whitelist|
|whitelist_disable||disable whitelist|
|whitelist_add_name|name|add player to whitelist|
|whitelist_add_ip|ip|add player to whitelist|
|whitelist_remove_name|name|remove player from whitelist|
|whitelist_remove_ip|ip|remove player from whitelist|
|whitelist||print out all entries on whitelist|

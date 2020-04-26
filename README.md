# GhostServer
Server for the Ghost feature in SAR (https://github.com/NeKzor/SourceAutoRecord)

## Installation

- Download the last release in [releases page](https://github.com/Blenderiste09/GhostServer/releases)
- Unzip the archive
- Change the port the server will use to communicate by changing the value in **port.cfg**. Default one is **53000**
- In the settings of your router, open both UDP and TCP xxxxx port, xxxxx being the port you specified in **port.cfg**.
- Setup is done :)

## Usage
- Launch __Server.exe__
- Once the server console is open, the server is running. Your public IP adress is shown on the first line along the port the server use to communicate. From there, two options : 
    - Give your public IP to people that wants to connect to your server.
    - Do a no-ip solution and give the address to people.
- People can connect to your server by using SAR ghost commands (those starting with *sar_ghost* or *sar_hud_ghost*) available here : https://github.com/Blenderiste09/SourceAutoRecord/blob/Ghosts/doc/cvars.md.

## Commands
You can find a list of commands and their explanations here : https://github.com/Blenderiste09/GhostServer/blob/master/commands.md

## Contributing
Pull requests are welcome.

## Bug report
If you encountered a bug, please open an issue report.

## License
[MIT](https://choosealicense.com/licenses/mit/)

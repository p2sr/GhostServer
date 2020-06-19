# GhostServer
Server for the Ghost feature in SAR (https://github.com/Blenderiste09/SourceAutoRecord)

## Installation

- Download the last release in [releases page](https://github.com/Blenderiste09/GhostServer/releases)
- Unzip the archive
- Launch __Server.exe__
- The default port is **53000**. You can change it in the ``Settings`` tab.
- In the settings of your router, open both UDP and TCP xxxxx port, xxxxx beign the port you specified in the ``port`` setting.
- Setup is done.

## Usage

- The server will run as soon as you click on the ``Start Server`` button. Your IP adress is shown on the first line along the port the server use to communicate.
- You can set a countdown on the right part of the interface
  - You can set some commands to execute before and after the countdown. You can write them on one line like "sv_cheats 0; say lol" or on multi-line
- Server stops by clicking on the ``Stop Server`` button (previously ``Start Server``)

## Contributing
Pull requests are welcome.

## Bug report
If you encountered a bug, please open an issue report.

## License
[MIT](https://choosealicense.com/licenses/mit/)

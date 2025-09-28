# GhostServer

[![CI](https://github.com/p2sr/GhostServer/workflows/CI/badge.svg)](https://github.com/p2sr/GhostServer/actions?query=workflow%3ACI+branch%3Amaster)

Server for the Ghost feature in [SAR](https://github.com/p2sr/SourceAutoRecord)

## Installation

- Download the latest release in [releases page](https://github.com/p2sr/GhostServer/releases)
- Unzip the archive
- Launch ``Server.exe``
- The default port is **53000**. You can change it in the ``Settings`` tab.
- In the settings of your router, open both UDP and TCP xxxxx port, xxxxx being the port you specified in the ``port`` setting.

## Usage

- The server will run as soon as you click on the ``Start Server`` button. Your IP address is shown on the first line along with the port the server uses to communicate.
- Players and spectators connect to the server with the SAR commands `ghost_connect <ip> [port=53000]` or `ghost_spec_connect <ip> [port=53000]`
- You can set a countdown on the right part of the interface
  - You can set some commands to execute before and after the countdown. You can write them on one line like `"sv_cheats 0; say lol"` or on multi-line
- Server stops by clicking on the ``Stop Server`` button (previously ``Start Server``)

## Contributing

Pull requests are welcome.

## Bug report

If you encountered a bug, please open an issue report.

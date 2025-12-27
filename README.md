# auto-debug

A simple replacement for vsjitdebugger.exe because i don't want to manually choose my only running Visual Studio instance when debugging my 4D Miner mods bruh.  

## Usage:
`auto-debug <pid>` in console, where `pid` is the process id to attach to.  
To use with the [4DModLoader](<https://github.com/4D-Modding/4DModLoader>) you can add `-debugger "auto-debug.exe"` to the launch arguments alongside `-debug`.  
This will only work if the exe is in your PATH or if it is in your 4DM directory, otherwise you must provide the full path to it.  

----

Distributed on https://4d-modding.com/core-files/auto-debug.exe aswell i guess.  

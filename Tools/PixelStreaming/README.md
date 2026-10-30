# Pixel Streaming (Day 13, local only)

Streams `L_TwinStudio` to a browser on the same machine with the PixelStreaming2 plugin. No packaging: the game runs from the editor
exe with `-game`. Decided 2026-10-28: camera control in the browser is the dashboard's view tabs and keys 1–4, forwarded by the stream.

## Setup, once per machine

The signalling server (Wilbur) and the browser frontend ship with UE 5.7 but aren't built. Epic's scripts break on spaces in the
path, so copy them outside the project folder:

1. Copy `C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Media\PixelStreaming2\Resources\WebServers` to `D:\PixelStreaming\WebServers`.
2. Run `D:\PixelStreaming\WebServers\SignallingWebServer\platform_scripts\cmd\setup.bat`. It downloads its own Node (version in
   `NODE_VERSION`) into that folder and builds the server and the frontend. Free, no account.

## Run

```
powershell -ExecutionPolicy Bypass -File Tools\PixelStreaming\start_stream.ps1
```

Opens three windows: Wilbur, the relay (`--source live --loop --rate 20`) and the game. Then open http://localhost:8080 and click
the page to start the stream. `-NoRelay` skips the relay; `-ResX 1280 -ResY 720` makes a smaller stream; `-WebServersDir` if the
server copy is elsewhere.

Ports: 8080 for the browser (not the default 80, which needs admin), 8888 for the game.

# Day 13: streams L_TwinStudio to a local browser with Pixel Streaming 2, no packaging.
# Starts three windows: the signalling server (Wilbur), the telemetry relay, and the game (editor exe with -game).
# Then open http://localhost:<PlayerPort> and click to start the stream. Setup once: see README.md in this folder.
#
#   powershell -ExecutionPolicy Bypass -File Tools\PixelStreaming\start_stream.ps1
#   ... -NoRelay              use the trip file or your own relay
#   ... -ResX 1280 -ResY 720  smaller stream

param(
    [int]$PlayerPort = 8080,
    [int]$StreamerPort = 8888,
    [int]$ResX = 1920,
    [int]$ResY = 1080,
    [int]$RelayRate = 20,
    [switch]$NoRelay,
    [string]$EngineDir = "C:\Program Files\Epic Games\UE_5.7",
    # A copy of the engine's PixelStreaming2\Resources\WebServers. Epic's scripts break on spaces in the path, so it lives outside
    # the project folder.
    [string]$WebServersDir = "D:\PixelStreaming\WebServers"
)

$ErrorActionPreference = "Stop"
$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$ProjectFile = Join-Path $ProjectDir "CarDigitalTwins.uproject"
$ServerDir = Join-Path $WebServersDir "SignallingWebServer"
$EditorExe = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path (Join-Path $ServerDir "platform_scripts\cmd\node"))) {
    throw "Signalling server not set up in $WebServersDir: see README.md in this folder."
}
if (-not (Test-Path $EditorExe)) {
    throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir."
}

# Signalling server. Player port 8080, not the default 80 (needs admin, often taken); no https redirect on localhost.
$ServerArgs = "-- --player_port=$PlayerPort --streamer_port=$StreamerPort --serve --https_redirect=false --console_messages verbose"
Start-Process cmd -ArgumentList "/k", "title Wilbur && cd /d `"$ServerDir\platform_scripts\cmd`" && start.bat $ServerArgs"

if (-not $NoRelay) {
    $RelayScript = Join-Path $ProjectDir "Tools\Relay\relay.py"
    Start-Process cmd -ArgumentList "/k", "title Relay && python `"$RelayScript`" --source live --loop --rate $RelayRate"
}

# Give the server a moment to open its streamer port; the game retries the connection anyway.
Start-Sleep -Seconds 3

$GameArgs = @(
    "`"$ProjectFile`"",
    "/Game/Maps/L_TwinStudio",
    "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY",
    "-PixelStreamingConnectionURL=ws://127.0.0.1:$StreamerPort",
    "-AllowPixelStreamingCommands",
    "-log"
)
Start-Process $EditorExe -ArgumentList $GameArgs

Write-Host "Open http://localhost:$PlayerPort in a browser and click to start the stream."

# Demo video script (60–90 s)

One take of the remote-operations dashboard, cut once. It shows three things in order: the link dropping and recovering, the
overheating incident going Warning → Critical, and the engine derate command's round trip. Recorded on the personal laptop (it has the
car) after build page F (wrap-up item 2). On-screen text below is quoted from the code, so it should match the recording.

## Set-up

1. `Config/DefaultGame.ini`: `TelemetrySource=WebSocket`; Auto Engine Derate On Critical Coolant left on (the default).
2. Relay at 2× (warning lands ~45 s after it starts instead of ~91 s): `python Tools/Relay/relay.py --source live --rate 20`.
   No `--loop`: a loop would restart the trip mid-take.
3. Game, standalone at the recording size:
   `UnrealEditor.exe CarDigitalTwins.uproject /Game/Maps/L_TwinStudio -game -windowed -ResX=1920 -ResY=1080`.
4. OBS: window capture of the game, 1920×1080, 60 fps. Start recording once the link chip reads **Live**.

## Take (wall clock from the moment recording starts)

| Wall | Do | On screen | Caption (bottom left, 3–4 s) |
|------|----|-----------|------------------------------|
| 0:00 | Nothing; Overview view (key 1) | Car idling on the studio floor; top bar tell-tale "All signals normal"; link chip **Live** · "3 ms · 0 lost" | Vehicle telemetry twin · remote-operations dashboard · Unreal Engine 5.7, C++ |
| 0:06 | Stop the relay (Ctrl+C) | Link chip **Disconnected**, hollow amber dot, "retry in 0.5 s", then 1 s, 2 s; side panel and callouts dim (they show the last data received) | Link lost: backoff reconnect, stale data dimmed |
| ~0:12 | Start the relay again, same command | Link chip **Live** at the next retry (≤ 4 s); panels back to full; Events: "Trip restarted" | Back to Live |
| 0:20 | Let it drive (speed, gear and rpm rise on the Overview) | | |
| ~1:00 | Key 2 (Powertrain view) just before | Coolant crosses 105 °C: tell-tale **◆ WARNING** "Coolant 105.x °C"; hood ring pulses amber on the car; amber outline pulses slowly; Powertrain tab pip amber; coolant range-bar marker in the amber zone; Events: Coolant Warning | Overheating: Warning at 105 °C |
| ~1:14 | Nothing | Coolant reaches 115 °C: **◆ CRITICAL**, red outline pulsing fast, banner "CRITICAL · COOLANT 115.x °C" flashing, detail "Engine derate requested automatically"; Events: "Command 1 derate on sent (auto)" right above Coolant Critical | Critical at 115 °C: the twin requests an engine derate |
| ~1:15 | Watch the Powertrain panel | Command line "Command 1 derate on (auto) · sent, waiting for the ack", then "· applied by the vehicle from seq N · ack X ms"; Events: "Vehicle applied derate on from seq N"; drive mode chip **Engine derate**; rpm drops to the 2500 cap, speed to ≤ 50 km/h | The ack only says "accepted". The dashboard shows what the vehicle reports. |
| ~1:18 | Click **Acknowledge** on the banner | Banner stops flashing, button reads "Acknowledged"; Events: "Alarm acknowledged" | |
| ~1:21 | Nothing | Coolant falls below 112 °C: Critical → **Warning** (red → amber) and stays there | Derate protects the engine; it doesn't fix the cooling |
| ~1:26 | Stop recording | | |

The ~ times follow from the trip (SPEC.md §2.2: Warning ~91 s, Critical ~119 s of trip time; the restarted relay starts its trip when the game reconnects, ~0:14; at 2× Warning comes ~45 s later, Critical ~60 s
later, and the fall below 112 °C ~7 s after Critical) and shift with the reconnect; judge by the coolant value, not the clock.

## Edit (target 70–80 s)

- **Cut** 0:20 → ~0:55 (cruise, nothing changes): jump cut to just before Warning. Everything else stays real time.
- **End card, 4 s:** repo link, then three measured figures from PERFORMANCE.md: 5.0 ms frame time, 2.9 ms telemetry latency,
  101 automated tests. Name the machine in small type if the figures are re-measured with the car.
- **Captions:** state that the trip plays at 2× (caption at 0:20 or on the end card). Plain words; no "digital twin" as the lead,
  no claims about prediction.
- **Credit** on the end card: "Vehicle model: 2010 Jeep Wrangler Rubicon by Kostiantyn Abramov, GrabCAD Community Library". Present
  it as a vehicle model, not anything from Jeep.

## GIF for the README (6–8 s, loops)

From the same take: ~1:12 → ~1:19, Warning → Critical with the outline going amber → red and the banner appearing. 960 px wide,
15 fps.

## Check before recording

- The hood ring and the RR wheel ring sit on the car and are round (item 2).
- The cameras are framed and the lights retuned (item 2), since every shot uses them.
- A dry run without recording: the Warning and Critical times land as expected and the banner text fits its box.

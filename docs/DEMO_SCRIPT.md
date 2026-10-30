# Demo video script (60–90 s)

One take of the remote-operations dashboard, all at 1× (real time), cut in two places. It shows two things in order: the
overheating incident going Warning → Critical with the engine derate command's round trip, then the link dropping and recovering.
Recorded on the personal laptop (it has the car) after build page F (wrap-up item 2). On-screen text below is quoted from the code,
so it should match the recording. Order and 1× playback follow the outside review of 2026-10-30.

## Set-up

1. `Config/DefaultGame.ini`: `TelemetrySource=WebSocket`; Auto Engine Derate On Critical Coolant left on (the default).
2. Relay at the trip's own rate (1×): `python Tools/Relay/relay.py --source live`. No `--loop`: a loop would restart the trip
   mid-take.
3. Game, standalone at the recording size:
   `UnrealEditor.exe CarDigitalTwins.uproject /Game/Maps/L_TwinStudio -game -windowed -ResX=1920 -ResY=1080`.
4. OBS: window capture of the game, 1920×1080, 60 fps. Start recording once the link chip reads **Live**.

## Take (wall clock from the moment recording starts ≈ trip time)

| Wall | Do | On screen | Caption (bottom left, 3–4 s) |
|------|----|-----------|------------------------------|
| 0:00 | Nothing; Overview view (key 1) | Car on the studio floor, driving off; top bar tell-tale "All signals normal"; link chip **Live** · "3 ms · 0 lost" | Vehicle telemetry twin · remote-operations dashboard · Unreal Engine 5.7, C++ |
| 0:06 | Keys 3, 4, then 1 (Tyres, Body, back to Overview), ~2 s each | The camera blends to each view; tabs, side panel and callouts follow | Four views, one camera per job |
| ~1:28 | Key 2 (Powertrain view) | Coolant rising past 100 °C on the range bar | Cooling failure: coolant rising |
| ~1:31 | Nothing | Coolant crosses 105 °C: tell-tale **◆ WARNING** "Coolant 105.x °C"; hood ring pulses amber on the car; amber outline pulses slowly; Powertrain tab pip amber; Events: Coolant Warning | Warning at 105 °C |
| ~1:59 | Nothing | Coolant reaches 115 °C: **◆ CRITICAL**, red outline pulsing fast, banner "CRITICAL · COOLANT 115.x °C" flashing, detail "Engine derate requested automatically"; Events: "Command 1 derate on sent (auto)" right above Coolant Critical | Critical at 115 °C: the twin requests an engine derate |
| ~2:00 | Watch the Powertrain panel | Command line "Command 1 derate on (auto) · sent, waiting for the ack", then "· applied by the vehicle from seq N · ack X ms"; Events: "Vehicle applied derate on from seq N"; drive mode chip **Engine derate**; rpm drops to the 2500 cap, speed to ≤ 50 km/h | The ack only says "accepted". The dashboard shows what the vehicle reports. |
| ~2:04 | Click **Acknowledge** on the banner | Banner stops flashing, button reads "Acknowledged"; Events: "Alarm acknowledged" | |
| ~2:13 | Nothing | Coolant falls below 112 °C: Critical → **Warning** (red → amber) and stays there | Derate protects the engine; it doesn't fix the cooling |
| ~2:18 | Key 1 (Overview), then stop the relay (Ctrl+C) | Link chip **Disconnected**, hollow amber dot, "retry in 0.5 s", then 1 s, 2 s; side panel and callouts dim (they show the last data received) | Link lost: backoff reconnect, stale data dimmed |
| ~2:26 | Start the relay again, same command | Link chip **Live** at the next retry (≤ 4 s); panels back to full; Events: "Trip restarted" (the relay starts a new trip, derate off) | Back to Live |
| ~2:32 | Stop recording | | |

The ~ times follow from the trip (SPEC.md §2.2: Warning ~91 s, Critical ~119 s of trip time; the relay starts its trip when the game
connects, just before recording starts; under derate the coolant is back below 112 °C ~14 s after Critical, SPEC §2.5). Judge by
the coolant value, not the clock.

## Edit (target 60–75 s)

- **Cut 1** ~0:13 → ~1:28 (cruise, nothing changes): jump cut to the Powertrain view with coolant rising.
- **Cut 2** ~1:40 → ~1:54 (Warning holds while coolant climbs to Critical): jump cut, the coolant value shows time has passed.
- Kept: ~13 s + ~12 s + ~38 s ≈ 63 s, plus the 4 s end card.
- Everything kept plays at 1×; no speed change anywhere, so no speed caption. A jump cut is a cut, not a time-lapse.
- **End card, 4 s:** repo link, then three measured figures from PERFORMANCE.md: 5.0 ms frame time (studio + dashboard, without the
  car, RTX 4060 Ti; replace with the figure measured with the car once item 2 has it), 2.9 ms telemetry latency, 101 automated tests.
- **Captions:** plain words; no "digital twin" as the lead, no claims about prediction.
- **Credit** on the end card: "Vehicle model: 2010 Jeep Wrangler Rubicon by Kostiantyn Abramov, GrabCAD Community Library". Present
  it as a vehicle model, not anything from Jeep.

## GIF for the README (6–8 s, loops)

From the same take: ~1:57 → ~2:04, Critical with the outline going amber → red, the banner appearing and the drive mode chip turning
**Engine derate**. 960 px wide, 15 fps. It goes above the diagram in the README.

## Check before recording

- `Content/Jeep/` backed up (PROGRESS.md checkpoint, step 0).
- The hood ring and the RR wheel ring sit on the car and are round (item 2).
- The cameras are framed and the lights retuned (item 2), since every shot uses them.
- A dry run without recording: the Warning and Critical times land as expected and the banner text fits its box.

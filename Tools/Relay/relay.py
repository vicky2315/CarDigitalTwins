"""Streams a recorded trip over WebSocket, one telemetry message per sample (docs/SPEC.md §2.3).

Stands in for the live link a real car would have (telematics unit → cloud → stream service → dashboards). The options break the
stream on purpose, so the Unreal receiver (Day 10) can be tested against what real networks do: messages lost in a tunnel, a link
that goes quiet, a car that never stops sending.

Every connected client sees the same live stream, like viewers of one car. Playback starts when the first client connects.
Messages from clients are logged: this is where commands for the vehicle will arrive (Twin completion T1).

Usage:
    python relay.py                                   # Data/Trips/trip_sample.json at its own rate on ws://127.0.0.1:8765
    python relay.py --loop --rate 50                  # 5x faster than the 10 Hz trip, never ends
    python relay.py --loop --drop-percent 5 --pause-after 30
"""

import argparse
import asyncio
import json
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from websockets.asyncio.server import broadcast, serve
from websockets.exceptions import ConnectionClosed

REPO_ROOT = Path(__file__).resolve().parents[2]

# The trip generator is a script, not a package: put its folder on the path to reuse its validator.
sys.path.insert(0, str(REPO_ROOT / "Tools" / "TripGenerator"))
import trip_generator  # noqa: E402

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8765
STATUS_LOG_INTERVAL_S = 5.0
MAX_CATCH_UP_S = 1.0                       # further behind than this (PC hitch): skip ahead instead of sending a burst


@dataclass
class RelaySettings:
    messages_per_second: float
    loop_trip: bool = False
    drop_percent: float = 0.0
    pause_after_s: float | None = None     # seconds of streaming before the link goes quiet once; None = never
    pause_duration_s: float = 10.0
    drop_random_seed: int | None = None    # same seed = same messages dropped, for repeatable tests


def load_trip(trip_path):
    """Reads a trip.json and checks it against SPEC.md §2 with the generator's own validator. Raises ValueError if it's wrong."""
    with Path(trip_path).open(encoding="utf-8") as trip_file:
        trip = json.load(trip_file)
    trip_generator.validate_trip(trip)
    if trip["schemaVersion"] != trip_generator.SCHEMA_VERSION:
        raise ValueError(f"schemaVersion {trip['schemaVersion']}, the relay speaks {trip_generator.SCHEMA_VERSION} (SPEC.md §2.4)")
    return trip


def current_unix_ms():
    return int(time.time() * 1000)


def build_hello_message(trip, messages_per_second):
    """First message to every new client: which car this is and how fast messages will come."""
    return json.dumps({"type": "hello", "schemaVersion": trip_generator.SCHEMA_VERSION, "vehicleId": trip["vehicleId"],
                       "messagesPerSecond": messages_per_second, "tripRateHz": trip["rateHz"]}, separators=(",", ":"))


def build_telemetry_message(vehicle_id, frame, sent_unix_ms):
    """One sample in the SPEC.md §2.3 envelope. sentUnixMs lets the receiver measure latency."""
    return json.dumps({"type": "telemetry", "schemaVersion": trip_generator.SCHEMA_VERSION, "vehicleId": vehicle_id,
                       "sentUnixMs": sent_unix_ms, "frame": frame}, separators=(",", ":"))


def frames_in_send_order(frames, loop_trip):
    """Yields the frames in order; with loop_trip it starts again at frame 0 forever (seq goes backwards, SPEC.md §2.3)."""
    while True:
        yield from frames
        if not loop_trip:
            return


def should_drop_message(drop_random_generator, drop_percent):
    """True for about drop_percent of calls: the message is 'lost on the way' and never sent."""
    return drop_random_generator.random() * 100.0 < drop_percent


class TelemetryRelay:
    """WebSocket server that plays one trip to every connected client and logs what the clients send back."""

    def __init__(self, trip, relay_settings):
        self.trip = trip
        self.relay_settings = relay_settings
        self.connected_clients = set()
        self.first_client_connected = asyncio.Event()
        self.drop_random_generator = random.Random(relay_settings.drop_random_seed)
        self.sent_message_count = 0
        self.dropped_message_count = 0
        self.received_client_message_count = 0
        self.loop_count = 0

    async def handle_client(self, client_connection):
        """Runs once per client for as long as it stays connected: greets it, then reads what it sends."""
        client_address = client_connection.remote_address
        try:
            # Hello first, then join the stream, so a client never sees telemetry before it knows the rate.
            await client_connection.send(build_hello_message(self.trip, self.relay_settings.messages_per_second))
            self.connected_clients.add(client_connection)
            print(f"Client connected: {client_address} ({len(self.connected_clients)} connected)")
            self.first_client_connected.set()
            async for client_message_text in client_connection:
                self.handle_client_message(client_address, client_message_text)
        except ConnectionClosed:
            pass    # the client vanished without a proper close (killed, network gone): normal for a live link
        finally:
            # Also reached when the client drops without a goodbye; the stream simply stops going to it.
            self.connected_clients.discard(client_connection)
            print(f"Client disconnected: {client_address} ({len(self.connected_clients)} connected)")

    def handle_client_message(self, client_address, client_message_text):
        """Messages going back towards the car. Only logged for now; Twin completion T1 turns them into commands."""
        self.received_client_message_count += 1
        try:
            client_message = json.loads(client_message_text)
        except json.JSONDecodeError:
            print(f"From {client_address}: not JSON, ignored: {client_message_text[:80]!r}")
            return
        message_type = client_message.get("type") if isinstance(client_message, dict) else None
        print(f"From {client_address}: type {message_type!r}: {client_message_text[:200]}")

    async def stream_trip(self):
        """Sends one frame every 1/messages_per_second seconds to all clients, applying the drop and pause options."""
        await self.first_client_connected.wait()
        event_loop = asyncio.get_running_loop()
        send_interval_s = 1.0 / self.relay_settings.messages_per_second
        stream_start_time_s = event_loop.time()
        next_send_time_s = stream_start_time_s
        next_status_log_time_s = stream_start_time_s + STATUS_LOG_INTERVAL_S
        pause_already_done = self.relay_settings.pause_after_s is None
        previous_seq = -1
        print(f"Streaming {len(self.trip['frames'])} frames at {self.relay_settings.messages_per_second:g} messages/s"
              f"{', looping' if self.relay_settings.loop_trip else ''}")

        for frame in frames_in_send_order(self.trip["frames"], self.relay_settings.loop_trip):
            await asyncio.sleep(max(0.0, next_send_time_s - event_loop.time()))
            if event_loop.time() - next_send_time_s > MAX_CATCH_UP_S:
                next_send_time_s = event_loop.time()

            if not pause_already_done and event_loop.time() - stream_start_time_s >= self.relay_settings.pause_after_s:
                pause_already_done = True
                print(f"Pausing for {self.relay_settings.pause_duration_s:g} s (--pause-after): connections stay open, no data")
                await asyncio.sleep(self.relay_settings.pause_duration_s)
                print("Resuming")
                next_send_time_s = event_loop.time()

            if frame["seq"] < previous_seq:
                self.loop_count += 1
                print(f"Trip looped ({self.loop_count})")
            previous_seq = frame["seq"]

            # A dropped frame still uses up its time slot: the car sent it, the network lost it, so the receiver sees a seq gap.
            if should_drop_message(self.drop_random_generator, self.relay_settings.drop_percent):
                self.dropped_message_count += 1
            else:
                broadcast(self.connected_clients, build_telemetry_message(self.trip["vehicleId"], frame, current_unix_ms()))
                self.sent_message_count += 1
            next_send_time_s += send_interval_s

            if event_loop.time() >= next_status_log_time_s:
                next_status_log_time_s += STATUS_LOG_INTERVAL_S
                print(f"seq {frame['seq']}, sent {self.sent_message_count}, dropped {self.dropped_message_count}, "
                      f"{len(self.connected_clients)} clients")

        print(f"Trip finished: sent {self.sent_message_count}, dropped {self.dropped_message_count}")

    async def run(self, host, port, on_listening=None):
        """Starts the server, streams the trip, and closes every connection when a non-looping trip ends.
        on_listening(port) is called once the server accepts connections (port 0 = any free port, used by the tests)."""
        async with serve(self.handle_client, host, port) as websocket_server:
            listening_port = websocket_server.sockets[0].getsockname()[1]
            print(f"Relay listening on ws://{host}:{listening_port}, waiting for the first client")
            if on_listening:
                on_listening(listening_port)
            await self.stream_trip()

    def print_summary(self):
        print(f"Summary: sent {self.sent_message_count}, dropped {self.dropped_message_count}, loops {self.loop_count}, "
              f"messages from clients {self.received_client_message_count}")


def parse_arguments(argument_list=None):
    parser = argparse.ArgumentParser(description="Stream a trip.json over WebSocket (docs/SPEC.md §2.3).")
    parser.add_argument("--trip", type=Path, default=trip_generator.DEFAULT_OUTPUT_PATH, help="trip.json to play")
    parser.add_argument("--host", default=DEFAULT_HOST, help="address to listen on (default: this machine only)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--rate", type=float, default=None,
                        help="messages per second (default: the trip's own rateHz; higher plays the trip faster)")
    parser.add_argument("--loop", action="store_true", help="start again at frame 0 when the trip ends, forever")
    parser.add_argument("--drop-percent", type=float, default=0.0, help="lose this %% of messages at random (0-100)")
    parser.add_argument("--drop-seed", type=int, default=None, help="random seed for --drop-percent, for repeatable runs")
    parser.add_argument("--pause-after", type=float, default=None, metavar="SECONDS",
                        help="go quiet once after this many seconds of streaming (connections stay open)")
    parser.add_argument("--pause-duration", type=float, default=10.0, metavar="SECONDS", help="how long --pause-after stays quiet")
    arguments = parser.parse_args(argument_list)

    if arguments.rate is not None and arguments.rate <= 0:
        parser.error("--rate must be above 0")
    if not 0.0 <= arguments.drop_percent <= 100.0:
        parser.error("--drop-percent must be 0-100")
    return arguments


def main(argument_list=None):
    arguments = parse_arguments(argument_list)
    trip = load_trip(arguments.trip)
    relay_settings = RelaySettings(messages_per_second=arguments.rate or trip["rateHz"], loop_trip=arguments.loop,
                                   drop_percent=arguments.drop_percent, pause_after_s=arguments.pause_after,
                                   pause_duration_s=arguments.pause_duration, drop_random_seed=arguments.drop_seed)
    telemetry_relay = TelemetryRelay(trip, relay_settings)
    try:
        asyncio.run(telemetry_relay.run(arguments.host, arguments.port))
    except KeyboardInterrupt:
        print("Stopped (Ctrl+C)")
    telemetry_relay.print_summary()


if __name__ == "__main__":
    main()

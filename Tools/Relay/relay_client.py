"""Command-line client for relay.py: prints the stream and checks it the way the Unreal receiver (Day 10) will.

Counts lost messages from gaps in seq, notices trip loops (seq going backwards), measures latency from sentUnixMs, and says when
no data has arrived for a while. Sends one hello back on connect, to prove the return path to the car works (Twin completion T1).

Usage:
    python relay_client.py                     # ws://127.0.0.1:8765, prints every 10th message
    python relay_client.py --print-every 1
"""

import argparse
import asyncio
import json
import time
from dataclasses import dataclass, field

from websockets.asyncio.client import connect
from websockets.exceptions import ConnectionClosed

DEFAULT_RELAY_URL = "ws://127.0.0.1:8765"


@dataclass
class StreamCheckResults:
    telemetry_message_count: int = 0
    missing_message_count: int = 0         # seq gaps: messages the relay (or the network) lost
    loop_count: int = 0
    other_message_types: list = field(default_factory=list)
    latencies_ms: list = field(default_factory=list)
    last_seq: int | None = None


def check_telemetry_message(stream_check_results, telemetry_message, receive_unix_ms):
    """Updates the counters for one telemetry message. Returns a short note about seq ('gap of 3', 'trip looped') or ''."""
    seq = telemetry_message["frame"]["seq"]
    stream_check_results.telemetry_message_count += 1
    stream_check_results.latencies_ms.append(receive_unix_ms - telemetry_message["sentUnixMs"])

    seq_note = ""
    previous_seq = stream_check_results.last_seq
    if previous_seq is not None:
        if seq < previous_seq:
            stream_check_results.loop_count += 1
            seq_note = "trip looped"
        elif seq > previous_seq + 1:
            stream_check_results.missing_message_count += seq - previous_seq - 1
            seq_note = f"gap of {seq - previous_seq - 1}"
    stream_check_results.last_seq = seq
    return seq_note


def describe_frame(telemetry_message, latency_ms):
    frame = telemetry_message["frame"]
    return (f"seq {frame['seq']:5d}  t {frame['sampleTimeS']:6.1f} s  {frame['speedKmh']:5.1f} km/h  {frame['engineRpm']:4.0f} rpm  "
            f"coolant {frame['coolantTempC']:5.1f} C  latency {latency_ms} ms")


def print_results(stream_check_results):
    latencies_ms = stream_check_results.latencies_ms
    latency_text = (f"latency avg {sum(latencies_ms) / len(latencies_ms):.1f} ms, max {max(latencies_ms)} ms"
                    if latencies_ms else "no latency samples")
    print(f"Received {stream_check_results.telemetry_message_count} telemetry messages, missing {stream_check_results.missing_message_count}, "
          f"loops {stream_check_results.loop_count}, {latency_text}")


async def watch_relay(relay_url, print_every=10, stall_warning_s=1.0, max_telemetry_messages=None, quiet=False):
    """Connects, says hello, then reads until the relay closes or max_telemetry_messages arrived. Returns the results."""
    stream_check_results = StreamCheckResults()
    async with connect(relay_url) as relay_connection:
        print(f"Connected to {relay_url}")
        await relay_connection.send(json.dumps({"type": "hello", "client": "relay_client"}))
        while max_telemetry_messages is None or stream_check_results.telemetry_message_count < max_telemetry_messages:
            try:
                relay_message_text = await asyncio.wait_for(relay_connection.recv(), timeout=stall_warning_s)
            except asyncio.TimeoutError:
                print(f"No data for {stall_warning_s:g} s")
                continue
            except ConnectionClosed:
                print("Relay closed the connection")
                break

            receive_unix_ms = int(time.time() * 1000)
            relay_message = json.loads(relay_message_text)
            if relay_message.get("type") != "telemetry":
                # Unknown types are ignored by the receivers (SPEC.md §2.3); hello is shown so the setup is visible.
                stream_check_results.other_message_types.append(relay_message.get("type"))
                if not quiet:
                    print(f"{relay_message.get('type')}: {relay_message_text}")
                continue

            seq_note = check_telemetry_message(stream_check_results, relay_message, receive_unix_ms)
            if quiet:
                continue
            if seq_note:
                print(f"  ({seq_note})")
            if stream_check_results.telemetry_message_count % print_every == 0:
                print(describe_frame(relay_message, stream_check_results.latencies_ms[-1]))
    return stream_check_results


def main():
    parser = argparse.ArgumentParser(description="Watch and check the relay.py stream.")
    parser.add_argument("--url", default=DEFAULT_RELAY_URL)
    parser.add_argument("--print-every", type=int, default=10, help="print one line per N telemetry messages")
    parser.add_argument("--stall-warning", type=float, default=1.0, metavar="SECONDS", help="say so when nothing arrives this long")
    arguments = parser.parse_args()

    stream_check_results = StreamCheckResults()
    try:
        stream_check_results = asyncio.run(watch_relay(arguments.url, max(1, arguments.print_every), arguments.stall_warning))
    except KeyboardInterrupt:
        print("Stopped (Ctrl+C)")
    except OSError as connection_error:
        print(f"Could not connect to {arguments.url}: {connection_error}. Is relay.py running?")
        return
    print_results(stream_check_results)


if __name__ == "__main__":
    main()

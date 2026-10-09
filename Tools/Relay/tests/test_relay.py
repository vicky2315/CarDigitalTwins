"""Relay tests R1–R10 (docs/TESTS.md §4). R6 and R10 run a real relay and client on a free local port; no Unreal needed."""

import asyncio
import itertools
import json
import random

import pytest

import relay
import relay_client


def test_r1_telemetry_envelope_matches_spec(committed_sample_trip):
    first_frame = committed_sample_trip["frames"][0]
    telemetry_message = json.loads(relay.build_telemetry_message("jeep-01", first_frame, 1790577294123))

    assert list(telemetry_message.keys()) == ["type", "schemaVersion", "vehicleId", "sentUnixMs", "frame"]
    assert telemetry_message["type"] == "telemetry"
    assert telemetry_message["schemaVersion"] == 1
    assert telemetry_message["sentUnixMs"] == 1790577294123
    assert telemetry_message["frame"] == first_frame


def test_r2_frames_in_send_order_stops_or_loops(short_trip):
    frames = short_trip["frames"]
    assert list(relay.frames_in_send_order(frames, loop_trip=False)) == frames

    looped_frames = list(itertools.islice(relay.frames_in_send_order(frames, loop_trip=True), 2 * len(frames) + 1))
    assert [frame["seq"] for frame in looped_frames] == list(range(20)) * 2 + [0]


def test_r3_drop_percent_drops_about_that_share():
    drop_random_generator = random.Random(1)
    assert not any(relay.should_drop_message(drop_random_generator, 0.0) for _ in range(1000))
    assert all(relay.should_drop_message(drop_random_generator, 100.0) for _ in range(1000))

    dropped_count = sum(relay.should_drop_message(drop_random_generator, 10.0) for _ in range(10000))
    assert 900 <= dropped_count <= 1100


def test_r4_client_counts_gaps_loops_and_latency():
    stream_check_results = relay_client.StreamCheckResults()

    def telemetry_message_with_seq(seq):
        return {"type": "telemetry", "sentUnixMs": 1000, "frame": {"seq": seq}}

    notes = [relay_client.check_telemetry_message(stream_check_results, telemetry_message_with_seq(seq), 1012)
             for seq in [0, 1, 4, 5, 0]]

    assert notes == ["", "", "gap of 2", "", "trip looped"]
    assert stream_check_results.missing_message_count == 2      # a loop back to 0 is not a gap
    assert stream_check_results.loop_count == 1
    assert stream_check_results.latencies_ms == [12] * 5


def test_r5_load_trip_rejects_other_schema_version(tmp_path, short_trip):
    future_trip_path = tmp_path / "future_trip.json"
    future_trip_path.write_text(json.dumps({**short_trip, "schemaVersion": 2}), encoding="utf-8")

    with pytest.raises(ValueError, match="schemaVersion 2"):
        relay.load_trip(future_trip_path)


def test_r6_end_to_end_stream_reaches_client_in_order(short_trip):
    async def run_relay_and_client():
        listening_port_future = asyncio.get_running_loop().create_future()
        telemetry_relay = relay.TelemetryRelay(short_trip, relay.RelaySettings(messages_per_second=200.0))
        relay_task = asyncio.create_task(
            telemetry_relay.run("127.0.0.1", 0, on_listening=listening_port_future.set_result))
        listening_port = await listening_port_future

        stream_check_results = await relay_client.watch_relay(f"ws://127.0.0.1:{listening_port}", quiet=True)
        await asyncio.wait_for(relay_task, timeout=5.0)
        return telemetry_relay, stream_check_results

    telemetry_relay, stream_check_results = asyncio.run(asyncio.wait_for(run_relay_and_client(), timeout=10.0))

    assert stream_check_results.other_message_types == ["hello"]
    assert stream_check_results.telemetry_message_count == 20
    assert stream_check_results.last_seq == 19
    assert stream_check_results.missing_message_count == 0
    assert all(0 <= latency_ms < 1000 for latency_ms in stream_check_results.latencies_ms)
    assert telemetry_relay.received_client_message_count == 1    # the client's hello came back: the return path works


def live_relay(messages_per_second=10.0):
    return relay.TelemetryRelay(None, relay.RelaySettings(messages_per_second=messages_per_second, source=relay.SOURCE_LIVE))


def derate_command_text(command_id=7, enabled=True, **changed_keys):
    command_message = {"type": "command", "schemaVersion": 1, "commandId": command_id, "name": "engineDerate",
                       "enabled": enabled, "sentUnixMs": 1790577294123}
    command_message.update(changed_keys)
    return json.dumps(command_message)


def test_r7_command_ack_envelope_matches_spec():
    command_ack = json.loads(relay.build_command_ack_message(7, "applied", "", 1193))

    assert list(command_ack.keys()) == ["type", "commandId", "status", "reason", "appliedAtSeq"]
    assert command_ack == {"type": "commandAck", "commandId": 7, "status": "applied", "reason": "", "appliedAtSeq": 1193}


def test_r8_file_source_rejects_live_source_applies(short_trip, committed_sample_trip):
    file_relay = relay.TelemetryRelay(short_trip, relay.RelaySettings(messages_per_second=10.0))
    assert json.loads(file_relay.handle_client_message("test", derate_command_text())) == {
        "type": "commandAck", "commandId": 7, "status": "rejected", "reason": "source is a recorded trip", "appliedAtSeq": -1}

    telemetry_relay = live_relay()
    live_frames = telemetry_relay.frames_to_send()
    sent_frames = list(itertools.islice(live_frames, 50))
    assert sent_frames == committed_sample_trip["frames"][:50]     # live seed 42 without commands = the recorded sample
    assert telemetry_relay.handle_client_message("test", json.dumps({"type": "hello", "client": "x"})) is None

    first_ack = json.loads(telemetry_relay.handle_client_message("test", derate_command_text(7)))
    repeated_ack = json.loads(telemetry_relay.handle_client_message("test", derate_command_text(8)))
    assert (first_ack["status"], first_ack["appliedAtSeq"]) == ("applied", 50)
    assert (repeated_ack["status"], repeated_ack["appliedAtSeq"]) == ("applied", 50)    # idempotent: a retry is harmless
    assert next(live_frames)["driveMode"] == "EngineDerate"


@pytest.mark.parametrize("command_text, expected_command_id, expected_reason", [
    (derate_command_text(commandId="7"), -1, "bad command message"),
    (derate_command_text(commandId=True), -1, "bad command message"),
    (derate_command_text(schemaVersion=2), 7, "bad command message"),
    (derate_command_text(name="launchControl"), 7, "unknown command"),
    (derate_command_text(enabled="yes"), 7, "bad command message"),
])
def test_r9_bad_and_unknown_commands_are_rejected(command_text, expected_command_id, expected_reason):
    command_ack = json.loads(live_relay().handle_client_message("test", command_text))

    assert command_ack["commandId"] == expected_command_id
    assert command_ack["status"] == "rejected"
    assert command_ack["reason"] == expected_reason
    assert command_ack["appliedAtSeq"] == -1


def test_r10_end_to_end_derate_command_changes_live_stream():
    async def run_relay_and_client():
        listening_port_future = asyncio.get_running_loop().create_future()
        telemetry_relay = live_relay(messages_per_second=500.0)
        relay_task = asyncio.create_task(
            telemetry_relay.run("127.0.0.1", 0, on_listening=listening_port_future.set_result))
        listening_port = await listening_port_future

        stream_check_results = await relay_client.watch_relay(f"ws://127.0.0.1:{listening_port}", quiet=True,
                                                              max_telemetry_messages=60, send_derate_after_s=0.0)
        relay_task.cancel()    # a live trip runs for minutes; the client has seen enough
        return stream_check_results

    stream_check_results = asyncio.run(asyncio.wait_for(run_relay_and_client(), timeout=10.0))

    assert len(stream_check_results.command_acks) == 1                  # one ack, to the sender
    command_ack = stream_check_results.command_acks[0]
    assert (command_ack["commandId"], command_ack["status"]) == (1, "applied")
    assert command_ack["appliedAtSeq"] >= 0
    assert stream_check_results.first_engine_derate_seq == command_ack["appliedAtSeq"]

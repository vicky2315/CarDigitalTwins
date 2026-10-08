"""Tests for trip_generator.py. Plan and IDs (P1-P8): docs/TESTS.md §1. Open bugs they flag: docs/BUGS.md.

Run from the project folder:  python -m pytest Tools/TripGenerator/tests -v
"""

import copy
import json
import math

import pytest

import trip_generator
from conftest import COMMITTED_SAMPLE_TRIP_PATH

PARKED_PHASE_START_S = 170.0               # idle 10 + accelerate 20 + cruise 115 + brake 15 + idle 10 (SPEC.md §2.2)
TYRE_CRITICAL_LOW_KPA = 140.0              # SPEC.md §3


def first_sample_time_s(frames, condition):
    return next((frame["sampleTimeS"] for frame in frames if condition(frame)), None)


# P1: determinism -------------------------------------------------------------------------------------------------------------

def test_seed42_reproduces_committed_sample_byte_for_byte(generated_seed42_trip, tmp_path):
    regenerated_trip_path = tmp_path / "trip_seed42.json"
    trip_generator.write_trip(generated_seed42_trip, regenerated_trip_path)
    assert regenerated_trip_path.read_bytes() == COMMITTED_SAMPLE_TRIP_PATH.read_bytes(), \
        "Data/Trips/trip_sample.json is out of date with trip_generator.py: regenerate and commit it"


def test_same_seed_twice_gives_identical_trip(generated_seed42_trip):
    regenerated_trip, _ = trip_generator.generate_trip(seed=42, rate_hz=10)
    assert regenerated_trip == generated_seed42_trip


def test_different_seed_gives_different_trip(generated_seed42_frames):
    seed7_trip, _ = trip_generator.generate_trip(seed=7, rate_hz=10)
    assert seed7_trip["seed"] == 7
    assert seed7_trip["frames"] != generated_seed42_frames


# P2: committed sample is valid -----------------------------------------------------------------------------------------------

def test_committed_sample_passes_validation_and_has_expected_header():
    committed_trip = json.loads(COMMITTED_SAMPLE_TRIP_PATH.read_text(encoding="utf-8"))
    trip_generator.validate_trip(committed_trip)
    assert committed_trip["schemaVersion"] == 1
    assert committed_trip["vehicleId"] == "jeep-01"
    assert committed_trip["rateHz"] == 10
    assert committed_trip["seed"] == 42
    assert len(committed_trip["frames"]) == 1900          # 190 s at 10 Hz


# P3: other sample rates ------------------------------------------------------------------------------------------------------

@pytest.mark.parametrize("rate_hz", [5, 20])
def test_rate_that_divides_1000_validates_with_right_frame_count(rate_hz):
    trip, _ = trip_generator.generate_trip(seed=42, rate_hz=rate_hz)
    trip_generator.validate_trip(trip)
    assert len(trip["frames"]) == 190 * rate_hz


@pytest.mark.parametrize("rate_hz", [3, 7])
def test_rate_that_does_not_divide_1000_validates(rate_hz):
    trip, _ = trip_generator.generate_trip(seed=42, rate_hz=rate_hz)
    trip_generator.validate_trip(trip)


# P4: validator rejects bad input ---------------------------------------------------------------------------------------------

def delete_header_key(trip):
    del trip["vehicleId"]


def swap_first_two_frame_keys(trip):
    first_frame = trip["frames"][0]
    reordered_keys = [first_frame_key for first_frame_key in first_frame]
    reordered_keys[0], reordered_keys[1] = reordered_keys[1], reordered_keys[0]
    trip["frames"][0] = {frame_key: first_frame[frame_key] for frame_key in reordered_keys}


def skip_seq_number(trip):
    trip["frames"][2]["seq"] = 3


def move_sample_time_off_grid(trip):
    trip["frames"][1]["sampleTimeS"] = 0.15


def set_negative_speed(trip):
    trip["frames"][1]["speedKmh"] = -0.1


def set_gear_7(trip):
    trip["frames"][1]["gear"] = 7


def set_throttle_101(trip):
    trip["frames"][1]["throttlePct"] = 101.0


def set_negative_fuel(trip):
    trip["frames"][1]["fuelPct"] = -1.0


def delete_tyre_key(trip):
    del trip["frames"][1]["tyreKpa"]["rr"]


def set_unknown_openings_bit(trip):
    trip["frames"][1]["openings"] = 64


def set_unknown_drive_mode(trip):
    trip["frames"][1]["driveMode"] = "Derate"


@pytest.mark.parametrize("break_trip", [
    delete_header_key, swap_first_two_frame_keys, skip_seq_number, move_sample_time_off_grid, set_negative_speed, set_gear_7,
    set_throttle_101, set_negative_fuel, delete_tyre_key, set_unknown_openings_bit, set_unknown_drive_mode,
])
def test_validator_rejects_broken_trip(generated_seed42_trip, break_trip):
    short_trip = copy.deepcopy({**generated_seed42_trip, "frames": generated_seed42_trip["frames"][:5]})
    trip_generator.validate_trip(short_trip)                  # the unbroken short trip is valid
    break_trip(short_trip)
    with pytest.raises(ValueError):
        trip_generator.validate_trip(short_trip)


# P5: physics helpers ---------------------------------------------------------------------------------------------------------

def test_wheel_rpm_at_90_kmh():
    # 90 km/h = 25 m/s; circumference 2 * pi * 0.422 m = 2.6515 m; 25 / 2.6515 = 9.4286 rev/s = 565.72 rpm.
    assert trip_generator.wheel_rpm_from_speed(90.0) == pytest.approx(565.72, abs=0.01)
    assert trip_generator.wheel_rpm_from_speed(0.0) == 0.0


def test_engine_rpm_at_90_kmh_in_6th_gear():
    # Wheel rpm x 6th gear 0.84 x final drive 4.10 = 565.72 x 3.444 = 1948.3 rpm.
    assert trip_generator.engine_rpm_for_gear(90.0, 6) == pytest.approx(1948.3, abs=0.1)


def test_ease_toward_does_not_depend_on_step_size():
    one_step_value = trip_generator.ease_toward(0.0, 100.0, 4.0, 2.0)
    many_steps_value = 0.0
    for _ in range(20):
        many_steps_value = trip_generator.ease_toward(many_steps_value, 100.0, 4.0, 0.1)
    assert one_step_value == pytest.approx(100.0 * (1.0 - math.exp(-0.5)))
    assert many_steps_value == pytest.approx(one_step_value)


# P6: incident timing (SPEC.md §2.2) ------------------------------------------------------------------------------------------

def test_coolant_stays_normal_before_cooling_failure(generated_seed42_frames):
    frames_before_failure = [frame for frame in generated_seed42_frames if frame["sampleTimeS"] < trip_generator.OVERHEAT_START_S]
    assert max(frame["coolantTempC"] for frame in frames_before_failure) < 100.0


def test_coolant_warning_and_critical_times(generated_seed42_frames):
    coolant_warning_time_s = first_sample_time_s(generated_seed42_frames, lambda f: f["coolantTempC"] >= trip_generator.COOLANT_WARNING_C)
    coolant_critical_time_s = first_sample_time_s(generated_seed42_frames, lambda f: f["coolantTempC"] >= trip_generator.COOLANT_CRITICAL_C)
    assert coolant_warning_time_s == pytest.approx(91.0, abs=3.0)
    assert coolant_critical_time_s == pytest.approx(119.0, abs=3.0)
    assert coolant_critical_time_s < PARKED_PHASE_START_S     # goes red before the driver stops


def test_rear_right_tyre_warning_time_and_never_critical(generated_seed42_frames):
    tyre_warning_time_s = first_sample_time_s(generated_seed42_frames,
                                              lambda f: f["tyreKpa"]["rr"] < trip_generator.TYRE_WARNING_LOW_KPA)
    assert tyre_warning_time_s == pytest.approx(148.0, abs=3.0)
    assert min(frame["tyreKpa"]["rr"] for frame in generated_seed42_frames) > TYRE_CRITICAL_LOW_KPA


@pytest.mark.parametrize("healthy_wheel_key", ["fl", "fr", "rl"])
def test_only_rear_right_tyre_leaks(generated_seed42_frames, healthy_wheel_key):
    assert min(frame["tyreKpa"][healthy_wheel_key] for frame in generated_seed42_frames) > 230.0


def test_door_and_hood_open_after_parking(generated_seed42_frames):
    door_open_time_s = first_sample_time_s(generated_seed42_frames, lambda f: f["openings"] & trip_generator.OPENING_DOOR_FL)
    hood_open_time_s = first_sample_time_s(generated_seed42_frames, lambda f: f["openings"] & trip_generator.OPENING_HOOD)
    assert door_open_time_s == pytest.approx(PARKED_PHASE_START_S + trip_generator.DOOR_OPEN_AFTER_PARKED_S)
    assert hood_open_time_s == pytest.approx(PARKED_PHASE_START_S + trip_generator.HOOD_OPEN_AFTER_PARKED_S)
    assert {frame["openings"] for frame in generated_seed42_frames} == {0, 1, 17}  # nothing else opens, nothing closes


# P7: physical sanity ---------------------------------------------------------------------------------------------------------

def test_phases_in_spec_order(generated_seed42_trip_and_phases):
    _, phase_start_times_s = generated_seed42_trip_and_phases
    assert phase_start_times_s == [("idle", 0.0), ("accelerate", 10.0), ("cruise", 30.0), ("brake", 145.0), ("idle", 160.0),
                                   ("parked", PARKED_PHASE_START_S)]


def test_speed_never_negative_and_peaks_at_cruise_speed(generated_seed42_frames):
    assert min(frame["speedKmh"] for frame in generated_seed42_frames) >= 0.0
    assert max(frame["speedKmh"] for frame in generated_seed42_frames) == pytest.approx(90.0, abs=0.5)


def test_engine_off_exactly_while_parked(generated_seed42_frames):
    for frame in generated_seed42_frames:
        parked = frame["sampleTimeS"] >= PARKED_PHASE_START_S
        assert (frame["engineRpm"] == 0) == parked, f"seq {frame['seq']}: rpm {frame['engineRpm']}"


def test_running_engine_stays_between_idle_and_upshift(generated_seed42_frames):
    running_rpms = [frame["engineRpm"] for frame in generated_seed42_frames if frame["engineRpm"] > 0]
    assert min(running_rpms) >= trip_generator.IDLE_RPM - 20
    assert max(running_rpms) <= trip_generator.UPSHIFT_RPM + 20


def test_neutral_when_stopped(generated_seed42_frames):
    assert {frame["gear"] for frame in generated_seed42_frames if frame["speedKmh"] == 0.0} == {0}


def test_odometer_never_decreases_and_matches_distance(generated_seed42_frames):
    odometer_values_km = [frame["odometerKm"] for frame in generated_seed42_frames]
    assert all(later >= earlier for earlier, later in zip(odometer_values_km, odometer_values_km[1:]))
    # Distance = integral of speed: sum(km/h / 3600 * 0.1 s) over all frames.
    integrated_distance_km = sum(frame["speedKmh"] for frame in generated_seed42_frames) / 3600.0 * 0.1
    assert odometer_values_km[-1] - odometer_values_km[0] == pytest.approx(integrated_distance_km, abs=0.01)


def test_throttle_and_brake_never_both_pressed(generated_seed42_frames):
    both_pressed_seqs = [frame["seq"] for frame in generated_seed42_frames if frame["throttlePct"] > 0.0 and frame["brakePct"] > 0.0]
    assert both_pressed_seqs == []


def test_steering_within_wheel_limits(generated_seed42_frames):
    assert max(abs(frame["steerDeg"]) for frame in generated_seed42_frames) <= 40.0


def test_fuel_never_rises(generated_seed42_frames):
    fuel_values_pct = [frame["fuelPct"] for frame in generated_seed42_frames]
    assert all(later <= earlier for earlier, later in zip(fuel_values_pct, fuel_values_pct[1:]))


# P8: output format -----------------------------------------------------------------------------------------------------------

def test_written_file_has_one_frame_per_line_and_lf_endings(generated_seed42_trip, tmp_path):
    trip_path = tmp_path / "trip.json"
    trip_generator.write_trip(generated_seed42_trip, trip_path)
    trip_bytes = trip_path.read_bytes()
    assert b"\r\n" not in trip_bytes

    trip_lines = trip_bytes.decode("utf-8").splitlines()
    frame_count = len(generated_seed42_trip["frames"])
    assert len(trip_lines) == frame_count + 4                  # header, "frames": [, one line per frame, ], }
    for frame_line, frame in zip(trip_lines[2:2 + frame_count], generated_seed42_trip["frames"]):
        assert json.loads(frame_line.strip().rstrip(",")) == frame


def test_written_file_reads_back_equal(generated_seed42_trip, tmp_path):
    trip_path = tmp_path / "trip.json"
    trip_generator.write_trip(generated_seed42_trip, trip_path)
    assert json.loads(trip_path.read_text(encoding="utf-8")) == generated_seed42_trip


# P9: engine protection derate (Twin completion T1, SPEC.md §2.5) -------------------------------------------------------------

COOLANT_CRITICAL_SEQ = 1191                # first frame at or above 115 °C in the seed 42 trip (119.1 s)


def run_simulator_to_end(vehicle_simulator):
    frames = []
    while not vehicle_simulator.is_finished():
        frames.append(vehicle_simulator.step())
    return frames


def test_drive_mode_is_normal_without_a_command(generated_seed42_frames):
    assert {frame["driveMode"] for frame in generated_seed42_frames} == {trip_generator.DRIVE_MODE_NORMAL}


def test_set_engine_derate_returns_next_seq_and_repeating_it_changes_nothing():
    vehicle_simulator = trip_generator.VehicleSimulator(seed=42, rate_hz=10)
    for _ in range(50):
        vehicle_simulator.step()
    assert vehicle_simulator.set_engine_derate(True) == 50                  # the ack's appliedAtSeq: next frame produced
    assert vehicle_simulator.set_engine_derate(True) == 50                  # idempotent
    assert vehicle_simulator.step()["driveMode"] == trip_generator.DRIVE_MODE_ENGINE_DERATE


def test_derate_at_critical_caps_speed_and_brings_coolant_back_to_warning():
    vehicle_simulator = trip_generator.VehicleSimulator(seed=42, rate_hz=10)
    for _ in range(COOLANT_CRITICAL_SEQ):
        vehicle_simulator.step()
    vehicle_simulator.set_engine_derate(True)
    frames_after_derate = run_simulator_to_end(vehicle_simulator)
    derate_time_s = COOLANT_CRITICAL_SEQ / 10

    assert {frame["driveMode"] for frame in frames_after_derate} == {trip_generator.DRIVE_MODE_ENGINE_DERATE}
    assert all(frame["speedKmh"] <= 50.5 for frame in frames_after_derate if frame["sampleTimeS"] >= derate_time_s + 15.0)
    first_below_critical_clear_s = next(frame["sampleTimeS"] for frame in frames_after_derate if frame["coolantTempC"] < 112.0)
    assert first_below_critical_clear_s <= derate_time_s + 20.0             # measured 13.8 s
    engine_running_frames = [frame for frame in frames_after_derate
                             if derate_time_s + 20.0 <= frame["sampleTimeS"] < PARKED_PHASE_START_S]
    assert all(frame["coolantTempC"] >= 105.0 for frame in engine_running_frames)   # still Warning: derate protects, doesn't repair


def test_derate_caps_engine_rpm_while_accelerating():
    vehicle_simulator = trip_generator.VehicleSimulator(seed=42, rate_hz=10)
    vehicle_simulator.set_engine_derate(True)
    derated_frames = run_simulator_to_end(vehicle_simulator)
    assert max(frame["engineRpm"] for frame in derated_frames) <= trip_generator.DERATE_MAX_ENGINE_RPM
    assert max(frame["engineRpm"] for frame in derated_frames) > 2400        # the cap is actually reached, not just never tested


def test_switching_derate_off_restores_phase_speed():
    vehicle_simulator = trip_generator.VehicleSimulator(seed=42, rate_hz=10)
    for _ in range(400):                                                    # 40 s: cruising
        vehicle_simulator.step()
    vehicle_simulator.set_engine_derate(True)
    for _ in range(200):
        vehicle_simulator.step()
    vehicle_simulator.set_engine_derate(False)
    frames_after_release = [vehicle_simulator.step() for _ in range(300)]
    assert frames_after_release[0]["driveMode"] == trip_generator.DRIVE_MODE_NORMAL
    assert frames_after_release[-1]["speedKmh"] > 80.0

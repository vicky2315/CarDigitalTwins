"""Generates a believable, repeatable vehicle trip in the trip.json format of docs/SPEC.md §2.2.

The trip is a list of phases (idle, accelerate, cruise, brake, idle, parked). Each phase sets targets; the car eases toward them at
10 Hz. Two incidents make the demo interesting: the engine overheats during the cruise (amber, then red), and the rear-right tyre
slowly loses pressure. The driver pulls over and opens the hood.

Usage:
    python trip_generator.py                      # writes Data/Trips/trip_sample.json with seed 42
    python trip_generator.py --seed 7 --plot      # another trip, plus a check plot
"""

import argparse
import json
import math
import random
from dataclasses import dataclass
from pathlib import Path

SCHEMA_VERSION = 1
REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT_PATH = REPO_ROOT / "Data" / "Trips" / "trip_sample.json"

# sampleTimeS is written to the millisecond. The validator allows the resulting rounding error (up to half a millisecond) when it
# checks the rate grid, so rates that don't divide 1000 evenly (3 Hz → 0.333 s) still validate (BUGS.md BUG-001).
SAMPLE_TIME_DECIMALS = 3
SAMPLE_TIME_GRID_TOLERANCE_S = 0.5 * 10 ** -SAMPLE_TIME_DECIMALS + 1e-9    # + 1e-9 absorbs float error on top of the rounding

# Vehicle: 2010 Jeep Wrangler Rubicon, 6-speed manual. Demo values, not manufacturer data (SPEC.md §3).
TYRE_RADIUS_M = 0.422                      # measured on the tyre mesh in UE (AVehicleTwinActor), so speed and wheel spin agree
GEAR_RATIOS = [4.46, 2.61, 1.72, 1.25, 1.00, 0.84]
FINAL_DRIVE_RATIO = 4.10
IDLE_RPM = 780.0
UPSHIFT_RPM = 2600.0
DOWNSHIFT_RPM = 1250.0
MAX_ACCELERATION_MPS2 = 2.2
MAX_BRAKING_MPS2 = 3.5
STOPPING_BRAKING_MPS2 = (1.0, 2.8)         # min/max deceleration when slowing down: firm at speed, gentle just before the stop
CLUTCH_IN_BELOW_KMH = 15.0                 # stopping: clutch in (neutral, idle rpm) below this speed
NOMINAL_TYRE_KPA = 240.0

# Engine protection derate (SPEC.md §2.5): what the vehicle does while a derate command is active.
DERATE_MAX_ENGINE_RPM = 2500.0
DERATE_MAX_SPEED_KMH = 50.0
DERATE_FAILED_COOLING_TARGET_C = 108.0     # less load, less heat: with failed cooling it settles in Warning instead of reaching 125 °C
DERATE_FAILED_COOLING_TIME_CONSTANT_S = 25.0

# Values of the frame's driveMode (SPEC.md §2.1, EVehicleDriveMode in C++).
DRIVE_MODE_NORMAL = "Normal"
DRIVE_MODE_ENGINE_DERATE = "EngineDerate"

# EVehicleOpening bits (SPEC.md §2.1).
OPENING_DOOR_FL = 1
OPENING_HOOD = 16

# Status thresholds from SPEC.md §3, only used for the summary printout.
COOLANT_WARNING_C = 105.0
COOLANT_CRITICAL_C = 115.0
TYRE_WARNING_LOW_KPA = 180.0

# Keys every frame must have, in SPEC.md §2.1 order.
FRAME_KEYS = ["seq", "sampleTimeS", "speedKmh", "engineRpm", "gear", "throttlePct", "brakePct", "steerDeg", "odometerKm",
              "coolantTempC", "fuelPct", "batteryV", "tyreKpa", "openings", "driveMode"]


@dataclass
class TripPhase:
    name: str
    duration_s: float
    target_speed_kmh: float
    engine_running: bool = True


# About 3 minutes. The overheating starts during the cruise; "brake" is the driver pulling over because of it.
TRIP_PHASES = [
    TripPhase("idle", 10, 0),
    TripPhase("accelerate", 20, 90),
    TripPhase("cruise", 115, 90),
    TripPhase("brake", 15, 0),
    TripPhase("idle", 10, 0),
    TripPhase("parked", 20, 0, engine_running=False),
]

OVERHEAT_START_S = 70.0                    # trip time the cooling fails (40 s into the cruise)
SLOW_PUNCTURE_START_S = 40.0               # trip time the rear-right tyre starts leaking
SLOW_PUNCTURE_KPA_PER_S = 0.6              # warning (< 180 kPa) while pulling over, still above critical (140) at the end
DOOR_OPEN_AFTER_PARKED_S = 3.0
HOOD_OPEN_AFTER_PARKED_S = 8.0


def ease_toward(current, target, time_constant_s, delta_s):
    """Exponential approach: the same maths as a spring-arm camera lag. Independent of the sample rate."""
    return target + (current - target) * math.exp(-delta_s / time_constant_s)


def wheel_rpm_from_speed(speed_kmh):
    speed_mps = speed_kmh / 3.6
    return speed_mps / (2.0 * math.pi * TYRE_RADIUS_M) * 60.0


def engine_rpm_for_gear(speed_kmh, gear):
    return wheel_rpm_from_speed(speed_kmh) * GEAR_RATIOS[gear - 1] * FINAL_DRIVE_RATIO


def steer_deg_for_cruise(time_in_phase_s):
    """A gentle right-hand curve, then a left-hand one, as front road-wheel angles (+ = right)."""
    if 25.0 <= time_in_phase_s < 45.0:
        return 3.0 * math.sin(math.pi * (time_in_phase_s - 25.0) / 20.0)
    if 70.0 <= time_in_phase_s < 85.0:
        return -2.5 * math.sin(math.pi * (time_in_phase_s - 70.0) / 15.0)
    return 0.0


class VehicleSimulator:
    """The trip's vehicle, one sample at a time. generate_trip() runs it to the end in one go; the relay's live source steps it in
    real time, so a command (SPEC.md §2.5) can change what the next samples look like. Same seed and rate = same samples."""

    def __init__(self, seed, rate_hz):
        self.noise = random.Random(seed)
        self.rate_hz = rate_hz
        self.delta_s = 1.0 / rate_hz

        self.speed_kmh = 0.0
        self.gear = 0
        self.odometer_km = 12450.2
        self.coolant_temp_c = 88.5
        self.fuel_pct = 72.0
        self.battery_v = 14.1
        self.tyre_kpa = {"fl": 240.0, "fr": 241.0, "rl": 238.0, "rr": 240.0}
        self.openings = 0
        self.steer_deg = 0.0
        self.engine_derate_active = False

        self.seq = 0
        self.trip_time_s = 0.0
        self.phase_index = 0
        self.frame_in_phase = 0
        self.phase_start_times_s = [(TRIP_PHASES[0].name, 0.0)]
        self._skip_finished_phases()

    def frames_in_phase(self, phase):
        return int(round(phase.duration_s * self.rate_hz))

    def is_finished(self):
        return self.phase_index >= len(TRIP_PHASES)

    def set_engine_derate(self, enabled):
        """Engine protection derate on or off (SPEC.md §2.5). Setting the current value again changes nothing, so a repeated
        command is harmless. Returns the seq of the first frame produced under the new setting (the ack's appliedAtSeq)."""
        self.engine_derate_active = bool(enabled)
        return self.seq

    def _skip_finished_phases(self):
        """Moves on once the current phase has produced all its frames, recording each phase's start time (also for phases too
        short for one frame at this rate, as the old nested loops did)."""
        while not self.is_finished() and self.frame_in_phase >= self.frames_in_phase(TRIP_PHASES[self.phase_index]):
            self.phase_index += 1
            self.frame_in_phase = 0
            if not self.is_finished():
                self.phase_start_times_s.append((TRIP_PHASES[self.phase_index].name, self.trip_time_s))

    def step(self):
        """Simulates one sample and returns it as a SPEC.md §2.1 frame. Call only while not is_finished()."""
        phase = TRIP_PHASES[self.phase_index]
        noise = self.noise
        delta_s = self.delta_s
        seq = self.seq
        trip_time_s = self.trip_time_s
        time_in_phase_s = self.frame_in_phase * delta_s

        # The physics below works on local variables, written back at the end, so it reads the same as before the simulator existed.
        speed_kmh = self.speed_kmh
        gear = self.gear
        odometer_km = self.odometer_km
        coolant_temp_c = self.coolant_temp_c
        fuel_pct = self.fuel_pct
        battery_v = self.battery_v
        tyre_kpa = self.tyre_kpa
        openings = self.openings
        steer_deg = self.steer_deg

        # Derate caps the road speed; without it the phase's own target applies unchanged.
        target_speed_kmh = min(phase.target_speed_kmh, DERATE_MAX_SPEED_KMH) if self.engine_derate_active else phase.target_speed_kmh

        # Speed: speeding up eases toward the target, limited by what the car can do. Slowing down uses a near-constant
        # deceleration, like a driver braking for a stop; an exponential ease would creep towards zero for many seconds.
        if speed_kmh > target_speed_kmh + 0.5:
            speed_gap_mps = (speed_kmh - target_speed_kmh) / 3.6
            acceleration_mps2 = -max(STOPPING_BRAKING_MPS2[0], min(STOPPING_BRAKING_MPS2[1], speed_gap_mps / 2.0))
        else:
            desired_speed_kmh = ease_toward(speed_kmh, target_speed_kmh, 4.0, delta_s)
            acceleration_mps2 = (desired_speed_kmh - speed_kmh) / 3.6 / delta_s
        acceleration_mps2 = max(-MAX_BRAKING_MPS2, min(MAX_ACCELERATION_MPS2, acceleration_mps2))
        speed_kmh = max(0.0, speed_kmh + acceleration_mps2 * 3.6 * delta_s)
        if target_speed_kmh == 0.0 and speed_kmh < 0.3:
            speed_kmh = 0.0
            acceleration_mps2 = 0.0

        # Gear: neutral when stopped or about to stop, shift on rpm limits while moving.
        stopping = target_speed_kmh == 0.0
        if not phase.engine_running or speed_kmh < 1.0 or (stopping and speed_kmh < CLUTCH_IN_BELOW_KMH):
            gear = 0
        elif speed_kmh < 3.0:
            gear = max(gear, 1)
        else:
            gear = max(gear, 1)
            if gear < len(GEAR_RATIOS) and engine_rpm_for_gear(speed_kmh, gear) > UPSHIFT_RPM:
                gear += 1
            elif gear > 1 and engine_rpm_for_gear(speed_kmh, gear) < DOWNSHIFT_RPM:
                gear -= 1
            # Cruising: use the tallest gear that keeps the engine above the downshift point.
            if phase.name == "cruise":
                while gear < len(GEAR_RATIOS) and engine_rpm_for_gear(speed_kmh, gear + 1) > DOWNSHIFT_RPM + 300.0:
                    gear += 1

        # Throttle and brake from the acceleration; cruising needs throttle to hold speed against drag.
        if acceleration_mps2 >= 0.0 and phase.engine_running and speed_kmh > 0.0:
            road_load_pct = 8.0 + 0.22 * speed_kmh
            throttle_pct = road_load_pct + acceleration_mps2 / MAX_ACCELERATION_MPS2 * 60.0
            brake_pct = 0.0
        else:
            throttle_pct = 0.0
            brake_pct = -acceleration_mps2 / MAX_BRAKING_MPS2 * 100.0
        if speed_kmh == 0.0 and phase.engine_running:
            brake_pct = 100.0                                   # held on the brake while stopped, like the SPEC example
        throttle_pct = max(0.0, min(100.0, throttle_pct + noise.uniform(-1.0, 1.0) * (throttle_pct > 0.0)))
        brake_pct = max(0.0, min(100.0, brake_pct))

        # Engine rpm: idle in neutral, from the gearbox in gear (never below idle), off when parked.
        if not phase.engine_running:
            engine_rpm = 0.0
        elif gear == 0:
            engine_rpm = IDLE_RPM + noise.uniform(-15.0, 15.0)
        else:
            engine_rpm = max(IDLE_RPM, engine_rpm_for_gear(speed_kmh, gear)) + noise.uniform(-20.0, 20.0)
        if self.engine_derate_active:
            engine_rpm = min(engine_rpm, DERATE_MAX_ENGINE_RPM)

        # Steering: small corrections all the time, curves during the cruise.
        target_steer_deg = steer_deg_for_cruise(time_in_phase_s) if phase.name == "cruise" else 0.0
        steer_deg = ease_toward(steer_deg, target_steer_deg, 0.5, delta_s)
        reported_steer_deg = steer_deg + (noise.gauss(0.0, 0.15) if speed_kmh > 0.0 else 0.0)

        # Coolant: settles around 90-95 °C with load. After the cooling failure it heads for 125 °C while the engine runs; derated,
        # the lower load makes less heat, so it falls back to about 108 °C (Warning) but the failure stays.
        # Engine off: heat soak keeps it high, then it cools slowly.
        cooling_failed = trip_time_s >= OVERHEAT_START_S
        if not phase.engine_running:
            coolant_temp_c = ease_toward(coolant_temp_c, 25.0, 900.0, delta_s)
        elif cooling_failed and self.engine_derate_active:
            coolant_temp_c = ease_toward(coolant_temp_c, DERATE_FAILED_COOLING_TARGET_C, DERATE_FAILED_COOLING_TIME_CONSTANT_S, delta_s)
        elif cooling_failed:
            coolant_temp_c = ease_toward(coolant_temp_c, 125.0, 40.0, delta_s)
        else:
            coolant_temp_c = ease_toward(coolant_temp_c, 90.0 + 0.06 * throttle_pct, 60.0, delta_s)

        # Fuel: idle burn plus a share proportional to throttle. A 3-minute trip uses well under 1 %.
        if phase.engine_running:
            fuel_pct -= (0.0004 + 0.00006 * throttle_pct) * delta_s

        # Battery: alternator holds ~14.1 V while running, a little lower under electrical load at idle.
        # Engine off: resting voltage, sagging slightly with a door open (interior light).
        if phase.engine_running:
            battery_target_v = 14.1 if gear > 0 else 13.9
        else:
            battery_target_v = 12.6 - (0.05 if openings & OPENING_DOOR_FL else 0.0)
        battery_v = ease_toward(battery_v, battery_target_v, 3.0, delta_s)

        # Tyres warm up with speed (+~8 kPa at 90 km/h). The rear-right one leaks after the puncture.
        warm_up_kpa = 8.0 * min(1.0, speed_kmh / 90.0)
        for wheel_key in tyre_kpa:
            cold_kpa = {"fl": 240.0, "fr": 241.0, "rl": 238.0, "rr": 240.0}[wheel_key]
            tyre_kpa[wheel_key] = ease_toward(tyre_kpa[wheel_key], cold_kpa + warm_up_kpa, 120.0, delta_s)
        if trip_time_s >= SLOW_PUNCTURE_START_S:
            leak_offset_kpa = (trip_time_s - SLOW_PUNCTURE_START_S) * SLOW_PUNCTURE_KPA_PER_S
            tyre_kpa["rr"] = min(tyre_kpa["rr"], 240.0 + warm_up_kpa - leak_offset_kpa)

        # Openings: once parked, the driver gets out, then opens the hood to look at the engine.
        if phase.name == "parked":
            if time_in_phase_s >= DOOR_OPEN_AFTER_PARKED_S:
                openings |= OPENING_DOOR_FL
            if time_in_phase_s >= HOOD_OPEN_AFTER_PARKED_S:
                openings |= OPENING_HOOD

        odometer_km += speed_kmh / 3600.0 * delta_s

        frame = {
            "seq": seq,
            "sampleTimeS": round(trip_time_s, SAMPLE_TIME_DECIMALS),
            "speedKmh": round(speed_kmh, 2),
            "engineRpm": round(engine_rpm),
            "gear": gear,
            "throttlePct": round(throttle_pct, 1),
            "brakePct": round(brake_pct, 1),
            "steerDeg": round(reported_steer_deg, 2),
            "odometerKm": round(odometer_km, 3),
            "coolantTempC": round(coolant_temp_c + noise.uniform(-0.15, 0.15), 1),
            "fuelPct": round(fuel_pct, 2),
            "batteryV": round(battery_v + noise.uniform(-0.03, 0.03), 2),
            "tyreKpa": {wheel_key: round(value + noise.uniform(-0.4, 0.4), 1) for wheel_key, value in tyre_kpa.items()},
            "openings": openings,
            "driveMode": DRIVE_MODE_ENGINE_DERATE if self.engine_derate_active else DRIVE_MODE_NORMAL,
        }

        self.speed_kmh = speed_kmh
        self.gear = gear
        self.odometer_km = odometer_km
        self.coolant_temp_c = coolant_temp_c
        self.fuel_pct = fuel_pct
        self.battery_v = battery_v
        self.openings = openings
        self.steer_deg = steer_deg

        self.seq += 1
        self.trip_time_s = self.seq * delta_s                  # from seq, so rounding errors don't add up
        self.frame_in_phase += 1
        self._skip_finished_phases()
        return frame


def generate_trip(seed, rate_hz):
    vehicle_simulator = VehicleSimulator(seed, rate_hz)
    frames = []
    while not vehicle_simulator.is_finished():
        frames.append(vehicle_simulator.step())

    trip = {
        "schemaVersion": SCHEMA_VERSION,
        "vehicleId": "jeep-01",
        "rateHz": rate_hz,
        "seed": seed,
        "frames": frames,
    }
    return trip, vehicle_simulator.phase_start_times_s


def validate_trip(trip):
    """Checks the trip against SPEC.md §2. Raises ValueError on the first problem."""
    for header_key in ["schemaVersion", "vehicleId", "rateHz", "seed", "frames"]:
        if header_key not in trip:
            raise ValueError(f"header is missing '{header_key}'")
    expected_step_s = 1.0 / trip["rateHz"]
    for frame_index, frame in enumerate(trip["frames"]):
        if list(frame.keys()) != FRAME_KEYS:
            raise ValueError(f"frame {frame_index}: keys {list(frame.keys())} != SPEC {FRAME_KEYS}")
        if frame["seq"] != frame_index:
            raise ValueError(f"frame {frame_index}: seq {frame['seq']}")
        if abs(frame["sampleTimeS"] - frame_index * expected_step_s) > SAMPLE_TIME_GRID_TOLERANCE_S:
            raise ValueError(f"frame {frame_index}: sampleTimeS {frame['sampleTimeS']} is off the {trip['rateHz']} Hz grid")
        if frame["speedKmh"] < 0.0 or not -1 <= frame["gear"] <= 6:
            raise ValueError(f"frame {frame_index}: speed or gear out of range")
        for percent_key in ["throttlePct", "brakePct", "fuelPct"]:
            if not 0.0 <= frame[percent_key] <= 100.0:
                raise ValueError(f"frame {frame_index}: {percent_key} {frame[percent_key]} outside 0..100")
        if sorted(frame["tyreKpa"].keys()) != ["fl", "fr", "rl", "rr"]:
            raise ValueError(f"frame {frame_index}: tyreKpa keys {list(frame['tyreKpa'].keys())}")
        if frame["openings"] & ~63:
            raise ValueError(f"frame {frame_index}: unknown openings bits {frame['openings']}")
        if frame["driveMode"] not in (DRIVE_MODE_NORMAL, DRIVE_MODE_ENGINE_DERATE):
            raise ValueError(f"frame {frame_index}: unknown driveMode {frame['driveMode']!r}")


def first_time_s(frames, condition):
    return next((frame["sampleTimeS"] for frame in frames if condition(frame)), None)


def print_summary(trip, phase_start_times_s, output_path):
    frames = trip["frames"]
    duration_s = len(frames) / trip["rateHz"]

    def describe(time_s):
        return "never" if time_s is None else f"at {time_s:.1f} s"

    print(f"Wrote {output_path} ({output_path.stat().st_size / 1024:.0f} KB)")
    print(f"  {len(frames)} frames, {duration_s:.0f} s at {trip['rateHz']} Hz, seed {trip['seed']}")
    print("  phases: " + ", ".join(f"{name} {start_s:.0f} s" for name, start_s in phase_start_times_s))
    print(f"  max speed {max(f['speedKmh'] for f in frames):.1f} km/h, max rpm {max(f['engineRpm'] for f in frames)}, "
          f"distance {frames[-1]['odometerKm'] - frames[0]['odometerKm']:.2f} km")
    print(f"  coolant max {max(f['coolantTempC'] for f in frames):.1f} C; warning (>= {COOLANT_WARNING_C:.0f}) "
          f"{describe(first_time_s(frames, lambda f: f['coolantTempC'] >= COOLANT_WARNING_C))}, critical (>= {COOLANT_CRITICAL_C:.0f}) "
          f"{describe(first_time_s(frames, lambda f: f['coolantTempC'] >= COOLANT_CRITICAL_C))}")
    print(f"  tyre RR min {min(f['tyreKpa']['rr'] for f in frames):.1f} kPa; warning (< {TYRE_WARNING_LOW_KPA:.0f}) "
          f"{describe(first_time_s(frames, lambda f: f['tyreKpa']['rr'] < TYRE_WARNING_LOW_KPA))}")
    print(f"  door FL opens {describe(first_time_s(frames, lambda f: f['openings'] & OPENING_DOOR_FL))}, "
          f"hood opens {describe(first_time_s(frames, lambda f: f['openings'] & OPENING_HOOD))}")


def write_trip(trip, output_path):
    """One frame per line: small enough to commit, and git diffs show which frames changed."""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    header = {key: value for key, value in trip.items() if key != "frames"}
    header_json = json.dumps(header, separators=(", ", ": "))
    frame_lines = ",\n".join("    " + json.dumps(frame, separators=(", ", ": ")) for frame in trip["frames"])
    with output_path.open("w", encoding="utf-8", newline="\n") as output_file:
        output_file.write(header_json[:-1] + ',\n  "frames": [\n' + frame_lines + "\n  ]\n}\n")


def plot_trip(trip, phase_start_times_s, plot_path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    frames = trip["frames"]
    times_s = [f["sampleTimeS"] for f in frames]
    figure, axes = plt.subplots(4, 1, figsize=(11, 9), sharex=True)

    axes[0].plot(times_s, [f["speedKmh"] for f in frames], color="tab:blue")
    axes[0].set_ylabel("speed (km/h)")
    gear_axis = axes[0].twinx()
    gear_axis.step(times_s, [f["gear"] for f in frames], color="tab:gray", where="post", linewidth=0.8)
    gear_axis.set_ylabel("gear")

    axes[1].plot(times_s, [f["engineRpm"] for f in frames], color="tab:purple")
    axes[1].set_ylabel("engine rpm")

    axes[2].plot(times_s, [f["coolantTempC"] for f in frames], color="tab:red")
    axes[2].axhline(COOLANT_WARNING_C, color="orange", linestyle="--", linewidth=0.8, label="warning")
    axes[2].axhline(COOLANT_CRITICAL_C, color="red", linestyle="--", linewidth=0.8, label="critical")
    axes[2].set_ylabel("coolant (°C)")
    axes[2].legend(loc="upper left", fontsize=8)

    for wheel_key in ["fl", "fr", "rl", "rr"]:
        axes[3].plot(times_s, [f["tyreKpa"][wheel_key] for f in frames], label=wheel_key.upper(), linewidth=0.9)
    axes[3].axhline(TYRE_WARNING_LOW_KPA, color="orange", linestyle="--", linewidth=0.8)
    axes[3].set_ylabel("tyre (kPa)")
    axes[3].set_xlabel("trip time (s)")
    axes[3].legend(loc="lower left", fontsize=8, ncol=4)

    for axis in axes:
        for phase_name, start_s in phase_start_times_s:
            axis.axvline(start_s, color="black", alpha=0.15, linewidth=0.8)
    for phase_name, start_s in phase_start_times_s:
        axes[0].text(start_s + 1.0, axes[0].get_ylim()[1] * 0.92, phase_name, fontsize=8, alpha=0.7)

    figure.suptitle(f"Trip check: {trip['vehicleId']}, seed {trip['seed']}")
    figure.tight_layout()
    plot_path.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(plot_path, dpi=110)
    print(f"Plot: {plot_path}")


def main():
    parser = argparse.ArgumentParser(description="Generate a trip.json (docs/SPEC.md §2.2).")
    parser.add_argument("--seed", type=int, default=42, help="random seed; the same seed gives the same trip")
    parser.add_argument("--rate", type=int, default=10, help="samples per second")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUTPUT_PATH, help="output trip.json path")
    parser.add_argument("--plot", nargs="?", const="", default=None, metavar="PNG",
                        help="also save a check plot (default: next to the trip, .png)")
    arguments = parser.parse_args()

    trip, phase_start_times_s = generate_trip(arguments.seed, arguments.rate)
    validate_trip(trip)
    write_trip(trip, arguments.out)
    print_summary(trip, phase_start_times_s, arguments.out)

    if arguments.plot is not None:
        plot_path = Path(arguments.plot) if arguments.plot else arguments.out.with_suffix(".png")
        plot_trip(trip, phase_start_times_s, plot_path)


if __name__ == "__main__":
    main()

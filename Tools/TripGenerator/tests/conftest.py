"""Shared fixtures for the trip generator tests (docs/TESTS.md §1)."""

import sys
from pathlib import Path

import pytest

# The generator is a script, not a package: put its folder on the path so the tests can import it.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import trip_generator  # noqa: E402

COMMITTED_SAMPLE_TRIP_PATH = trip_generator.DEFAULT_OUTPUT_PATH


@pytest.fixture(scope="session")
def generated_seed42_trip_and_phases():
    """The default trip (seed 42, 10 Hz) and its phase start times, generated once for all tests."""
    return trip_generator.generate_trip(seed=42, rate_hz=10)


@pytest.fixture(scope="session")
def generated_seed42_trip(generated_seed42_trip_and_phases):
    return generated_seed42_trip_and_phases[0]


@pytest.fixture(scope="session")
def generated_seed42_frames(generated_seed42_trip):
    return generated_seed42_trip["frames"]

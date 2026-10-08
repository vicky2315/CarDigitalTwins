"""Shared fixtures for the relay tests (docs/TESTS.md §4)."""

import sys
from pathlib import Path

import pytest

# relay.py and relay_client.py are scripts, not a package: put their folder on the path so the tests can import them.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import relay  # noqa: E402


@pytest.fixture(scope="session")
def committed_sample_trip():
    return relay.load_trip(relay.trip_generator.DEFAULT_OUTPUT_PATH)


@pytest.fixture
def short_trip(committed_sample_trip):
    """The first 20 frames of the committed sample: still a valid trip, fast to stream."""
    return {**committed_sample_trip, "frames": committed_sample_trip["frames"][:20]}

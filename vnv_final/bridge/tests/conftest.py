import os
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


@pytest.fixture
def valid_packet_bytes() -> bytes:
    """Root frozen golden delta vector."""
    return bytes.fromhex(
        "aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 2d"
    )


@pytest.fixture
def corrupted_packet_bytes() -> bytes:
    """Root frozen corrupted delta vector."""
    return bytes.fromhex(
        "aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 d2"
    )

"""Ground-truth tests for the Python binding (cross-language reproducibility).

The golden canonical JSON is the same reference asserted by the native C++ test
(``tests/test_moa_verify.cc``), which is transwritten from the paper's
``repro/expected_moa.txt`` (produced by ``repro/aggregate_moa.py``).
"""
from __future__ import annotations

import json
import pathlib

from moa_verify import verify_csv, verify_file

DATA = pathlib.Path(__file__).resolve().parents[2] / "data"
SEALED_CSV = DATA / "moa_batch.csv"
EXPECTED_TXT = DATA / "expected_moa.txt"

GOLDEN = (
    '{"arms":6,"per":{'
    '"scope":{"mean":63.17,"min":45,"max":78},'
    '"evidence":{"mean":69.00,"min":62,"max":78},'
    '"assumptions":{"mean":45.33,"min":25,"max":62},'
    '"recovery":{"mean":39.83,"min":28,"max":58},'
    '"references":{"mean":68.33,"min":55,"max":82},'
    '"bias":{"mean":62.17,"min":45,"max":82},'
    '"cost":{"mean":51.83,"min":40,"max":74}},'
    '"batch_mean":57.10,"consensus":{"not_landable":6}}'
)


def test_sealed_batch_matches_golden():
    assert verify_file(SEALED_CSV) == GOLDEN


def test_golden_is_parseable_and_consistent():
    agg = json.loads(verify_csv(SEALED_CSV.read_text(encoding="utf-8")))
    assert agg["arms"] == 6
    assert agg["batch_mean"] == 57.10
    assert agg["consensus"] == {"not_landable": 6}
    assert agg["per"]["scope"] == {"mean": 63.17, "min": 45, "max": 78}


def test_matches_python_repro_evidence_file():
    """The batch mean printed by repro/aggregate_moa.py must appear verbatim."""
    assert "batch mean (F1-F7)   : 57.10" in EXPECTED_TXT.read_text(encoding="utf-8")


def test_tampered_input_changes_verdict_payload():
    """A changed score must change the canonical JSON (not silently ignored)."""
    tampered = SEALED_CSV.read_text(encoding="utf-8").replace(",62,78,55,58,", ",1,78,55,58,")
    assert verify_csv(tampered) != GOLDEN
    assert json.loads(verify_csv(tampered))["per"]["scope"]["min"] == 1


def test_empty_batch_is_well_defined():
    agg = json.loads(verify_csv("arm,scope,evidence,assumptions,recovery,references,bias,cost,top_verdict\n"))
    assert agg["arms"] == 0
    assert agg["batch_mean"] == 0.0

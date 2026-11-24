#!/usr/bin/env python3
import argparse
import json
import re
import statistics
from pathlib import Path

PARSE_RE = re.compile(r"STDLIB_TEST_TIMING completeParseMs\s+(\d+)")
STATEMENT_RE = re.compile(r"STDLIB_TEST_STATEMENTS\s+(\d+)")


def values(path, pattern):
    return [int(match.group(1)) for match in pattern.finditer(path.read_text(encoding="utf-8"))]


def summarize(path):
    parse_times = values(path, PARSE_RE)
    statements = values(path, STATEMENT_RE)
    if not parse_times:
        raise ValueError(f"No parse timing found in {path}")
    result = {
        "runs": len(parse_times),
        "medianMs": statistics.median(parse_times),
        "minMs": min(parse_times),
        "maxMs": max(parse_times),
        "samplesMs": parse_times,
    }
    if statements:
        result["statements"] = statements[-1]
    return result


def main():
    parser = argparse.ArgumentParser(description="Compare complete bits/stdc++.h parse baselines")
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    baseline = summarize(args.baseline)
    candidate = summarize(args.candidate)
    delta = candidate["medianMs"] - baseline["medianMs"]
    result = {
        "baseline": baseline,
        "candidate": candidate,
        "deltaMs": delta,
        "deltaPercent": delta * 100.0 / baseline["medianMs"],
    }
    if "statements" in baseline and "statements" in candidate:
        result["statementDelta"] = candidate["statements"] - baseline["statements"]
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

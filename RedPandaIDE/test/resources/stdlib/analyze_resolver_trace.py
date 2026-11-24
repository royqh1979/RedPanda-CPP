#!/usr/bin/env python3
import argparse
import base64
from collections import Counter
from pathlib import Path

TRACE_TYPES = {
    "SYNTAX_ATTEMPT", "RESOLVE_TYPE", "DECLARATION_BINDINGS", "FIND_DECLARATION",
    "MEMBER_DECLARATION", "FUNCTION_CANDIDATE", "MEMBER_CANDIDATES",
    "SELECT_SPECIALIZATION", "SPECIALIZATION_ATTEMPT", "RESOLVE_ARGUMENT",
}


def decode_trace(path):
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = raw.split(" ", 2)
        if len(parts) != 3 or parts[0] not in TRACE_TYPES:
            continue
        try:
            payload = base64.b64decode(parts[2]).decode("utf-8", "replace")
        except ValueError:
            continue
        yield parts[0], payload


def print_counter(title, counter, top):
    print(title)
    for text, count in counter.most_common(top):
        print(f"{count:6d}  {text}")


def analyze(path, top):
    print(f"=== {path} ===")
    attempts = Counter()
    cache_status = Counter()
    resolve_types = Counter()
    resolve_type_texts = Counter()
    bindings = Counter()
    declarations = Counter()
    candidates = Counter()
    for kind, payload in decode_trace(path):
        if kind == "SYNTAX_ATTEMPT":
            try:
                origin, remainder = payload.split("|", 1)
                operation, status, text = remainder.split("|", 2)
            except ValueError:
                continue
            attempts[operation] += 1
            cache_status[f"{operation} ({status})"] += 1
            if status == "miss":
                attempts[f"MISS {operation} | {text}"] += 1
        elif kind == "RESOLVE_TYPE":
            resolve_types[payload] += 1
            resolve_type_texts[payload.rsplit("|", 1)[-1]] += 1
        elif kind == "DECLARATION_BINDINGS":
            bindings[payload] += 1
        elif kind in {"FIND_DECLARATION", "MEMBER_DECLARATION"}:
            declarations[payload] += 1
        elif kind in {"FUNCTION_CANDIDATE", "MEMBER_CANDIDATES"}:
            candidates[payload] += 1
    print_counter("Cache attempts by operation", attempts, top)
    print_counter("Cache hit/miss by operation", cache_status, top * 2)
    print_counter("Most repeated resolveType text", resolve_type_texts, top)
    print_counter("Most repeated resolveType contexts", resolve_types, top)
    print_counter("Most repeated declarationBindings", bindings, top)
    print_counter("Most repeated declaration lookups", declarations, top)
    print_counter("Function/member candidates", candidates, top)


def main():
    parser = argparse.ArgumentParser(description="Analyze REDPANDA_PARSER_TRACE resolver logs")
    parser.add_argument("logs", type=Path, nargs="+")
    parser.add_argument("--top", type=int, default=10)
    args = parser.parse_args()
    for path in args.logs:
        analyze(path, args.top)


if __name__ == "__main__":
    main()

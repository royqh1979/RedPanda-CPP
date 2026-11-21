import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path

groups = defaultdict(list)
for name in sys.argv[2:]:
    for line in Path(name).read_text(encoding="utf-8").splitlines():
        marker = None
        if "DOCUMENT_EDIT_PROFILE," in line:
            marker = "DOCUMENT_EDIT_PROFILE,"
            suite = "document"
        elif "VIEW_EDIT_PROFILE," in line:
            marker = "VIEW_EDIT_PROFILE,"
            suite = "view"
        else:
            continue
        row = next(csv.reader([line.split(marker, 1)[1]]))
        operation, count, kind, position, sample = row[:5]
        count = int(count)
        if suite == "document":
            sync_ns = int(row[5])
            total_ns = sync_ns
            reparsed = int(row[6])
            entries = int(row[7])
            changed = int(row[8])
            fold_rescans = ""
            folding = ""
        else:
            sync_ns = int(row[5])
            total_ns = int(row[6])
            reparsed = int(row[7])
            entries = int(row[8])
            changed = int(row[9])
            fold_rescans = int(row[11])
            folding = int(row[12]) if len(row) > 12 else 1
        key = (suite, operation, count, kind, position, folding)
        groups[key].append((sync_ns, total_ns, reparsed, entries, changed, fold_rescans))

with Path(sys.argv[1]).open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(["suite", "operation", "count", "kind", "position", "folding",
                     "samples", "sync_ms", "total_ms", "reparsed_lines",
                     "change_entries", "changed_lines", "fold_rescans"])
    for key, values in sorted(groups.items(), key=lambda item: (item[0][0], item[0][1], item[0][5] == "", item[0][2], item[0][3], item[0][4])):
        sync = statistics.median(v[0] for v in values) / 1e6
        total = statistics.median(v[1] for v in values) / 1e6
        reparsed = statistics.median(v[2] for v in values)
        entries = statistics.median(v[3] for v in values)
        changed = statistics.median(v[4] for v in values)
        fold_values = [v[5] for v in values if v[5] != ""]
        fold_rescans = statistics.median(fold_values) if fold_values else ""
        writer.writerow([*key, len(values), f"{sync:.3f}", f"{total:.3f}",
                         reparsed, entries, changed, fold_rescans])

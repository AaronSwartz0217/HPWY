"""Inventory every reflected project class and its static callers; never edits code/assets."""
import json
import re
from pathlib import Path

project = Path(__file__).resolve().parents[1]
source = project / "Source/CrashCourse"
files = {p: p.read_text(encoding="utf-8-sig") for p in source.rglob("*") if p.suffix in (".h", ".cpp")}
classes = []
for path, text in files.items():
    if path.suffix != ".h":
        continue
    for marker in re.finditer(r"\bUCLASS\s*\(", text):
        pos, depth = marker.end(), 1
        while depth and pos < len(text):
            depth += (text[pos] == "(") - (text[pos] == ")")
            pos += 1
        match = re.match(r"\s*class\s+(?:CRASHCOURSE_API\s+)?(\w+)\s*:\s*([^\{]+)\{", text[pos:])
        if not match:
            continue
        name, parents = match.groups()
        runtime, tests = [], []
        for candidate, contents in files.items():
            if candidate == path:
                continue
            hits = []
            for line_no, line in enumerate(contents.splitlines(), 1):
                if not re.search(r"\b" + name + r"\b", line):
                    continue
                if re.match(r"\s*(?://|class\s+" + name + r"\s*;)", line):
                    continue
                if re.match(r"\s*(?:\w+[\s:*&]+)*" + name + r"::", line):
                    continue
                hits.append(line_no)
            if hits:
                row = {"file": candidate.relative_to(project).as_posix(), "lines": hits}
                (tests if "Tests" in candidate.parts else runtime).append(row)
        classes.append({"name": name, "nativePath": "/Script/CrashCourse." + name[1:],
                        "parent": re.sub(r"\bpublic\s+", "", parents).strip(),
                        "header": path.relative_to(project).as_posix(),
                        "line": text.count("\n", 0, marker.start()) + 1,
                        "runtimeReferences": runtime,
                        "externalRuntimeReferences": [r for r in runtime if Path(r["file"]).stem != path.stem],
                        "testReferences": tests,
                        "documentation": (project / "Docs/CPPClasses" / (name + ".md")).is_file()})
classes.sort(key=lambda row: row["name"])
header_counts = {}
for row in classes:
    header_counts[row["header"]] = header_counts.get(row["header"], 0) + 1
script_files = {p: p.read_text(encoding="utf-8-sig") for p in (project / "Scripts").glob("*.py")
                if not p.name.startswith("audit_cpp_")}
for row in classes:
    # Effects and their executor share CC_ConfiguredAbility.cpp: calls in that
    # file must not disappear just because the types share a header.
    if header_counts[row["header"]] > 1:
        row["externalRuntimeReferences"] = row["runtimeReferences"]
    token = re.compile(r"(?<!\w)" + re.escape(row["name"][1:]) + r"(?!\w)")
    row["scriptReferences"] = [p.relative_to(project).as_posix() for p, body in script_files.items() if token.search(body)]
lengths = sorted([{"file": path.relative_to(project).as_posix(), "lines": len(contents.splitlines())}
                  for path, contents in files.items() if path.suffix == ".cpp" and "Tests" not in path.parts],
                 key=lambda row: row["lines"], reverse=True)
result = {"schemaVersion": 1, "classes": classes, "classCount": len(classes), "implementationSizes": lengths,
          "notes": ["Static references are evidence of coupling, not proof of runtime reachability.",
                    "Blueprint parents/asset references require the separate Unreal registry audit."]}
out = project / "Docs/Architecture/CPP类清单.json"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print("Class count:", len(classes), "Missing docs:", sum(not c["documentation"] for c in classes))
for row in classes:
    print(row["name"], "<", row["parent"], "runtime files:", len(row["runtimeReferences"]), "tests:", len(row["testReferences"]))
print("Largest implementations:", json.dumps(lengths[:8], ensure_ascii=False))

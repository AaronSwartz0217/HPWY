"""Read-only asset-registry dependency audit, run with LanTu's UE 5.5."""
import unreal
import pathlib
import json
from collections import deque

DEST = pathlib.Path(r"C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main")
source = pathlib.Path(unreal.Paths.project_dir()).resolve()
content = source / "Content"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
registry.scan_paths_synchronous(["/Game/Rebuild"], force_rescan=True)
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
roots = [str(a.package_name) for a in registry.get_assets_by_path("/Game/Rebuild", recursive=True)]
queue = deque(roots)
seen = set()
records = []
external = set()
script = set()
missing = set()
engine = set()

def external_packages(package):
    relative = package.removeprefix("/Game/")
    result = []
    for kind in ["__ExternalActors__", "__ExternalObjects__"]:
        folder = content / kind / relative
        if folder.exists():
            for file in folder.rglob("*.uasset"):
                result.append("/Game/" + file.relative_to(content).with_suffix("").as_posix())
    return result

while queue:
    package = queue.popleft()
    if package in seen: continue
    seen.add(package)
    if package.startswith("/Script/"):
        script.add(package)
        continue
    if not package.startswith("/Game/"):
        engine.add(package)
        continue
    relative = package.removeprefix("/Game/")
    file = next((content / (relative + ext) for ext in [".uasset", ".umap"] if (content / (relative + ext)).exists()), None)
    if not file:
        missing.add(package)
        continue
    assets = registry.get_assets_by_package_name(package)
    kind = str(assets[0].asset_class_path.asset_name) if assets else "ExternalObject"
    dependencies = [str(d) for d in registry.get_dependencies(package, options)]
    queue.extend(dependencies)
    if file.suffix == ".umap":
        extra = external_packages(package)
        external.update(extra)
        queue.extend(extra)
    records.append({"package": package, "class": kind, "file": str(file), "bytes": file.stat().st_size, "dependencies": dependencies})

report = {"source": str(source), "roots": roots, "assets": records, "externalPackages": sorted(external), "scriptDependencies": sorted(script), "engineAndPluginDependencies": sorted(engine), "missingSourcePackages": sorted(missing), "totalBytes": sum(r["bytes"] for r in records)}
output = DEST / "Saved/LanTuRebuildAudit.json"
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("LANTU_REBUILD_AUDIT_OK: %d packages, %d bytes, %d external, missing=%s" % (len(records), report["totalBytes"], len(external), sorted(missing)))

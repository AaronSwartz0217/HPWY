"""Fresh-process serialized dependency and map audit for classified LanTu assets."""
import unreal
import pathlib
import json
from collections import Counter
DEST = pathlib.Path(r"C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main")
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
registry.scan_paths_synchronous(["/Game/LanTuRebuild"], force_rescan=True)
manifest = json.loads((DEST / "Docs/LanTu资源迁移清单.json").read_text(encoding="utf-8"))
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
stale = []
missing = set()
maps = []
packages = {r["new"] for r in manifest["assets"]}
for item in manifest["assets"]:
    path = item["new"]
    if not unreal.EditorAssetLibrary.does_asset_exist(path): missing.add(path)
    for dep in registry.get_dependencies(path, options):
        dep = str(dep)
        if dep.startswith("/Script/"): continue
        if dep.startswith("/Game/") and dep not in packages:
            stale.append({"asset": path, "dependency": dep})
        if not unreal.EditorAssetLibrary.does_asset_exist(dep): missing.add(dep)
    if item["class"] == "World": maps.append(path)
report = {"packages": len(packages), "missingPackages": sorted(missing), "outsideClassifiedRoot": stale, "maps": maps, "categories": manifest["categories"]}
file = DEST / ("Saved/LanTuMigrationVerify.json" if pathlib.Path(unreal.Paths.project_dir()).resolve() == DEST else "Saved/LanTuStageVerify.json")
file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("LANTU_VERIFY: packages=%d, missing=%d, outside=%d" % (len(packages), len(missing), len(stale)))
assert not missing and not stale, "Serialized dependencies need repair; inspect verification report"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report["loadedMaps"] = []
for path in maps:
    assert levels.load_level(path), path
    placed = actors.get_all_level_actors()
    report["loadedMaps"].append({"path": path, "actors": len(placed)})
    file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("LANTU_MAP_LOADED: %s actors=%d" % (path, len(placed)))
report["passed"] = True
file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("LANTU_MIGRATION_VERIFIED")

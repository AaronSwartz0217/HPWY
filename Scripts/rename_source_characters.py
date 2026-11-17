"""UE asset renames with redirects, exact-package backups and an audit manifest."""
import unreal
import json
import pathlib
import shutil
from collections import defaultdict

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game/SOURCE"], force_rescan=True)
assets = sorted(registry.get_assets_by_path("/Game/SOURCE", recursive=True), key=lambda a: str(a.package_name))
prefixes = {"SkeletalMesh": "SK", "Skeleton": "SKEL", "PhysicsAsset": "PHYS", "Material": "M", "MaterialInstanceConstant": "MI", "AnimSequence": "A", "AnimBlueprint": "ABP"}
counts = defaultdict(int)
plan = []
project = pathlib.Path(unreal.Paths.project_dir())
backup = project / "Saved" / "SourceNamingBackup"
for data in assets:
    kind = str(data.asset_class_path.asset_name)
    if kind not in prefixes:
        continue
    package = str(data.package_name)
    parts = package.split("/")
    if len(parts) < 6:
        continue
    character = parts[4]
    folder = str(data.package_path)
    key = (folder, prefixes[kind], character)
    counts[key] += 1
    name = prefixes[kind] + "_" + character + ("_%02d" % counts[key] if counts[key] > 1 else "")
    if str(data.asset_name) == name:
        continue
    target = folder + "/" + name
    assert not unreal.EditorAssetLibrary.does_asset_exist(target), "Target exists: " + target
    source_file = project / "Content" / (package.removeprefix("/Game/") + ".uasset")
    backup_file = backup / source_file.relative_to(project)
    backup_file.parent.mkdir(parents=True, exist_ok=True)
    if not backup_file.exists():
        shutil.copy2(source_file, backup_file)
    plan.append({"old": package, "new": target, "class": kind})
report_file = project / "Docs" / "资源重命名清单.json"
report_file.write_text(json.dumps(plan, ensure_ascii=False, indent=2), encoding="utf-8")
objects = [unreal.load_asset(item["old"]) for item in plan]
assert all(objects)
if plan:
    changes = [unreal.AssetRenameData(obj, item["new"].rsplit("/", 1)[0], item["new"].rsplit("/", 1)[1]) for obj, item in zip(objects, plan)]
    assert unreal.AssetToolsHelpers.get_asset_tools().rename_assets(changes), "UE rename reported a failure; inspect manifest"
    for obj in objects:
        unreal.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
summary = {"renamed": len(plan), "assets": []}
for data in assets:
    if str(data.asset_class_path.asset_name) == "SkeletalMesh":
        mesh = unreal.load_asset(str(data.package_name))
        summary["assets"].append({"path": mesh.get_path_name(), "bounds": str(mesh.get_bounds()), "skeleton": mesh.get_editor_property("skeleton").get_path_name()})
summary["api"] = {name: [item for item in dir(getattr(unreal, name)) if any(word in item.lower() for word in ["bone", "retarget", "chain", "auto", "animation"]) ] for name in ["IKRigController", "IKRetargeterController", "SkeletalMeshEditorSubsystem", "IKRetargetBatchOperation"] if hasattr(unreal, name)}
(project / "Saved" / "SourceNamingResult.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("SOURCE_NAMING_OK: " + str(summary))

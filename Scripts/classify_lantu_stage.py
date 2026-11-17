"""Relocate an isolated dependency closure using UE, never touch source/current art."""
import unreal
import json
import pathlib
from collections import Counter

DEST = pathlib.Path(r"C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main")
audit = json.loads((DEST / "Saved/LanTuRebuildAudit.json").read_text(encoding="utf-8"))
tools = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
recovered = []
for missing, replacement in [
    ("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny", "/Game/Characters/Mannequins/Meshes/SKM_Manny"),
    ("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Quinn", "/Game/Characters/Mannequins/Meshes/SKM_Quinn"),
    ("/Game/FirstPersonArms/Character/Mesh/SKM_Mannequin", "/Game/FirstPersonArms/Character/Mesh/SK_Mannequin_Arms"),
]:
    if not unreal.EditorAssetLibrary.does_asset_exist(missing):
        obj = unreal.EditorAssetLibrary.duplicate_asset(replacement, missing)
        assert obj, missing
        unreal.EditorAssetLibrary.save_loaded_asset(obj, False)
    recovered.append({"missing": missing, "replacement": replacement})

def category(kind):
    if kind in ["World", "HLODLayer", "LevelSequence"]: return "Maps"
    if kind == "StaticMesh": return "Models/StaticMeshes"
    if kind == "SkeletalMesh": return "Models/SkeletalMeshes"
    if kind in ["Skeleton", "PhysicsAsset", "ControlRigBlueprint"]: return "Models/Rigs"
    if "Niagara" in kind or kind in ["ParticleSystem", "ParticleEmitter"]: return "Effects"
    if kind in ["Material", "MaterialInstanceConstant", "MaterialFunction", "MaterialParameterCollection"]: return "Materials"
    if "Texture" in kind: return "Textures"
    if kind in ["AnimSequence", "AnimBlueprint", "BlendSpace", "BlendSpace1D", "PoseAsset"]: return "Animations"
    if kind == "WidgetBlueprint": return "UI"
    if kind == "Blueprint": return "Blueprints"
    if kind == "SoundWave": return "Audio"
    return "Data"

paths = [r["package"] for r in audit["assets"] if r["class"] != "ObjectRedirector"] + [r["missing"] for r in recovered]
objects = []
plan = []
for i, path in enumerate(paths):
    obj = unreal.load_asset(path)
    assert obj, "Cannot load: " + path
    kind = obj.get_class().get_name()
    relative = path.removeprefix("/Game/")
    if relative.startswith("Rebuild/"): relative = relative.removeprefix("Rebuild/")
    target = "/Game/LanTuRebuild/" + category(kind) + "/" + relative
    assert not unreal.EditorAssetLibrary.does_asset_exist(target), "Target exists: " + target
    objects.append(obj)
    plan.append({"old": path, "new": target, "class": kind})
    if i % 100 == 0: unreal.log("LANTU_STAGE_LOAD %d/%d" % (i, len(paths)))

manifest = {"source": audit["source"], "assets": plan, "recoveredMissingReferences": recovered, "categories": dict(Counter(category(p["class"]) for p in plan))}
(DEST / "Docs/LanTu资源迁移清单.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("LANTU_STAGE_RENAME_BEGIN: %d" % len(plan))
changes = [unreal.AssetRenameData(obj, item["new"].rsplit("/", 1)[0], item["new"].rsplit("/", 1)[1]) for obj, item in zip(objects, plan)]
assert tools.rename_assets(changes), "Some UE renames failed, inspect log before copying"
for i, obj in enumerate(objects):
    assert obj.get_path_name().split(".")[0] == plan[i]["new"]
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj, False), "Cannot save: " + obj.get_path_name()
    if i % 100 == 0: unreal.log("LANTU_STAGE_SAVE %d/%d" % (i, len(objects)))
(DEST / "Saved/LanTuStageReady.json").write_text(json.dumps({"ready": True, "packages": len(plan)}, indent=2), encoding="utf-8")
unreal.log("LANTU_STAGE_READY: " + str(manifest["categories"]))

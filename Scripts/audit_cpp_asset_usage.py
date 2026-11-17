"""Read-only registry/default-object audit. Does not play, save assets or touch player saves."""
import json
import re
from collections import Counter
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
inventory = json.loads((project / "Docs/Architecture/CPP类清单.json").read_text(encoding="utf-8"))
native = {row["nativePath"]: row["name"] for row in inventory["classes"]}
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
roots = ["/Game/UI/MainMenu/Maps/L_MainMenu", "/Game/LanTuRebuild/Maps/初始房间/restartlife",
         "/Game/LanTuRebuild/Maps/古风小镇/古风小镇", "/Game/HPWH/Maps/L_FlatTestArena",
         "/Game/HPWH/Blueprints/BP_ProgressionCharacter", "/Game/HPWH/Blueprints/BP_ProgressionController",
         "/Game/HPWH/Blueprints/BP_ProgressionPlayerState", "/Game/UI/MuralMenu/BP_MuralHUD",
         "/Game/UI/MainMenu/DA_HomeArt", "/Game/UI/MuralMenu/DA_MenuArt"]
reachable, queue = set(), list(roots)
while queue:
    package = queue.pop()
    if package in reachable or not package.startswith("/Game/"):
        continue
    reachable.add(package)
    queue.extend(str(dep) for dep in (registry.get_dependencies(package, options) or []))

def tag(asset, name):
    value = unreal.AssetRegistryHelpers.get_tag_value(asset, name)
    if isinstance(value, tuple):
        value = value[-1]
    return str(value) if value else ""

usage = {row["name"]: [] for row in inventory["classes"]}
blueprints, counts = [], Counter()
assets = registry.get_assets_by_path("/Game", recursive=True)
for asset in assets:
    package = str(asset.package_name)
    kind = str(asset.asset_class_path.asset_name)
    counts[kind] += 1
    class_path = str(asset.asset_class_path.package_name) + "." + str(asset.asset_class_path.asset_name)
    if class_path in native:
        usage[native[class_path]].append({"asset": package, "evidence": "native_data_asset", "reachable": package in reachable})
    if "Blueprint" not in kind:
        continue
    parent, native_parent = tag(asset, "ParentClass"), tag(asset, "NativeParentClass")
    combined = parent + " " + native_parent
    for path, name in native.items():
        if re.search(re.escape(path) + r"(?:['\s\"]|$)", combined):
            usage[name].append({"asset": package, "evidence": "blueprint_parent", "reachable": package in reachable})
    if "/Script/CrashCourse." in combined:
        blueprints.append({"asset": package, "parent": parent, "nativeParent": native_parent, "reachable": package in reachable})

defaults = []
def stable_value(value):
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if hasattr(value, "get_path_name"):
        return value.get_path_name()
    if isinstance(value, (list, tuple, unreal.Array)):
        return [stable_value(item) for item in value]
    return str(value)

for package in roots[4:8]:
    asset = unreal.load_asset(package)
    row = {"asset": package, "properties": {}}
    try:
        cdo = unreal.get_default_object(asset.generated_class())
        for name in ("startup_abilities", "initialize_attributes_effect", "progression_menu_class", "bShowProgressionPanel", "bShowDebugPanel"):
            try:
                row["properties"][name] = stable_value(cdo.get_editor_property(name))
            except Exception:
                pass
        for name in ("skills", "progression", "locomotion"):
            try:
                component = cdo.get_editor_property(name)
                row["properties"][name] = {"class": component.get_class().get_name()}
                for field in ("default_loadout", "basic_attack", "native_form", "artifact_forms", "skill_catalog", "library", "summoned_soldier_class"):
                    try:
                        row["properties"][name][field] = stable_value(component.get_editor_property(field))
                    except Exception:
                        pass
            except Exception:
                pass
    except Exception as error:
        row["error"] = str(error)
    defaults.append(row)

module_refs = [str(package) for package in (registry.get_referencers("/Script/CrashCourse", options) or []) if str(package).startswith("/Game/")]
# Registry parent tags do not include graph functions, notify classes or async
# nodes. Observe serialized native names too, conservatively: a saved reference
# is not proof that the node executes, and a missing name is not deletion proof.
serialized_names = {row["name"]: [] for row in inventory["classes"]}
for package in sorted(module_refs):
    base = project / "Content" / package.removeprefix("/Game/")
    path = next((base.with_suffix(ext) for ext in (".uasset", ".umap") if base.with_suffix(ext).is_file()), None)
    if path is None:
        continue
    data = path.read_bytes()
    for row in inventory["classes"]:
        if (row["name"][1:] + "\0").encode("ascii") in data:
            serialized_names[row["name"]].append({"asset": package, "reachable": package in reachable})
report = {"schemaVersion": 1, "classCount": len(native), "assetCount": len(assets),
          "assetTypes": dict(counts), "roots": roots, "reachablePackageCount": len(reachable),
          "nativeClassUsage": usage, "blueprintParents": blueprints, "activeDefaults": defaults,
          "scriptModuleReferencers": sorted(module_refs), "serializedNativeNames": serialized_names,
          "gameStarted": False, "assetsSaved": False,
          "limitations": ["Reachability is an asset dependency closure, not proof a Blueprint executes.",
                          "Native constructors and runtime load paths may add roots; zero direct references does not prove a class is removable.",
                          "Blueprint graph function/notify references may exist without a native-parent tag."]}
out = project / "Docs/Architecture/蓝图与资源引用清单.json"
out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
unreal.log("CPP_ARCHITECTURE_ASSET_AUDIT_READY " + str(out))
unreal.SystemLibrary.quit_editor()

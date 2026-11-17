"""Narrow repair of migrated default material references and the obsolete widget pin."""
import json
import runpy
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for script in ("configure_game_home.py", "configure_background_music.py"):
    runpy.run_path(str(project / "Scripts" / script), init_globals={"DEFER_EDITOR_EXIT": True})
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(["/Game/LanTuRebuild"], True)
original_path = "/InterchangeAssets/gltf/M_Default"
references = [str(p) for p in (reg.get_referencers(unreal.Name(original_path), unreal.AssetRegistryDependencyOptions()) or []) if str(p).startswith("/Game/")]
original = unreal.load_asset(original_path)
assert isinstance(original, unreal.Material)
target_path = "/Game/LanTuRebuild/Materials/Compatibility/M_GltfDefault_Nanite"
replacement = unreal.load_asset(target_path)
if not replacement:
    replacement = unreal.EditorAssetLibrary.duplicate_asset(original_path, target_path)
assert isinstance(replacement, unreal.Material)
replacement.set_editor_property("used_with_nanite", True)
unreal.MaterialEditingLibrary.recompile_material(replacement)
assert unreal.EditorAssetLibrary.save_loaded_asset(replacement, False)
changed = []
unhandled = []
for package in references:
    value = unreal.load_asset(package)
    if isinstance(value, unreal.StaticMesh):
        slots = list(value.get_editor_property("static_materials"))
        touched = False
        for slot in slots:
            if slot.get_editor_property("material_interface") == original:
                slot.set_editor_property("material_interface", replacement)
                touched = True
        if touched:
            value.set_editor_property("static_materials", slots)
            assert unreal.EditorAssetLibrary.save_loaded_asset(value, False), package
            changed.append(package)
    else:
        unhandled.append({"package": package, "class": value.get_class().get_name() if value else "Missing"})

# Some imported map/component references are not recorded as package referencers.
# Inspect the actual loaded components as well; only replace the exact glTF fallback.
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
changed_maps = []
seen_meshes = set(changed)
material_usage = {}
changed_instances = []
disabled_nanite = []
mesh_slot_plugin_parents = []
parents = {original.get_path_name(): replacement}
for parent_path, name in (("/InterchangeAssets/gltf/M_Transmission", "M_GltfTransmission_Nanite"), ("/Fab/Materials/Standard/M_MS_Base_Masked", "M_FabMasked_Nanite")):
    parent = unreal.load_asset(parent_path)
    if not parent:
        continue
    clone_path = "/Game/LanTuRebuild/Materials/Compatibility/" + name
    clone = unreal.load_asset(clone_path) or unreal.EditorAssetLibrary.duplicate_asset(parent_path, clone_path)
    clone.set_editor_property("used_with_nanite", True)
    unreal.MaterialEditingLibrary.recompile_material(clone)
    assert unreal.EditorAssetLibrary.save_loaded_asset(clone, False)
    parents[parent.get_path_name()] = clone
cloned_plugin_instances = []
def compatible_parent(parent, visited=None):
    if not parent:
        return None
    path = parent.get_path_name()
    if path in parents:
        return parents[path]
    visited = set() if visited is None else visited
    if path in visited or not isinstance(parent, unreal.MaterialInstanceConstant) or not (path.startswith("/InterchangeAssets/") or path.startswith("/Fab/")):
        return None
    visited.add(path)
    base = compatible_parent(parent.get_editor_property("parent"), visited)
    if not base:
        return None
    copy_path = "/Game/LanTuRebuild/Materials/Compatibility/" + parent.get_name() + "_Nanite"
    clone = unreal.load_asset(copy_path) or unreal.EditorAssetLibrary.duplicate_asset(parent.get_path_name(), copy_path)
    if clone.get_editor_property("parent") != base:
        unreal.MaterialEditingLibrary.set_material_instance_parent(clone, base)
        unreal.MaterialEditingLibrary.update_material_instance(clone)
        assert unreal.EditorAssetLibrary.save_loaded_asset(clone, False)
    parents[path] = clone
    cloned_plugin_instances.append(copy_path)
    return clone

def repair_parent(material):
    seen = set()
    while isinstance(material, unreal.MaterialInstanceConstant) and material.get_path_name().startswith("/Game/") and material.get_path_name() not in seen:
        seen.add(material.get_path_name())
        parent = material.get_editor_property("parent")
        clone = compatible_parent(parent)
        if clone:
            unreal.MaterialEditingLibrary.set_material_instance_parent(material, clone)
            unreal.MaterialEditingLibrary.update_material_instance(material)
            assert unreal.EditorAssetLibrary.save_loaded_asset(material, False)
            changed_instances.append(material.get_path_name())
            return
        material = parent
for map_path in ("/Game/LanTuRebuild/Maps/初始房间/restartlife", "/Game/LanTuRebuild/Maps/古风小镇/古风小镇", "/Game/HPWH/Maps/L_FlatTestArena"):
    assert levels.load_level(map_path)
    map_changed = False
    for actor in actors.get_all_level_actors():
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
            if not mesh:
                continue
            mesh_path = mesh.get_path_name().split(".")[0]
            if mesh_path not in seen_meshes and mesh_path.startswith("/Game/"):
                seen_meshes.add(mesh_path)
                slots = list(mesh.get_editor_property("static_materials"))
                touched = False
                for slot in slots:
                    material = slot.get_editor_property("material_interface")
                    parent_chain = []
                    parent = material
                    while isinstance(parent, unreal.MaterialInstanceConstant) and parent.get_path_name() not in parent_chain:
                        parent_chain.append(parent.get_path_name())
                        parent = parent.get_editor_property("parent")
                    if parent and parent.get_path_name() in parents:
                        mesh_slot_plugin_parents.append({"mesh": mesh_path, "material": material.get_path_name(), "base": parent.get_path_name()})
                    if material:
                        repair_parent(material)
                    if slot.get_editor_property("material_interface") == original:
                        slot.set_editor_property("material_interface", replacement)
                        touched = True
                if touched:
                    mesh.set_editor_property("static_materials", slots)
                    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, False), mesh_path
                    changed.append(mesh_path)
            overrides = list(component.get_editor_property("override_materials"))
            touched = False
            for index, material in enumerate(overrides):
                if material == original:
                    overrides[index] = replacement
                    touched = True
            if touched:
                component.set_editor_property("override_materials", overrides)
                map_changed = True
            for index in range(component.get_num_materials()):
                material = component.get_material(index)
                if material:
                    repair_parent(material)
                    material_usage[material.get_path_name()] = material_usage.get(material.get_path_name(), 0) + 1
            settings = mesh.get_editor_property("nanite_settings")
            if mesh_path.startswith("/Game/") and settings.get_editor_property("enabled"):
                # Nanite only supports opaque/masked shading. Retain translucent art on standard rendering.
                materials = [slot.get_editor_property("material_interface") for slot in mesh.get_editor_property("static_materials")]
                materials += [component.get_material(i) for i in range(component.get_num_materials())]
                if any(m and not unreal.CC_AnimationSetupLibrary.is_nanite_blend_supported(m) for m in materials):
                    settings.set_editor_property("enabled", False)
                    mesh.set_editor_property("nanite_settings", settings)
                    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
                    disabled_nanite.append(mesh_path)
    if map_changed:
        assert levels.save_current_level()
        changed_maps.append(map_path)

legacy_path = "/Game/LanTuRebuild/Blueprints/极客ugc资源/战斗玩法模块/BP_Enermyfree"
legacy = unreal.load_asset(legacy_path)
removed = unreal.CC_AnimationSetupLibrary.repair_legacy_widget_pins(legacy)
assert removed >= 0
assert unreal.EditorAssetLibrary.save_loaded_asset(legacy, False)
report = {"originalMaterial": original_path, "replacementMaterial": target_path,
          "referencersBefore": references, "changedMeshes": changed, "unhandledReferencers": unhandled,
          "obsoleteWidgetPinsRemoved": removed, "legacyBlueprint": legacy_path,
          "changedMaps": changed_maps, "componentMaterialUsage": material_usage,
          "changedMaterialInstances": changed_instances, "disabledNaniteForTransparentMeshes": disabled_nanite,
          "meshSlotPluginParentsBeforeRepair": mesh_slot_plugin_parents,
          "clonedPluginMaterialInstanceParents": cloned_plugin_instances,
          "engineAssetsEdited": False, "gameStarted": False}
out = project / "Saved/UI/TravelPresentationRepair.json"
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("TRAVEL_PRESENTATION_REPAIR_READY " + str(out))
unreal.SystemLibrary.quit_editor()

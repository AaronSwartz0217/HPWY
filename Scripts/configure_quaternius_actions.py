"""Retarget the CC0 Standard releases of Quaternius Universal Animation Library 1 and 2.

Extract UAL1_Standard.glb and UAL2_Standard.glb into Saved/NewAnimationPacks.
Run with Unreal Editor's -ExecutePythonScript option.
https://quaternius.com/packs/universalanimationlibrary.html
https://quaternius.com/packs/universalanimationlibrary2.html
"""
import json
from pathlib import Path
import runpy
import struct

import unreal


ROOT = "/Game/HPWH/Locomotion"
IMPORT_ROOT = "/Game/Developers/AnimationPackImport"
PACKS = {
    "UAL1": {"Spell_Simple_Shoot": "SpellCast", "Roll": "Roll"},
    "UAL2": {"Sword_Block": "Block", "Melee_Hook": "Hook"},
}
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
saved = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
shared = runpy.run_path(str(Path(__file__).with_name("configure_shared_skill_presentation.py")))
load = shared["load"]
save = shared["save"]
skeleton = shared["skeleton"]
repair_scale = shared["repair_guard_scale"]


def import_pack(pack, names):
    destination = f"{IMPORT_ROOT}/{pack}"
    folder = f"{destination}/{pack}_Selected/SkeletalMeshes"
    if not all(assets.does_asset_exist(f"{folder}/{name}") for name in names):
        path = saved / "NewAnimationPacks"
        data = (path / f"{pack}_Standard.glb").read_bytes()
        json_length = struct.unpack_from("<I", data, 12)[0]
        scene = json.loads(data[20:20 + json_length])
        scene["animations"] = [clip for clip in scene["animations"] if clip["name"] in names]
        if {clip["name"] for clip in scene["animations"]} != names:
            raise RuntimeError(f"Missing animations in {pack}")
        chunk = json.dumps(scene, separators=(",", ":")).encode()
        chunk += b" " * (-len(chunk) % 4)
        binary = data[20 + json_length:]
        filtered = path / f"{pack}_Selected.glb"
        filtered.write_bytes(
            struct.pack("<4sIII4s", b"glTF", 2, 20 + len(chunk) + len(binary), len(chunk), b"JSON")
            + chunk + binary
        )
        task = unreal.AssetImportTask()
        task.filename = str(filtered)
        task.destination_path = destination
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
    return load(f"{folder}/{pack}_Selected"), {name: load(f"{folder}/{name}") for name in names}


def get_or_create(name, kind, factory):
    return unreal.load_asset(f"{IMPORT_ROOT}/{name}") or tools.create_asset(name, IMPORT_ROOT, kind, factory)


def bake_pack(pack, mapping, entries):
    source, clips = import_pack(pack, set(mapping))
    rig = get_or_create(f"IK_{pack}", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(source)
    if not controller.apply_auto_generated_retarget_definition():
        raise RuntimeError(f"Could not generate humanoid chains for {pack}")
    save(rig)
    out = f"{ROOT}/Animations/Quaternius"
    baked = {}
    for key, entry in entries.items():
        pending = {
            name: f"{out}/A_{key}_{pack}_{suffix}"
            for name, suffix in mapping.items()
            if not assets.does_asset_exist(f"{out}/A_{key}_{pack}_{suffix}")
        }
        if pending:
            target = load(f"{ROOT}/IK_InitialInk_Target" if key == "InitialInk" else f"{ROOT}/IK_{key}")
            retargeter = get_or_create(f"RTG_{pack}_{key}", unreal.IKRetargeter, unreal.IKRetargetFactory())
            rc = unreal.IKRetargeterController.get_controller(retargeter)
            rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig)
            rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target)
            if not rc.get_num_retarget_ops():
                rc.add_default_ops()
            rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
            rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
            save(retargeter)
            result = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [unreal.AssetRegistryHelpers.create_asset_data(clips[name]) for name in pending],
                source, entry.mesh, retargeter, "", "", f"Bake_{key}_", "", False
            )
            packages = []
            for data in result:
                clip = data.get_asset()
                name = next(name for name in pending if name in clip.get_name())
                destination = pending[name]
                if not assets.rename_loaded_asset(clip, destination):
                    raise RuntimeError(f"Could not save {destination}")
                packages.append(load(destination).get_outermost())
            clip = None
            unreal.EditorLoadingAndSavingUtils.reload_packages(
                packages, unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE
            )
        baked[key] = {}
        for name, suffix in mapping.items():
            clip = load(f"{out}/A_{key}_{pack}_{suffix}")
            if not isinstance(clip, unreal.AnimSequence) or skeleton(clip) != skeleton(entry.mesh):
                raise RuntimeError(f"Unexpected skeleton for {clip.get_path_name()}")
            unreal.AnimationLibrary.remove_all_animation_notify_tracks(clip)
            clip.set_editor_property("enable_root_motion", False)
            repair_scale(clip, key)
            if key != "InitialInk":
                get_pose = unreal.AnimationLibrary.get_bone_pose_for_frame
                scale = get_pose(clip, "Hips", 0, False).scale3d.x
                idle_scale = get_pose(entry.idle, "Hips", 0, False).scale3d.x
                if abs(scale - idle_scale) > 0.01:
                    raise RuntimeError(f"Unexpected root scale for {clip.get_path_name()}")
            save(clip)
            baked[key][suffix] = clip
            unreal.log(f"NEW_ACTION_READY {key} {suffix} {clip.get_play_length():.3f}")
    return baked


def main():
    library = load(f"{ROOT}/DA_NPCLocomotion")
    entries = library.entries
    keys = {
        index: entry.jab.get_name().removeprefix("A_").removesuffix("_Jab")
        for index, entry in enumerate(entries) if entry.mesh and entry.jab
    }
    targets = {key: entries[index] for index, key in keys.items()}
    packs = {pack: bake_pack(pack, mapping, targets) for pack, mapping in PACKS.items()}
    for index, key in keys.items():
        entry = entries[index]
        entry.set_editor_property("cast_gesture", packs["UAL1"][key]["SpellCast"])
        entry.set_editor_property("guard_stance", packs["UAL2"][key]["Block"])
        entry.set_editor_property("cross_jab", packs["UAL2"][key]["Hook"])
        entries[index] = entry
    library.set_editor_property("entries", entries)
    save(library)
    unreal.log(f"NEW_ACTIONS_CONFIGURED {len(keys)} characters")
    unreal.SystemLibrary.quit_editor()


if __name__ == "__main__":
    main()

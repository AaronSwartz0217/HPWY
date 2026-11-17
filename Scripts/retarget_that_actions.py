"""Bake THAT action clips to eleven meshes without editing source animation.

THAT mixes UE4 and UE5 skeletons. Export each group with an exact source mesh.
"""
import hashlib
import json
import pathlib
import time
import unreal

ROOT = "/Game/HPWH/Locomotion"
NAMES = ["铜机械小兵", "铜械战士", "铜械将军", "铜械降神", "竹节人", "竹路人", "竹拳击手", "陶罐头", "陶童子", "陶土行者", "陶土僧人"]
CLIPS = {"Anim_Jump_Action": "Jump", "Anim_Falling_Reaction": "Fall", "Anim_Land_Reaction": "Land", "Anim_Death_Reaction": "Death", "Anim_Jab_Action": "Jab", "Anim_CrossJab_Action": "CrossJab", "Anim_Uppercut_Action": "Uppercut"}
tools = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)


def asset(name, cls, factory):
    result = unreal.load_asset(ROOT + "/" + name) or tools.create_asset(name, ROOT, cls, factory)
    assert result, "Could not create " + name
    return result


def skeleton_path(obj):
    return obj.get_editor_property("skeleton").get_path_name()


# Discover exact matches first. No source packages are saved by this script.
candidates = []
for directory in ["/Game/GameAnimsPack/Demo", "/Game/Characters"]:
    for data in registry.get_assets_by_path(directory, recursive=True):
        if str(data.asset_class_path.asset_name) == "SkeletalMesh":
            candidates.append(data.get_asset())
candidates.sort(key=lambda mesh: ("Simple" in mesh.get_name(), mesh.get_path_name()))
groups = {}
for source_name, kind in CLIPS.items():
    animation = unreal.load_asset("/Game/GameAnimsPack/Animations/" + source_name)
    assert isinstance(animation, unreal.AnimSequence), "Missing source clip " + source_name
    groups.setdefault(skeleton_path(animation), []).append((source_name, kind, animation))

report = []
for source_skeleton, animations in groups.items():
    source = next((mesh for mesh in candidates if skeleton_path(mesh) == source_skeleton), None)
    assert source, "No exact source mesh for skeleton: " + source_skeleton
    source_key = hashlib.sha1(source_skeleton.encode("utf-8")).hexdigest()[:8]
    source_rig = asset("IK_THAT_Source_" + source_key, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(source_rig)
    controller.set_skeletal_mesh(source)
    assert controller.apply_auto_generated_retarget_definition(), "Unrecognized humanoid: " + source.get_path_name()
    unreal.EditorAssetLibrary.save_loaded_asset(source_rig, False)
    for name in NAMES:
        mesh = unreal.load_asset("/Game/SOURCE/小怪/" + name + "/SK_" + name)
        assert isinstance(mesh, unreal.SkeletalMesh), "Missing target mesh " + name
        pending = [(original, kind, clip) for original, kind, clip in animations if not unreal.EditorAssetLibrary.does_asset_exist(ROOT + "/Animations/A_" + name + "_" + kind)]
        if pending:
            rig = unreal.load_asset(ROOT + "/IK_" + name)
            assert rig, "Run retarget_npc_locomotion.py first: " + name
            retarget = asset("RTG_THAT_" + source_key + "_" + name, unreal.IKRetargeter, unreal.IKRetargetFactory())
            rc = unreal.IKRetargeterController.get_controller(retarget)
            rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
            rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig)
            if rc.get_num_retarget_ops() == 0:
                rc.add_default_ops()
            rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
            rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
            unreal.EditorAssetLibrary.save_loaded_asset(retarget, False)
            results = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [unreal.AssetRegistryHelpers.create_asset_data(clip) for _, _, clip in pending],
                source, mesh, retarget, "", "", "Action_" + source_key + "_" + name + "_", "", False)
            for data in results:
                baked = data.get_asset()
                matches = [(original, kind) for original, kind, _ in pending if original in baked.get_name()]
                assert len(matches) == 1, "Unexpected retarget result: " + baked.get_path_name()
                original, kind = matches[0]
                target = ROOT + "/Animations/A_" + name + "_" + kind
                assert unreal.EditorAssetLibrary.rename_loaded_asset(baked, target), target
        # Clean interrupted exports which already reached their final path too.
        for original, kind, _ in animations:
            target = ROOT + "/Animations/A_" + name + "_" + kind
            baked = unreal.load_asset(target)
            assert isinstance(baked, unreal.AnimSequence), "Retarget omitted " + target
            assert skeleton_path(baked) == skeleton_path(mesh), "Wrong target skeleton: " + target
            # Damage/timing belong to the server ability, never legacy notifies.
            unreal.AnimationLibrary.remove_all_animation_notify_tracks(baked)
            unreal.EditorAssetLibrary.save_loaded_asset(baked, False)
            report.append({"source": original, "sourceMesh": source.get_path_name(), "sourceSkeleton": source_skeleton, "target": target, "targetSkeleton": skeleton_path(baked)})
        pathlib.Path(unreal.Paths.project_saved_dir() + "THATActionsRetarget.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.log("THAT_ACTIONS_MESH_READY: " + name + " source=" + source_key)
assert len(report) == 77, "Expected 77 baked action clips"
unreal.log("THAT_ACTIONS_RETARGET_READY: 77")
quit_at = time.monotonic() + 5.0


def quit_later(delta):
    if time.monotonic() >= quit_at:
        unreal.unregister_slate_post_tick_callback(quit_handle)
        unreal.SystemLibrary.quit_editor()


quit_handle = unreal.register_slate_post_tick_callback(quit_later)

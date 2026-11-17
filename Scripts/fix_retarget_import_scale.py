"""Repair imported Hips scale/child bind offsets in generated clips only.

IK export normalizes the imported root scale of 100, leaving a compensating
child offset. Keep rotations/motion and restore bind units. Safe to rerun.
"""
import json
from pathlib import Path
import unreal

NAMES = ["铜机械小兵", "铜械战士", "铜械将军", "铜械降神", "竹节人", "竹路人", "竹拳击手", "陶罐头", "陶童子", "陶土行者", "陶土僧人"]
KINDS = ["Idle", "Run", "Jump", "Fall", "Land", "Death", "Jab", "CrossJab", "Uppercut"]
report = []
for name in NAMES:
    rig = unreal.load_asset("/Game/HPWH/Locomotion/IK_" + name)
    assert rig, "Missing generated IK rig: " + name
    rig_controller = unreal.IKRigController.get_controller(rig)
    ref = rig_controller.get_ref_pose_transform_of_bone("Hips")
    for kind in KINDS:
        path = "/Game/HPWH/Locomotion/Animations/A_" + name + "_" + kind
        clip = unreal.load_asset(path)
        assert isinstance(clip, unreal.AnimSequence), "Missing generated clip: " + path
        model = clip.data_model_interface
        controller = clip.controller or unreal.new_object(unreal.AnimDataController, outer=clip)
        controller.set_model(model)
        frame_count = model.get_number_of_keys()
        first = unreal.AnimationLibrary.get_bone_pose_for_frame(clip, "Hips", 0, False)
        item = {"name": name, "kind": kind, "before": str(first), "ref": str(ref), "keys": frame_count, "rootFixed": False, "childOffsetsFixed": []}
        if first.scale3d.x < 2.0 and ref.scale3d.x > 10.0:
            poses = [unreal.AnimationLibrary.get_bone_pose_for_frame(clip, "Hips", frame, False) for frame in range(frame_count)]
            assert controller.set_bone_track_keys("Hips", [ref.translation + pose.translation * ref.scale3d for pose in poses], [pose.rotation for pose in poses], [ref.scale3d] * frame_count, False)
            item["rootFixed"] = True
        for bone in ["Spine", "LeftUpLeg", "RightUpLeg"]:
            child_ref = rig_controller.get_ref_pose_transform_of_bone(bone)
            local_ref = unreal.MathLibrary.make_relative_transform(child_ref, ref)
            first_child = unreal.AnimationLibrary.get_bone_pose_for_frame(clip, bone, 0, False)
            if first_child.translation.length() > local_ref.translation.length() * 10.0:
                rotations = [unreal.AnimationLibrary.get_bone_pose_for_frame(clip, bone, frame, False).rotation for frame in range(frame_count)]
                assert controller.set_bone_track_keys(bone, [local_ref.translation] * frame_count, rotations, [local_ref.scale3d] * frame_count, False)
                item["childOffsetsFixed"].append(bone)
        item["after"] = str(unreal.AnimationLibrary.get_bone_pose_for_frame(clip, "Hips", 0, False))
        if item["rootFixed"] or item["childOffsetsFixed"]:
            unreal.EditorAssetLibrary.save_loaded_asset(clip, False)
        report.append(item)
        Path(unreal.Paths.project_saved_dir() + "AnimationScaleFix.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("ANIMATION_SCALE_FIX_OK: " + str(len(report)))

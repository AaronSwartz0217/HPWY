"""Apply shared melee, casting and defensive presentation to the playable skill catalog."""
import hashlib
import json
from pathlib import Path

import unreal

ROOT = "/Game/HPWH"
ANIMS = ROOT + "/Locomotion"
PARTICLES = "/Game/StarterContent/Particles/"
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing asset: {path}")
    return asset


def save(asset):
    if not assets.save_loaded_asset(asset, False):
        raise RuntimeError(f"Could not save: {asset.get_path_name()}")


def skeleton(asset):
    return asset.get_editor_property("skeleton")


def repair_guard_scale(pose, key):
    if key == "InitialInk":
        return
    rig = load(ANIMS + "/IK_" + key)
    rig_controller = unreal.IKRigController.get_controller(rig)
    ref = rig_controller.get_ref_pose_transform_of_bone("Hips")
    model = pose.data_model_interface
    controller = pose.controller or unreal.new_object(unreal.AnimDataController, outer=pose)
    controller.set_model(model)
    frames = model.get_number_of_keys()
    changed = False
    first = unreal.AnimationLibrary.get_bone_pose_for_frame(pose, "Hips", 0, False)
    if first.scale3d.x < 2.0 and ref.scale3d.x > 10.0:
        keys = [unreal.AnimationLibrary.get_bone_pose_for_frame(pose, "Hips", i, False) for i in range(frames)]
        assert controller.set_bone_track_keys("Hips", [ref.translation + frame.translation * ref.scale3d for frame in keys],
                                              [frame.rotation for frame in keys], [ref.scale3d] * frames, False)
        changed = True
    for bone in ("Spine", "LeftUpLeg", "RightUpLeg"):
        local_ref = unreal.MathLibrary.make_relative_transform(rig_controller.get_ref_pose_transform_of_bone(bone), ref)
        first_child = unreal.AnimationLibrary.get_bone_pose_for_frame(pose, bone, 0, False)
        if first_child.translation.length() > local_ref.translation.length() * 10.0:
            rotations = [unreal.AnimationLibrary.get_bone_pose_for_frame(pose, bone, i, False).rotation for i in range(frames)]
            assert controller.set_bone_track_keys(bone, [local_ref.translation] * frames, rotations,
                                                  [local_ref.scale3d] * frames, False)
            changed = True
    if changed:
        save(pose)


def build_guard_stances():
    library = load(ANIMS + "/DA_NPCLocomotion")
    entries = library.entries
    source = load("/Game/GameAnimsPack/Animations/Anim_Attack_Idle")
    source_key = hashlib.sha1(skeleton(source).get_path_name().encode("utf-8")).hexdigest()[:8]
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    source_mesh = next((
        data.get_asset() for folder in ("/Game/GameAnimsPack/Demo", "/Game/Characters")
        for data in registry.get_assets_by_path(folder, recursive=True)
        if str(data.asset_class_path.asset_name) == "SkeletalMesh"
        and skeleton(data.get_asset()) == skeleton(source)
    ), None)
    ready = {}
    for index, entry in enumerate(entries):
        if not entry.mesh or not entry.jab:
            continue
        key = entry.jab.get_name().removeprefix("A_").removesuffix("_Jab")
        if entry.guard_stance:
            ready[key] = entry.guard_stance.get_path_name()
            continue
        destination = ANIMS + "/Animations/A_" + key + "_GuardStance"
        pose = unreal.load_asset(destination)
        if not pose:
            if skeleton(source) == skeleton(entry.mesh):
                pose = assets.duplicate_loaded_asset(source, destination)
            else:
                name = ("RTG_InitialInk_" + source_key) if key == "InitialInk" else ("RTG_THAT_" + source_key + "_" + key)
                retargeter = unreal.load_asset(ANIMS + "/" + name)
                if not source_mesh or not retargeter:
                    unreal.log_warning("GUARD_STANCE_MISSING_RETARGETER " + key + " " + source_key)
                    continue
                result = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                    [unreal.AssetRegistryHelpers.create_asset_data(source)],
                    source_mesh, entry.mesh, retargeter, "", "", "Guard_" + key + "_", "", False)
                if len(result) != 1:
                    unreal.log_warning("GUARD_STANCE_RETARGET_FAILED " + key)
                    continue
                pose = result[0].get_asset()
                assert assets.rename_loaded_asset(pose, destination), destination
                pose = None
                package = unreal.load_asset(destination).get_outermost()
                unreal.EditorLoadingAndSavingUtils.reload_packages(
                    [package], unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE)
                pose = load(destination)
            assert isinstance(pose, unreal.AnimSequence) and skeleton(pose) == skeleton(entry.mesh), destination
            unreal.AnimationLibrary.remove_all_animation_notify_tracks(pose)
            save(pose)
        assert skeleton(pose) == skeleton(entry.mesh), destination
        repair_guard_scale(pose, key)
        entry.set_editor_property("guard_stance", pose)
        entries[index] = entry
        ready[key] = pose.get_path_name()
    if ready:
        library.set_editor_property("entries", entries)
        save(library)
    return ready


def shield_material():
    folder = ROOT + "/Effects/SkillPresentation"
    name = "M_SharedSkillShield"
    material = unreal.load_asset(folder + "/" + name)
    if material:
        return material
    material = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    assert material
    edit = unreal.MaterialEditingLibrary
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, -120)
    color.set_editor_property("constant", unreal.LinearColor(.24, .78, .54, 1))
    rim = edit.create_material_expression(material, unreal.MaterialExpressionFresnel, -400, 90)
    rim.set_editor_property("base_reflect_fraction", .025)
    edit.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.connect_material_property(rim, "", unreal.MaterialProperty.MP_OPACITY)
    edit.recompile_material(material)
    save(material)
    return material


def main():
    sparks = load(PARTICLES + "P_Sparks")
    steam = load(PARTICLES + "P_Steam_Lit")
    dust = load(PARTICLES + "P_Ambient_Dust")
    smoke = load(PARTICLES + "P_Smoke")
    burst = load("/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/Hits/NS_Projectile_01_Hit")
    shield = shield_material()
    poses = unreal.CC_AttackPose
    attack = dict(attack_pose=poses.JAB, impact_particles=sparks, effect_scale=.35, effect_lifetime=.65)
    cast = dict(attack_pose=poses.CAST, cast_particles=steam, effect_scale=.45, effect_lifetime=.9)
    guard = dict(attack_pose=poses.GUARD, shield_visual_material=shield, effect_scale=.45, effect_lifetime=.7)
    plans = {
        "basic_straight_punch": attack,
        "basic_combo": dict(attack, alternate_attack_pose=True, animation_play_rate=1.15, effect_scale=.28),
        "basic_sweep": dict(attack, attack_pose=poses.CROSS_JAB, impact_particles=dust, effect_scale=.50),
        "basic_heavy_punch": dict(attack, attack_pose=poses.UPPERCUT, impact_particles=None, impact_niagara=burst, effect_scale=.40),
        "basic_particle": dict(cast, cast_particles=None),
        "basic_heal": dict(cast, cast_particles=None, impact_particles=steam),
        "bronze_summon": dict(cast, cast_particles=None, impact_particles=dust, effect_scale=.70),
        "bronze_evolve": dict(cast, cast_particles=sparks, impact_particles=steam),
        "bronze_charged_hit": dict(attack, attack_pose=poses.UPPERCUT, cast_particles=sparks,
                                   impact_particles=None, impact_niagara=burst, animation_play_rate=1.15, effect_scale=.55, effect_lifetime=1.0),
        "bronze_guard": dict(guard, cast_particles=sparks),
        "temple_golden_body": dict(guard, cast_particles=None),
        "temple_gravity": dict(cast, cast_particles=None, impact_particles=dust, effect_scale=.35, effect_lifetime=.22),
        "temple_lamps": dict(cast, cast_particles=None, impact_particles=sparks, effect_scale=.42, effect_lifetime=.35),
        "temple_cleanse": dict(guard, impact_particles=steam),
        "field_stealth": dict(cast, cast_particles=None, impact_particles=smoke, effect_scale=.40),
        "field_regrowth": dict(cast, cast_particles=None, impact_particles=steam, effect_scale=.40, effect_lifetime=.7),
        "field_lifesteal": dict(cast, impact_particles=sparks),
        "field_root_prison": dict(cast, cast_particles=dust, impact_particles=steam, animation_play_rate=.88, effect_scale=.75, effect_lifetime=1.4),
    }
    gallery = load("/Game/UI/MuralMenu/DA_SkillGallery")
    entries = {str(entry.design_id): entry.executable_skill for entry in gallery.entries}
    assert set(entries) == set(plans) and all(entries.values())
    report = {"guard_clips": build_guard_stances(), "skills": {}}
    for key, plan in plans.items():
        skill = entries[key]
        props = dict(animation_play_rate=1.0, alternate_attack_pose=False, cast_particles=None,
                     impact_particles=None, cast_niagara=None, impact_niagara=None, shield_visual_material=None)
        props.update(plan)
        for name, value in props.items():
            skill.set_editor_property(name, value)
        save(skill)
        report["skills"][key] = {"asset": skill.get_path_name(), "pose": str(props["attack_pose"]),
                                 "shield": bool(props["shield_visual_material"])}
    (Path(unreal.Paths.project_saved_dir()) / "SharedSkillPresentation.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log(f"SHARED_SKILL_PRESENTATION_READY {len(plans)} skills {len(report['guard_clips'])} guard clips")
    unreal.SystemLibrary.quit_editor()


if __name__ == "__main__":
    main()

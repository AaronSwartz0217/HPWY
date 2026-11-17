"""Configure initial Ink (75% baseline) and the white Nature interaction cube.

Run after compiling CrashCourseEditor, before import_mural_menu_assets.py. Changes only
HPWH configuration, generated animation assets and the existing arena;
imported art/material packages are never saved. Safe to run repeatedly.
"""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = "/Game/HPWH"
ANIM_ROOT = ROOT + "/Locomotion"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def data(name, cls, folder):
    result = unreal.load_asset(f"{ROOT}/{folder}/{name}")
    if result:
        return result
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    result = tools.create_asset(name, f"{ROOT}/{folder}", cls, factory)
    assert result, name
    return result


def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False), asset.get_path_name()


def generated(name, cls, factory):
    result = unreal.load_asset(ANIM_ROOT + "/" + name)
    if not result:
        result = tools.create_asset(name, ANIM_ROOT, cls, factory)
    assert result, name
    return result


# Preserve the original template pawn's model/transform/material defaults.
# The black initial material is intentionally left for the user's later art pass.
original_class = unreal.load_class(None, "/Game/THAT/BP_PlayerCharacter.BP_PlayerCharacter_C")
assert original_class, "Original template character is required"
original_mesh = unreal.get_default_object(original_class).get_editor_property("mesh")
source_template_mesh = original_mesh.get_editor_property("skeletal_mesh_asset")
assert source_template_mesh, "Original template mesh is missing"
# Own a mesh copy so a future black material edit affects only initial Ink.
template_mesh = unreal.load_asset(ROOT + "/Characters/SK_InitialInk_Template")
if not template_mesh:
    template_mesh = unreal.EditorAssetLibrary.duplicate_loaded_asset(source_template_mesh, ROOT + "/Characters/SK_InitialInk_Template")
assert template_mesh, "Could not create independent initial Ink mesh"
save(template_mesh)
target_skeleton = template_mesh.get_editor_property("skeleton")

# Build a template-specific six-state graph + DefaultSlot. This retains run,
# jump, death and the same three attack poses after leaving the bamboo model.
clip_sources = {
    "Idle": "/Game/GameAnimsPack/Animations/Anim_Idle_Idle",
    "Run": "/Game/GameAnimsPack/Animations/Anim_Jog_Action",
    "Jump": "/Game/GameAnimsPack/Animations/Anim_Jump_Action",
    "Fall": "/Game/GameAnimsPack/Animations/Anim_Falling_Reaction",
    "Land": "/Game/GameAnimsPack/Animations/Anim_Land_Reaction",
    "Death": "/Game/GameAnimsPack/Animations/Anim_Death_Reaction",
    "Jab": "/Game/GameAnimsPack/Animations/Anim_Jab_Action",
    "CrossJab": "/Game/GameAnimsPack/Animations/Anim_CrossJab_Action",
    "Uppercut": "/Game/GameAnimsPack/Animations/Anim_Uppercut_Action",
}
clips, groups = {}, {}
retargeted_paths = []
for kind, source_path in clip_sources.items():
    destination = ANIM_ROOT + "/Animations/A_InitialInk_" + kind
    clip = unreal.load_asset(destination)
    if not clip:
        source_clip = unreal.load_asset(source_path)
        assert isinstance(source_clip, unreal.AnimSequence), source_path
        skeleton = source_clip.get_editor_property("skeleton")
        if skeleton == target_skeleton:
            clip = unreal.EditorAssetLibrary.duplicate_loaded_asset(source_clip, destination)
        else:
            groups.setdefault(skeleton.get_path_name(), []).append((kind, source_clip, destination))
    if clip:
        clips[kind] = clip

if groups:
    target_rig = generated("IK_InitialInk_Target", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(target_rig)
    controller.set_skeletal_mesh(template_mesh)
    assert controller.apply_auto_generated_retarget_definition()
    save(target_rig)
    candidate_paths = [
        "/Game/GameAnimsPack/Demo/Mannequins/Meshes/SKM_Manny",
        "/Game/Characters/Mannequins/Meshes/SKM_Manny",
        "/Game/Characters/Mannequin_UE4/Meshes/SK_Mannequin",
    ]
    candidates = [unreal.load_asset(path) for path in candidate_paths]
    for skeleton_path, pending in groups.items():
        source_mesh = next((mesh for mesh in candidates if mesh and mesh.get_editor_property("skeleton").get_path_name() == skeleton_path), None)
        assert source_mesh, "No exact source mesh for " + skeleton_path
        source_key = hashlib.sha1(skeleton_path.encode("utf-8")).hexdigest()[:8]
        source_rig = generated("IK_InitialInk_Source_" + source_key, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
        controller = unreal.IKRigController.get_controller(source_rig)
        controller.set_skeletal_mesh(source_mesh)
        assert controller.apply_auto_generated_retarget_definition()
        save(source_rig)
        retarget = generated("RTG_InitialInk_" + source_key, unreal.IKRetargeter, unreal.IKRetargetFactory())
        controller = unreal.IKRetargeterController.get_controller(retarget)
        controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
        controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
        if controller.get_num_retarget_ops() == 0:
            controller.add_default_ops()
        controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
        controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
        save(retarget)
        results = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            [unreal.AssetRegistryHelpers.create_asset_data(clip) for _, clip, _ in pending],
            source_mesh, template_mesh, retarget, "", "", "InitialInk_" + source_key + "_", "", False)
        for result in results:
            clip = result.get_asset()
            matches = [(kind, dest) for kind, original, dest in pending if original.get_name() in clip.get_name()]
            assert len(matches) == 1, clip.get_path_name()
            kind, destination = matches[0]
            assert unreal.EditorAssetLibrary.rename_loaded_asset(clip, destination)
            clips[kind] = clip
            retargeted_paths.append(destination)

# UE 5.6 can leave FK ControlRig bound to the source USkeleton after retargeting
# between the two template skeletons with the same bone names. UpdateWithSkeleton
# only rebinds when bones/curves are removed, so a later PreSave validation fails.
# rename_loaded_asset has already persisted each generated package. Reload just
# those owned packages to run AnimationSequencerDataModel::PostLoad, which always
# rebinds the FK rig to its owning animation's actual skeleton. No source packages
# or model data are cleared. Interrupted runs load the existing target clips and
# skip retargeting, so they are automatically repaired by PostLoad as well.
if retargeted_paths:
    packages = [unreal.load_asset(path).get_outermost() for path in retargeted_paths]
    clips.clear()  # Do not retain stale Python object wrappers across package reload.
    clip = None
    packages_reloaded, reload_error = unreal.EditorLoadingAndSavingUtils.reload_packages(
        packages, unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE)
    assert packages_reloaded, "Could not rebind generated animation rigs: " + str(reload_error)
    clips = {kind: unreal.load_asset(ANIM_ROOT + "/Animations/A_InitialInk_" + kind) for kind in clip_sources}

assert len(clips) == 9, "Initial ink requires all nine template clips"
for kind, clip in clips.items():
    assert clip.get_editor_property("skeleton") == target_skeleton, kind
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(clip)
    save(clip)

animation_bp = unreal.load_asset(ANIM_ROOT + "/ABP_InitialInk_Template")
if not animation_bp:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CC_CharacterAnimInstance)
    factory.set_editor_property("target_skeleton", target_skeleton)
    animation_bp = tools.create_asset("ABP_InitialInk_Template", ANIM_ROOT, unreal.AnimBlueprint, factory)
assert unreal.CC_AnimationSetupLibrary.build_locomotion_graph(animation_bp, *[clips[kind] for kind in ["Idle", "Run", "Jump", "Fall", "Land", "Death"]])
save(animation_bp)
anim_class = animation_bp.generated_class()
library = unreal.load_asset(ANIM_ROOT + "/DA_NPCLocomotion")
assert library, "Existing locomotion library is required"
entries = [entry for entry in library.get_editor_property("entries") if entry.get_editor_property("mesh") != template_mesh]
entry = unreal.CC_MeshLocomotion()
entry.set_editor_property("mesh", template_mesh)
entry.set_editor_property("anim_class", anim_class)
entry.set_editor_property("reference_speed", 400.)
for kind, clip in clips.items():
    entry.set_editor_property("cross_jab" if kind == "CrossJab" else kind.lower(), clip)
entries.append(entry)
library.set_editor_property("entries", entries)
save(library)

nature = unreal.load_asset(ROOT + "/Forms/DA_Form_Native")
assert nature and str(nature.get_editor_property("form_id")) == "Native", "Stable Nature ID must remain Native"
nature.set_editor_property("available", True)
nature.set_editor_property("can_evolve", True)
save(nature)
balance = unreal.load_asset(ROOT + "/Balance/DA_CombatBalance")
assert balance, "Existing shared balance is required"
profiles = [row for row in balance.get_editor_property("profiles") if str(row.get_editor_property("profile_id")) != "InitialInk"]
row = unreal.CC_CombatProfile()
row.set_editor_property("profile_id", "InitialInk")
row.set_editor_property("health_multiplier", .75)
row.set_editor_property("attack_multiplier", .75)
profiles.append(row)
balance.set_editor_property("profiles", profiles)
save(balance)

initial = data("DA_Form_InitialInk", unreal.CC_FormDefinition, "Forms")
initial.set_editor_property("form_id", "InitialInk")
initial.set_editor_property("display_name", "墨水人")
initial.set_editor_property("available", True)
initial.set_editor_property("can_evolve", False)
loadout = data("DA_Loadout_InitialInk", unreal.CC_SkillLoadout, "Loadouts")
loadout.set_editor_property("slots", nature.get_editor_property("stages")[0].get_editor_property("loadout").get_editor_property("slots"))
save(loadout)
stages = initial.get_editor_property("stages")
for tier, stage in enumerate(stages):
    # Three identical stages satisfy the fixed-size form schema. bCanEvolve=false
    # is checked by the server; only tier zero is ever selectable for this form.
    for field, value in dict(cost=0, max_health=75., attack=11.25, max_mana=100., defense=0., move_speed=500.,
                             combat_balance=balance, stat_profile_id="InitialInk", loadout=loadout,
                             mesh=template_mesh, mesh_scale=original_mesh.get_editor_property("relative_scale3d"),
                             anim_class=anim_class, menu_display_name="墨水人").items():
        stage.set_editor_property(field, value)
    stages[tier] = stage
initial.set_editor_property("stages", stages)
save(initial)

player_bp = unreal.load_asset(ROOT + "/Blueprints/BP_ProgressionCharacter")
player = unreal.get_default_object(player_bp.generated_class())
component = player.get_editor_property("mesh")
component.set_editor_property("skeletal_mesh_asset", template_mesh)
for field in ["relative_location", "relative_rotation", "relative_scale3d"]:
    component.set_editor_property(field, original_mesh.get_editor_property(field))
# Each mesh uses its own asset material slots, including the independent Ink copy.
component.set_editor_property("override_materials", [])
component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
component.set_editor_property("anim_class", anim_class)
player.get_editor_property("locomotion").set_editor_property("library", library)
player.get_editor_property("skills").set_editor_property("default_loadout", loadout)
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
save(player_bp)
state_bp = unreal.load_asset(ROOT + "/Blueprints/BP_ProgressionPlayerState")
progression = unreal.get_default_object(state_bp.generated_class()).get_editor_property("progression")
progression.set_editor_property("native_form", initial)
catalog = [form for form in progression.get_editor_property("artifact_forms") if form and form != initial and form != nature]
catalog.append(nature)
progression.set_editor_property("artifact_forms", catalog)
unreal.BlueprintEditorLibrary.compile_blueprint(state_bp)
save(state_bp)

material_path = ROOT + "/Materials/MI_Pickup_Nature_White"
material = unreal.load_asset(material_path) or tools.create_asset("MI_Pickup_Nature_White", ROOT + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
unreal.MaterialEditingLibrary.set_material_instance_parent(material, unreal.load_asset(ROOT + "/Materials/M_TestTint"))
unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "Tint", unreal.LinearColor(1., 1., 1., 1.))
save(material)
artifact_bp = unreal.load_asset(ROOT + "/Blueprints/BP_Artifact_Nature")
if not artifact_bp:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CC_FormArtifact)
    artifact_bp = tools.create_asset("BP_Artifact_Nature", ROOT + "/Blueprints", unreal.Blueprint, factory)
artifact = unreal.get_default_object(artifact_bp.generated_class())
artifact.get_editor_property("artifact").set_editor_property("form", nature)
artifact.get_editor_property("artifact").set_editor_property("enabled", True)
artifact.get_editor_property("mesh").set_editor_property("static_mesh", unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
artifact.get_editor_property("mesh").set_material(0, material)
unreal.BlueprintEditorLibrary.compile_blueprint(artifact_bp)
save(artifact_bp)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(ROOT + "/Maps/L_FlatTestArena")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = next((actor for actor in actors.get_all_level_actors() if actor.get_actor_label() == "Pickup_Nature_White"), None)
if not existing:
    existing = actors.spawn_actor_from_class(artifact_bp.generated_class(), unreal.Vector(0., -750., 60.))
    existing.set_actor_label("Pickup_Nature_White")
    existing.set_actor_scale3d(unreal.Vector(1.2, 1.2, 1.2))
existing.get_editor_property("artifact").set_editor_property("form", nature)
existing.get_editor_property("mesh").set_material(0, material)
sign = next((actor for actor in actors.get_all_level_actors() if actor.get_actor_label() == "Label_Nature_White"), None)
if not sign:
    sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(0., -750., 160.))
    sign.set_actor_label("Label_Nature_White")
    sign.set_actor_rotation(unreal.Rotator(0., 180., 0.), False)
sign.get_editor_property("text_render").set_text("NATURE - WHITE [E] STORE IN BAG")
sign.get_editor_property("text_render").set_world_size(24.)
assert levels.save_current_level()
report = dict(initialForm=initial.get_path_name(), initialFormId="InitialInk", health=75., attack=11.25,
              canEvolve=False, mesh=template_mesh.get_path_name(), animClass=anim_class.get_path_name(),
              natureForm=nature.get_path_name(), natureFormId="Native", naturePickup=existing.get_path_name(),
              artifactForms=[form.get_path_name() for form in catalog], animationClips=len(clips),
              sourceTemplateMesh=source_template_mesh.get_path_name(), saveVersion=2, legacyMigration="Version 1 NativeTier -> unlocked Nature route, selected form retained")
Path(unreal.Paths.project_saved_dir(), "InitialInkNatureSetup.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("INITIAL_INK_NATURE_SETUP_PASS")

"""Build editable state machines and connect the arena's actors and nine forms.

Run after retarget_that_actions.py and fix_retarget_import_scale.py. Imported
SOURCE/THAT and LanTuRebuild packages are never modified here.
"""
import json
from pathlib import Path
import unreal

ROOT = "/Game/HPWH"
ANIM_ROOT = ROOT + "/Locomotion"
NAMES = ["铜机械小兵", "铜械战士", "铜械将军", "铜械降神", "竹节人", "竹路人", "竹拳击手", "陶罐头", "陶童子", "陶土行者", "陶土僧人"]
KINDS = ["Idle", "Run", "Jump", "Fall", "Land", "Death", "Jab", "CrossJab", "Uppercut"]
tools = unreal.AssetToolsHelpers.get_asset_tools()
library = unreal.load_asset(ANIM_ROOT + "/DA_NPCLocomotion")
assert isinstance(library, unreal.CC_LocomotionLibrary), "Run configure_npc_art.py first"
report = {"stateMachines": [], "forms": [], "blueprints": [], "npcs": []}
entries = []
anim_classes = {}
for name in NAMES:
    mesh = unreal.load_asset("/Game/SOURCE/小怪/" + name + "/SK_" + name)
    assert mesh, "Missing mesh: " + name
    skeleton = mesh.get_editor_property("skeleton")
    clips = {}
    for kind in KINDS:
        clip = unreal.load_asset(ANIM_ROOT + "/Animations/A_" + name + "_" + kind)
        assert isinstance(clip, unreal.AnimSequence), "Missing animation: " + name + " " + kind
        assert clip.get_editor_property("skeleton") == skeleton, "Skeleton mismatch: " + clip.get_path_name()
        clips[kind] = clip
    bp_name = "ABP_" + name
    bp = unreal.load_asset(ANIM_ROOT + "/" + bp_name)
    if not bp:
        factory = unreal.AnimBlueprintFactory()
        factory.set_editor_property("parent_class", unreal.CC_CharacterAnimInstance)
        factory.set_editor_property("target_skeleton", skeleton)
        bp = tools.create_asset(bp_name, ANIM_ROOT, unreal.AnimBlueprint, factory)
    assert bp, "Could not create " + bp_name
    assert unreal.CC_AnimationSetupLibrary.build_locomotion_graph(bp, *[clips[kind] for kind in KINDS[:6]]), "State machine compilation failed: " + bp_name
    anim_class = bp.generated_class()
    assert anim_class
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    anim_classes[mesh.get_path_name()] = anim_class
    entry = unreal.CC_MeshLocomotion()
    entry.set_editor_property("mesh", mesh)
    for kind in KINDS:
        field = "cross_jab" if kind == "CrossJab" else kind.lower()
        entry.set_editor_property(field, clips[kind])
    entry.set_editor_property("anim_class", anim_class)
    entry.set_editor_property("reference_speed", 400.0)
    entries.append(entry)
    report["stateMachines"].append({"mesh": mesh.get_path_name(), "blueprint": bp.get_path_name(), "clips": [clip.get_path_name() for clip in clips.values()]})
library.set_editor_property("entries", entries)
unreal.EditorAssetLibrary.save_loaded_asset(library, False)

for key in ["Native", "Artifact_01", "Artifact_02"]:
    form = unreal.load_asset(ROOT + "/Forms/DA_Form_" + key)
    assert form
    stages = form.get_editor_property("stages")
    assert len(stages) == 3, key
    for index, stage in enumerate(stages):
        mesh = stage.get_editor_property("mesh")
        anim_class = anim_classes[mesh.get_path_name()]
        stage.set_editor_property("anim_class", anim_class)
        stages[index] = stage
        report["forms"].append({"form": key, "tier": index + 1, "mesh": mesh.get_path_name(), "animClass": anim_class.get_path_name()})
    form.set_editor_property("stages", stages)
    unreal.EditorAssetLibrary.save_loaded_asset(form, False)

default_models = {"BP_ProgressionCharacter": "竹路人", "BP_PatrolEnemy": "铜机械小兵", "BP_TestDummy": "陶罐头", "BP_ProgressionEnemy": "铜机械小兵"}
native_form = unreal.load_asset(ROOT + "/Forms/DA_Form_Native")
assert native_form
native_mesh = native_form.get_editor_property("stages")[0].get_editor_property("mesh")
assert native_mesh and native_mesh.get_name() == "SK_竹路人", "Native tier 1 must be 竹路人"
for name, model_name in default_models.items():
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    assert bp
    default = unreal.get_default_object(bp.generated_class())
    default.get_editor_property("locomotion").set_editor_property("library", library)
    component = default.get_editor_property("mesh")
    mesh = native_mesh if name == "BP_ProgressionCharacter" else unreal.load_asset("/Game/SOURCE/小怪/" + model_name + "/SK_" + model_name)
    assert mesh and mesh.get_path_name() in anim_classes, "Default mesh lacks animation entry: " + name
    height = mesh.get_bounds().box_extent.z * 2.0
    assert height > 0.0, "Default mesh bounds are empty: " + name
    size = 180.0 / height
    # Only replace art/animation/scale; preserve each blueprint's mesh offset and rotation.
    component.set_editor_property("skeletal_mesh_asset", mesh)
    component.set_relative_scale3d(unreal.Vector(size, size, size))
    component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    component.set_editor_property("anim_class", anim_classes[mesh.get_path_name()])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    # Re-read the compiled CDO so the report verifies persisted blueprint defaults.
    compiled_default = unreal.get_default_object(bp.generated_class())
    compiled_mesh = compiled_default.get_editor_property("mesh")
    assert compiled_mesh.get_editor_property("skeletal_mesh_asset") == mesh, "Default mesh assignment failed: " + name
    assert compiled_mesh.get_editor_property("anim_class") == anim_classes[mesh.get_path_name()], "Default AnimClass assignment failed: " + name
    assert compiled_default.get_editor_property("locomotion").get_editor_property("library") == library, "Default animation library missing: " + name
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    report["blueprints"].append({"blueprint": bp.get_path_name(), "mesh": mesh.get_path_name(), "animClass": anim_classes[mesh.get_path_name()].get_path_name(), "scale": size})

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(ROOT + "/Maps/L_FlatTestArena")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if not isinstance(actor, unreal.CC_EnemyCharacter):
        continue
    component = actor.get_editor_property("mesh")
    mesh = component.get_editor_property("skeletal_mesh_asset")
    assert mesh and mesh.get_path_name() in anim_classes, "NPC lacks mapped skeleton: " + actor.get_actor_label()
    component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    component.set_editor_property("anim_class", anim_classes[mesh.get_path_name()])
    actor.get_editor_property("locomotion").set_editor_property("library", library)
    report["npcs"].append({"actor": actor.get_actor_label(), "mesh": mesh.get_path_name(), "animClass": anim_classes[mesh.get_path_name()].get_path_name()})
assert len(report["npcs"]) == 10, "Unexpected number of arena NPCs"
assert levels.save_current_level(), "Could not save arena"
Path(unreal.Paths.project_saved_dir() + "CharacterAnimationSetup.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("CHARACTER_ANIMATION_SETUP_OK: 11 state machines, 9 stages, 10 NPCs")

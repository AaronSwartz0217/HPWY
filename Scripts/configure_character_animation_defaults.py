"""Repair only the four character blueprint defaults using existing baked assets.

Does not rebuild animation blueprints, retarget clips, load/save maps, or edit
imported models. Run after configure_character_animation.py created the library.
"""
import json
from pathlib import Path
import unreal

ROOT = "/Game/HPWH"
library = unreal.load_asset(ROOT + "/Locomotion/DA_NPCLocomotion")
assert isinstance(library, unreal.CC_LocomotionLibrary), "Existing animation library is required"
anim_classes = {}
for entry in library.get_editor_property("entries"):
    mesh = entry.get_editor_property("mesh")
    anim_class = entry.get_editor_property("anim_class")
    assert mesh and anim_class, "All animation entries must already have mesh and AnimClass"
    anim_classes[mesh.get_path_name()] = anim_class

default_models = {"BP_ProgressionCharacter": "竹路人", "BP_PatrolEnemy": "铜机械小兵", "BP_TestDummy": "陶罐头", "BP_ProgressionEnemy": "铜机械小兵"}
native_form = unreal.load_asset(ROOT + "/Forms/DA_Form_Native")
assert native_form
native_mesh = native_form.get_editor_property("stages")[0].get_editor_property("mesh")
assert native_mesh and native_mesh.get_name() == "SK_竹路人", "Native tier 1 must be 竹路人"
report = []
for name, model_name in default_models.items():
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    assert bp, name
    default = unreal.get_default_object(bp.generated_class())
    component = default.get_editor_property("mesh")
    previous_location = component.get_editor_property("relative_location")
    previous_rotation = component.get_editor_property("relative_rotation")
    mesh = native_mesh if name == "BP_ProgressionCharacter" else unreal.load_asset("/Game/SOURCE/小怪/" + model_name + "/SK_" + model_name)
    assert mesh and mesh.get_path_name() in anim_classes, "Default mesh lacks animation entry: " + name
    anim_class = anim_classes[mesh.get_path_name()]
    height = mesh.get_bounds().box_extent.z * 2.0
    assert height > 0.0, "Default mesh bounds are empty: " + name
    size = 180.0 / height
    component.set_editor_property("skeletal_mesh_asset", mesh)
    component.set_relative_scale3d(unreal.Vector(size, size, size))
    component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    component.set_editor_property("anim_class", anim_class)
    default.get_editor_property("locomotion").set_editor_property("library", library)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    compiled_default = unreal.get_default_object(bp.generated_class())
    compiled_mesh = compiled_default.get_editor_property("mesh")
    assert compiled_mesh.get_editor_property("skeletal_mesh_asset") == mesh, "Default mesh assignment failed: " + name
    assert compiled_mesh.get_editor_property("anim_class") == anim_class, "Default AnimClass assignment failed: " + name
    assert compiled_mesh.get_editor_property("relative_location") == previous_location, "Mesh location changed: " + name
    assert compiled_mesh.get_editor_property("relative_rotation") == previous_rotation, "Mesh rotation changed: " + name
    assert compiled_default.get_editor_property("locomotion").get_editor_property("library") == library, "Default animation library missing: " + name
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    report.append({"blueprint": bp.get_path_name(), "mesh": mesh.get_path_name(), "animClass": anim_class.get_path_name(), "scale": size, "relativeLocation": str(previous_location), "relativeRotation": str(previous_rotation)})
Path(unreal.Paths.project_saved_dir() + "CharacterAnimationDefaults.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("CHARACTER_ANIMATION_DEFAULTS_OK: " + str(len(report)))

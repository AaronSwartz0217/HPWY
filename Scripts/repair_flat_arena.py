"""Repair only assets/actors created for the flat test arena, preserving placement."""
import unreal

ROOT = "/Game/HPWH"
for name in ["BP_ProgressionCharacter", "BP_ProgressionEnemy", "BP_TestDummy"]:
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    obj = unreal.get_default_object(bp.generated_class())
    obj.set_editor_property("initialize_attributes_effect", unreal.CC_InitialAttributesEffect)
    obj.set_editor_property("reset_attributes_effect", unreal.CC_InitialAttributesEffect)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log("INIT_EFFECT_FIXED: " + name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level(ROOT + "/Maps/L_FlatTestArena")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.TextRenderActor) or actor.get_actor_label().startswith(("Target_Dummy_", "Active_Enemy_")):
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0), False)
    if isinstance(actor, unreal.DirectionalLight):
        actor.set_actor_rotation(unreal.Rotator(pitch=-55, yaw=-30, roll=0), False)
    if actor.get_actor_label().startswith("Style_"):
        parts = actor.get_actor_label().split("_")
        material = unreal.load_asset(ROOT + "/Materials/MI_Style_" + parts[2])
        actor.get_editor_property("mesh").set_material(0, material)

master = unreal.load_asset(ROOT + "/Materials/M_TestTint")
unreal.MaterialEditingLibrary.recompile_material(master)
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
levels.save_current_level()
unreal.log("ARENA_REPAIR_OK")

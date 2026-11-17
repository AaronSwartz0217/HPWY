"""Create three maze patrol variants using the existing bronze balance profiles."""
import unreal


ROOT = "/Game/HPWH"
DESTINATION = ROOT + "/Spawning/HeavenMaze"
assets = unreal.EditorAssetLibrary
source = unreal.load_asset(ROOT + "/Blueprints/BP_PatrolEnemy")
form = unreal.load_asset(ROOT + "/Forms/DA_Form_Artifact_01")
library = unreal.load_asset(ROOT + "/Locomotion/DA_NPCLocomotion")
assert source and form and library and len(form.stages) == 3

for tier, stage in enumerate(form.stages, 1):
    path = f"{DESTINATION}/BP_MazeBronze_T{tier}"
    blueprint = unreal.load_asset(path) or assets.duplicate_loaded_asset(source, path)
    assert blueprint and stage.mesh and stage.combat_balance
    enemy = unreal.get_default_object(blueprint.generated_class())
    enemy.set_editor_property("combat_balance", stage.combat_balance)
    enemy.set_editor_property("stat_profile_id", f"Bronze_T{tier}")
    enemy.set_editor_property("replicates", True)
    enemy.set_editor_property("patrol_radius", 130.0)
    enemy.set_editor_property("patrol_acceptance_radius", 40.0)
    enemy.set_editor_property("detection_radius", 850.0)
    enemy.set_editor_property("max_chase_distance", 1050.0)
    enemy.set_editor_property("start_patrolling", True)
    enemy.set_editor_property("detect_players", True)
    enemy.get_editor_property("locomotion").set_editor_property("library", library)
    mesh = enemy.get_editor_property("mesh")
    mesh.set_editor_property("skeletal_mesh_asset", stage.mesh)
    mesh.set_editor_property("override_materials", [])
    mesh.set_editor_property("relative_scale3d", stage.mesh_scale)
    mesh.set_editor_property("relative_location", unreal.Vector(0, 0, -96))
    mesh.set_editor_property("relative_rotation", unreal.Rotator(pitch=0, yaw=-90, roll=0))
    mesh.set_editor_property("anim_class", stage.anim_class)
    enemy.get_editor_property("capsule_component").set_capsule_size(42.0, 96.0, False)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assert assets.save_loaded_asset(blueprint, False), path

unreal.log("HEAVEN_MAZE_BRONZE_ASSETS_READY")
unreal.SystemLibrary.quit_editor()

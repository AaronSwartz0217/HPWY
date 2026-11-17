"""Create a separate playable smoke-test map. Run after setup_hpwh.py."""
import unreal

path = "/Game/HPWH/Maps/L_HPWH"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("PROGRESSION_MAP_EXISTS: leaving existing demo map unchanged")
else:
    unreal.EditorLevelLibrary.new_level(path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    gm = unreal.load_class(None, "/Game/HPWH/Blueprints/BP_ProgressionGameMode.BP_ProgressionGameMode_C")
    world.get_world_settings().set_editor_property("default_game_mode", gm)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -75))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
    floor.set_actor_scale3d(unreal.Vector(40, 40, 1))
    floor.set_actor_label("Progression_TestFloor")
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-45, 30, 0))
    sun.light_component.set_editor_property("intensity", 5.0)
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    for i in range(4):
        cls = unreal.load_class(None, "/Game/HPWH/Blueprints/BP_Artifact_%02d.BP_Artifact_%02d_C" % (i + 1, i + 1))
        artifact = actors.spawn_actor_from_class(cls, unreal.Vector(150, -180 + i * 120, 30))
        artifact.set_actor_scale3d(unreal.Vector(0.6, 0.6, 0.6))
        artifact.set_actor_label("Artifact_%02d" % (i + 1))
    enemy_cls = unreal.load_class(None, "/Game/HPWH/Blueprints/BP_ProgressionEnemy.BP_ProgressionEnemy_C")
    for i in range(4):
        actors.spawn_actor_from_class(enemy_cls, unreal.Vector(700, -300 + i * 200, 120))
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("PROGRESSION_MAP_OK: " + path)

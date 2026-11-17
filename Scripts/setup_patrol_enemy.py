"""Create a reusable patrol blueprint and add navigation to the flat arena."""
import unreal
import json

ROOT = "/Game/HPWH"
path = ROOT + "/Blueprints/BP_PatrolEnemy"
tools = unreal.AssetToolsHelpers.get_asset_tools()
bp = unreal.load_asset(path)
if not bp:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.CC_PatrolEnemyCharacter)
    bp = tools.create_asset("BP_PatrolEnemy", ROOT + "/Blueprints", unreal.Blueprint, factory)
obj = unreal.get_default_object(bp.generated_class())
player = unreal.get_default_object(unreal.load_class(None, ROOT + "/Blueprints/BP_ProgressionCharacter.BP_ProgressionCharacter_C"))
source_mesh = player.get_editor_property("mesh")
mesh = obj.get_editor_property("mesh")
for field in ["skeletal_mesh_asset", "anim_class", "relative_location", "relative_rotation", "relative_scale3d"]:
    mesh.set_editor_property(field, source_mesh.get_editor_property(field))
obj.get_editor_property("skills").set_editor_property("default_loadout", unreal.load_asset(ROOT + "/Loadouts/DA_EnemyLoadout"))
obj.set_editor_property("startup_abilities", [])
obj.set_editor_property("patrol_radius", 600.0)
obj.set_editor_property("detection_radius", 900.0)
obj.set_editor_property("max_chase_distance", 1800.0)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level(ROOT + "/Maps/L_FlatTestArena")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
existing = actors.get_all_level_actors()
for actor in existing:
    if actor.get_actor_label().startswith("Active_Enemy_"):
        location = actor.get_actor_location()
        rotation = actor.get_actor_rotation()
        replacement = actors.spawn_actor_from_class(bp.generated_class(), location, rotation)
        replacement.set_actor_label(actor.get_actor_label().replace("Active_Enemy_", "Patrol_Enemy_"))
        actors.destroy_actor(actor)
nav_bounds = next((a for a in actors.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume)), None)
if not nav_bounds:
    nav_bounds = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 150))
    nav_bounds.set_actor_label("Arena_NavMeshBounds")
origin, extent = nav_bounds.get_actor_bounds(False)
assert extent.x > 0 and extent.y > 0 and extent.z > 0, "Navigation bounds need a non-empty brush"
scale = nav_bounds.get_actor_scale3d()
nav_bounds.set_actor_scale3d(unreal.Vector(scale.x * 5900.0 / extent.x, scale.y * 5900.0 / extent.y, scale.z * 400.0 / extent.z))
nav_bounds.set_actor_location(unreal.Vector(0, 0, 150), False, False)
navs = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]
for nav in navs:
    nav.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
    nav.set_editor_property("force_rebuild_on_load", True)
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
levels.save_current_level()
report = {"blueprint": path, "navigation_extent": str(nav_bounds.get_actor_bounds(False)[1]), "recast_count": len(navs),
          "patrol_enemies": [a.get_actor_label() for a in actors.get_all_level_actors() if isinstance(a, unreal.CC_PatrolEnemyCharacter)]}
with open(unreal.Paths.project_saved_dir() + "PatrolSetup.json", "w", encoding="utf-8") as stream:
    json.dump(report, stream, indent=2)
unreal.log("PATROL_SETUP_OK: " + str(report))

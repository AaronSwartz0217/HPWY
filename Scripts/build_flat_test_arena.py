"""Build the requested flat prototype arena without editing the original THAT assets.
Requires the existing HPWH assets. Does not rerun/reset the skill generator.
"""
import unreal
import json

ROOT = "/Game/HPWH"
MAP = ROOT + "/Maps/L_FlatTestArena"
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def get_or_create(name, folder, cls, factory):
    path = ROOT + "/" + folder
    asset = unreal.load_asset(path + "/" + name)
    return asset or tools.create_asset(name, path, cls, factory)


def cdo(path):
    bp = unreal.load_asset(path)
    assert bp, path
    return unreal.get_default_object(bp.generated_class())


player = cdo(ROOT + "/Blueprints/BP_ProgressionCharacter")
player_mesh = player.get_editor_property("mesh")
mesh_asset = player_mesh.get_editor_property("skeletal_mesh_asset")
anim_class = player_mesh.get_editor_property("anim_class")
assert mesh_asset and anim_class, "Initial player model and animation must be configured"
for character in [player, cdo(ROOT + "/Blueprints/BP_ProgressionEnemy")]:
    character.set_editor_property("initialize_attributes_effect", unreal.CC_InitialAttributesEffect)
    character.set_editor_property("reset_attributes_effect", unreal.CC_InitialAttributesEffect)

# Keep all five forms on the initial character mesh until final art is provided.
for index in range(5):
    form_id = "Native" if index == 0 else "Artifact_%02d" % index
    form = unreal.load_asset(ROOT + "/Forms/DA_Form_" + form_id)
    form.set_editor_property("display_name", "原生形态" if index == 0 else ["样式一", "样式二", "样式三", "样式四"][index - 1])
    stages = form.get_editor_property("stages")
    for stage in stages:
        stage.set_editor_property("mesh", mesh_asset)
        stage.set_editor_property("anim_class", anim_class)
    form.set_editor_property("stages", stages)

enemy_bp = unreal.load_asset(ROOT + "/Blueprints/BP_ProgressionEnemy")
enemy = unreal.get_default_object(enemy_bp.generated_class())
enemy_mesh = enemy.get_editor_property("mesh")
enemy_mesh.set_editor_property("skeletal_mesh_asset", mesh_asset)
enemy_mesh.set_editor_property("anim_class", anim_class)
enemy_mesh.set_editor_property("relative_location", player_mesh.get_editor_property("relative_location"))
enemy_mesh.set_editor_property("relative_rotation", player_mesh.get_editor_property("relative_rotation"))
enemy_mesh.set_editor_property("relative_scale3d", player_mesh.get_editor_property("relative_scale3d"))

dummy_factory = unreal.BlueprintFactory()
dummy_factory.set_editor_property("parent_class", enemy_bp.generated_class())
dummy_bp = get_or_create("BP_TestDummy", "Blueprints", unreal.Blueprint, dummy_factory)
dummy = unreal.get_default_object(dummy_bp.generated_class())
dummy.set_editor_property("startup_abilities", [])
dummy.set_editor_property("auto_possess_ai", unreal.AutoPossessAI.DISABLED)
dummy.set_editor_property("experience_reward", 10)
dummy.set_editor_property("initialize_attributes_effect", unreal.CC_InitialAttributesEffect)
dummy.set_editor_property("reset_attributes_effect", unreal.CC_InitialAttributesEffect)
for bp in [enemy_bp, dummy_bp]:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)

master = get_or_create("M_TestTint", "Materials", unreal.Material, unreal.MaterialFactoryNew())
expressions = unreal.MaterialEditingLibrary.get_material_property_input_node(master, unreal.MaterialProperty.MP_BASE_COLOR)
if not expressions:
    tint = unreal.MaterialEditingLibrary.create_material_expression(master, unreal.MaterialExpressionVectorParameter, -200, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1))
    unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(master)

colors = [("Red", unreal.LinearColor(0.85, 0.025, 0.025, 1)),
          ("Blue", unreal.LinearColor(0.025, 0.12, 0.9, 1)),
          ("Green", unreal.LinearColor(0.025, 0.75, 0.06, 1)),
          ("Yellow", unreal.LinearColor(0.95, 0.65, 0.015, 1))]
for index, (name, color) in enumerate(colors, 1):
    material = get_or_create("MI_Style_" + name, "Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(material, master)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, "Tint", color)
    artifact = cdo(ROOT + "/Blueprints/BP_Artifact_%02d" % index)
    artifact.get_editor_property("mesh").set_material(0, material)

# A distinct map preserves both OverRide and the earlier smoke-test map.
assert not unreal.EditorAssetLibrary.does_asset_exist(MAP), "Arena already exists; edit it in the editor instead of recreating it."
levels.new_level(MAP)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, ROOT + "/Blueprints/BP_ProgressionGameMode.BP_ProgressionGameMode_C"))

floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -25))
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
floor.set_actor_scale3d(unreal.Vector(120, 120, 0.5))
floor.set_actor_label("Flat_Ground_120m_x_120m")
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-500, 0, 100))
start.set_actor_label("Player_Start")
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(pitch=-55, yaw=-30, roll=0))
sun.light_component.set_editor_property("intensity", 4.0)
actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000))
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))


def sign(text, location, size=35):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, location, unreal.Rotator(pitch=0, yaw=180, roll=0))
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(text)
    component.set_world_size(size)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    actor.set_actor_label(text)
    return actor


for index, (color, _) in enumerate(colors, 1):
    cls = unreal.load_class(None, ROOT + "/Blueprints/BP_Artifact_%02d.BP_Artifact_%02d_C" % (index, index))
    artifact = actors.spawn_actor_from_class(cls, unreal.Vector(0, -450 + (index - 1) * 300, 60))
    artifact.set_actor_scale3d(unreal.Vector(1.2, 1.2, 1.2))
    artifact.set_actor_label("Style_%d_%s" % (index, color))
    sign("Style %d - %s [E]" % (index, color), unreal.Vector(0, -450 + (index - 1) * 300, 160), 24)

for index in range(6):
    actor = actors.spawn_actor_from_class(dummy_bp.generated_class(), unreal.Vector(700, -750 + index * 300, 100), unreal.Rotator(pitch=0, yaw=180, roll=0))
    actor.set_actor_label("Target_Dummy_%02d" % (index + 1))
for index in range(4):
    actor = actors.spawn_actor_from_class(enemy_bp.generated_class(), unreal.Vector(2400, -600 + index * 400, 100), unreal.Rotator(pitch=0, yaw=180, roll=0))
    actor.set_actor_label("Active_Enemy_%02d" % (index + 1))
sign("TARGET DUMMIES | 10 XP", unreal.Vector(700, 0, 350))
sign("ACTIVE ENEMIES", unreal.Vector(2400, 0, 350))

preview = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-1450, -1900, 1450), unreal.Rotator(pitch=-26, yaw=40, roll=0))
preview.set_actor_label("Arena_PreviewCamera")
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
levels.save_current_level()
unreal.log("FLAT_ARENA_OK: " + MAP)

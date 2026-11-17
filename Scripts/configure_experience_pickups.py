"""Create the XP Niagara sprite and one black 50 XP pack in the existing flat map.

Run only after compiling CC_ExperienceFXSetupLibrary/CC_ExperiencePack. Existing NPC
models, materials, forms, skill balance and all other map actors are preserved.
"""
import json
import pathlib
import unreal

ROOT = "/Game/HPWH"
MAP = ROOT + "/Maps/L_FlatTestArena"
tools = unreal.AssetToolsHelpers.get_asset_tools()
materials = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary


def asset(name, folder, cls, factory):
    path = ROOT + "/" + folder
    return unreal.load_asset(path + "/" + name) or tools.create_asset(name, path, cls, factory)


def expression(material, cls, x, y):
    return materials.create_material_expression(material, cls, x, y)


def constant(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def color(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property("constant", unreal.LinearColor(*value))
    return node


black = asset("M_ExperiencePackBlack", "Materials", unreal.Material, unreal.MaterialFactoryNew())
materials.delete_all_material_expressions(black)
black.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
materials.connect_material_property(color(black, (0, 0, 0, 1), -300, -100), "", unreal.MaterialProperty.MP_BASE_COLOR)
materials.connect_material_property(constant(black, 0, -300, 0), "", unreal.MaterialProperty.MP_SPECULAR)
materials.connect_material_property(constant(black, 1, -300, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
materials.recompile_material(black)
assets.save_loaded_asset(black, False)

orb_material = asset("M_ExperienceOrb", "Effects", unreal.Material, unreal.MaterialFactoryNew())
materials.delete_all_material_expressions(orb_material)
orb_material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
orb_material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
orb_material.set_editor_property("two_sided", True)
materials.set_material_usage(orb_material, unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
uv = expression(orb_material, unreal.MaterialExpressionTextureCoordinate, -650, 100)
center = expression(orb_material, unreal.MaterialExpressionConstant2Vector, -650, 250)
center.set_editor_property("r", .5)
center.set_editor_property("g", .5)
mask = expression(orb_material, unreal.MaterialExpressionSphereMask, -350, 150)
mask.set_editor_property("attenuation_radius", .5)
mask.set_editor_property("hardness_percent", 0.0)
materials.connect_material_expressions(uv, "", mask, "A")
materials.connect_material_expressions(center, "", mask, "B")
materials.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY)
particle_color = expression(orb_material, unreal.MaterialExpressionParticleColor, -650, -200)
brightness = expression(orb_material, unreal.MaterialExpressionMultiply, -350, -150)
materials.connect_material_expressions(particle_color, "RGB", brightness, "A")
materials.connect_material_expressions(constant(orb_material, 5, -650, -50), "", brightness, "B")
materials.connect_material_property(brightness, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
materials.recompile_material(orb_material)
assets.save_loaded_asset(orb_material, False)

system = asset("NS_ExperienceOrb", "Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
assert unreal.CC_ExperienceFXSetupLibrary.build_experience_orb_system(system, orb_material), "XP Niagara emitter setup failed"
assets.save_loaded_asset(system, False)

enemy_defaults = []
for name in ("BP_ProgressionEnemy", "BP_PatrolEnemy", "BP_TestDummy"):
    bp = unreal.load_asset(ROOT + "/Blueprints/" + name)
    assert bp, "Missing enemy blueprint: " + name
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("experience_particle_system", system)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assets.save_loaded_asset(bp, False)
    enemy_defaults.append(bp.get_path_name())

pack_class = unreal.load_class(None, "/Script/CrashCourse.CC_ExperiencePack")
assert pack_class, "Compile the CC_ExperiencePack parent first"
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", pack_class)
pack_bp = asset("BP_ExperiencePack", "Blueprints", unreal.Blueprint, factory)
pack = unreal.get_default_object(pack_bp.generated_class())
pack.set_editor_property("experience_amount", 50)
pack.set_editor_property("experience_particle_system", system)
pack.set_editor_property("interaction_distance", 250.0)
pack.set_editor_property("repeatable", False)
pack.set_editor_property("reuse_delay", 1.0)
pack_mesh = pack.get_editor_property("mesh")
pack_mesh.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
pack_mesh.set_material(0, black)
unreal.BlueprintEditorLibrary.compile_blueprint(pack_bp)
assets.save_loaded_asset(pack_bp, False)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(MAP), "Could not load existing flat test map"
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world_actors = actors.get_all_level_actors()
updated_enemies = []
for actor in world_actors:
    if isinstance(actor, unreal.CC_EnemyCharacter):
        actor.set_editor_property("experience_particle_system", system)
        updated_enemies.append(actor.get_actor_label())

starts = [a for a in world_actors if isinstance(a, unreal.PlayerStart)]
assert starts, "The test map needs a PlayerStart"
start = starts[0]
pack_actors = [a for a in world_actors if isinstance(a, unreal.CC_ExperiencePack) and a.get_actor_label() == "Experience_Pack_Black_50XP"]
assert len(pack_actors) < 2, "Duplicate named experience packs must be resolved before setup"
if pack_actors:
    placed = pack_actors[0]
else:
    origin = start.get_actor_location()
    forward, right = start.get_actor_forward_vector(), start.get_actor_right_vector()
    location = origin + forward * 150.0 - right * 350.0
    location.z = origin.z - 50.0
    placed = actors.spawn_actor_from_class(pack_bp.generated_class(), location)
    placed.set_actor_label("Experience_Pack_Black_50XP")
placed.set_editor_property("experience_amount", 50)
placed.set_editor_property("experience_particle_system", system)
placed.set_editor_property("repeatable", False)
placed.get_editor_property("mesh").set_material(0, black)
label = placed.get_component_by_class(unreal.TextRenderComponent)
if label:
    label.set_text("50 XP  [E]")
    label.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    label.set_world_size(25.0)
    label.set_world_rotation(unreal.Rotator(pitch=0.0, yaw=start.get_actor_rotation().yaw + 180.0, roll=0.0), False, False)

assert levels.save_current_level(), "Could not save XP test map changes"
report = {
    "system": system.get_path_name(),
    "single_sprite_per_actor": True,
    "sprite_material": orb_material.get_path_name(),
    "pack_blueprint": pack_bp.get_path_name(),
    "pack_label": placed.get_actor_label(),
    "pack_location": str(placed.get_actor_location()),
    "experience_per_pack": 50,
    "repeatable": False,
    "enemy_blueprints": enemy_defaults,
    "enemy_instances": updated_enemies,
    "map": MAP,
}
pathlib.Path(unreal.Paths.project_saved_dir(), "ExperiencePickupSetup.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("EXPERIENCE_PICKUP_SETUP_PASS: single-sprite Niagara, 50 XP pack, %d enemies" % len(updated_enemies))

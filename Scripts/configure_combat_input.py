"""J basic attack + 1/2/3/4 skills. Preserve the template's movement modifiers."""
import unreal

ROOT = "/Game/HPWH"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def asset(name, cls):
    path = ROOT + "/Input"
    existing = unreal.load_asset(path + "/" + name)
    if existing:
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, path, cls, factory)


bp = unreal.load_asset(ROOT + "/Blueprints/BP_ProgressionController")
controller = unreal.get_default_object(bp.generated_class())
# Always clone the original template maps, so reruns do not stack bindings.
original = unreal.get_default_object(unreal.load_class(None, "/Game/THAT/Player/BP_CC_PlayController.BP_CC_PlayController_C"))
movement_actions = [original.get_editor_property(field) for field in ["move_action", "look_action", "jump_action"]]
assert all(movement_actions), "Template movement actions are required"
contexts = []
movement_report = []
for index, source in enumerate(original.get_editor_property("input_mapping_contexts")):
    mappings = source.get_editor_property("mappings")
    if not any(mapping.get_editor_property("action") in movement_actions for mapping in mappings):
        continue
    path = ROOT + "/Input/IMC_TemplateMovement_%d" % index
    copied = unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(source.get_path_name(), path)
    for mapping in list(copied.get_editor_property("mappings")):
        action = mapping.get_editor_property("action")
        if action not in movement_actions:
            copied.unmap_all_keys_from_action(action)
    contexts.append(copied)
    movement_report.append(str(copied.get_editor_property("mappings")))
assert contexts, "No template movement context found"

combat = asset("IMC_Combat_J1234", unreal.InputMappingContext)
combat.unmap_all()
for field, name, key in [
    ("basic_attack_action", "IA_BasicAttack_J", "J"),
    ("primary_action", "IA_Skill_1", "One"),
    ("secondary_action", "IA_Skill_2", "Two"),
    ("tertiary_action", "IA_Skill_3", "Three"),
    ("fourth_action", "IA_Skill_4", "Four"),
    ("interact_action", "IA_Interact_E", "E"),
]:
    action = asset(name, unreal.InputAction)
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    mapped_key = unreal.Key()
    mapped_key.import_text(key)
    assert mapped_key.export_text() == key, "FKey serialization mismatch: " + mapped_key.export_text()
    combat.map_key(action, mapped_key)
    controller.set_editor_property(field, action)
contexts.append(combat)
controller.set_editor_property("input_mapping_contexts", contexts)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)

basic_path = ROOT + "/Skills/DA_BasicAttack"
basic = unreal.load_asset(basic_path) or unreal.EditorAssetLibrary.duplicate_asset(ROOT + "/Skills/DA_Skill_01", basic_path)
basic.set_editor_property("skill_id", "BasicAttack")
basic.set_editor_property("display_name", "普攻")
basic.set_editor_property("behavior", unreal.CC_SkillBehavior.MELEE)
basic.set_editor_property("mana_cost", 0.0)
basic.set_editor_property("cooldown", 0.4)
basic.set_editor_property("power", 15.0)
basic.set_editor_property("range", 220.0)
basic.set_editor_property("angle", 100.0)
player = unreal.load_asset(ROOT + "/Blueprints/BP_ProgressionCharacter")
unreal.get_default_object(player.generated_class()).get_editor_property("skills").set_editor_property("basic_attack", basic)
unreal.BlueprintEditorLibrary.compile_blueprint(player)

assert len(combat.get_editor_property("mappings")) == 6
for mapping in combat.get_editor_property("mappings"):
    unreal.log("COMBAT_KEY: " + mapping.get_editor_property("action").get_name() + "=" + mapping.get_editor_property("key").export_text())
unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
# Mapping functions do not reliably mark an existing context package dirty in commandlets.
# Force-save the exact edited assets, then verify them in a fresh process.
for edited in [*contexts, bp, player, basic]:
    unreal.EditorAssetLibrary.save_loaded_asset(edited, only_if_is_dirty=False)
unreal.log("COMBAT_INPUT_OK: J basic, 1/2/3/4 skills, E interact; template movement preserved")
for report in movement_report:
    unreal.log("PRESERVED_MOVEMENT: " + report)

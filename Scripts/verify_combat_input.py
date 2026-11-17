import unreal
import json

ROOT = "/Game/HPWH"
controller = unreal.get_default_object(unreal.load_class(None, ROOT + "/Blueprints/BP_ProgressionController.BP_ProgressionController_C"))
report = {"combat": {}, "movement": [], "basic_attack": None}
contexts = controller.get_editor_property("input_mapping_contexts")
for context in contexts:
    for mapping in context.get_editor_property("mappings"):
        action = mapping.get_editor_property("action")
        key = mapping.get_editor_property("key").export_text()
        if "IMC_Combat_J1234" in context.get_name():
            report["combat"][action.get_name()] = key
        else:
            report["movement"].append({"action": action.get_name(), "key": key, "modifiers": len(mapping.get_editor_property("modifiers"))})
assert report["combat"] == {"IA_BasicAttack_J": "J", "IA_Skill_1": "One", "IA_Skill_2": "Two", "IA_Skill_3": "Three", "IA_Skill_4": "Four", "IA_Interact_E": "E"}, report
original = unreal.get_default_object(unreal.load_class(None, "/Game/THAT/Player/BP_CC_PlayController.BP_CC_PlayController_C"))
movement_actions = [original.get_editor_property(field) for field in ["move_action", "look_action", "jump_action"]]
expected = []
for context in original.get_editor_property("input_mapping_contexts"):
    for mapping in context.get_editor_property("mappings"):
        action = mapping.get_editor_property("action")
        if action in movement_actions:
            expected.append({"action": action.get_name(), "key": mapping.get_editor_property("key").export_text(), "modifiers": len(mapping.get_editor_property("modifiers"))})
assert report["movement"] == expected, "Movement differs from template"
player = unreal.get_default_object(unreal.load_class(None, ROOT + "/Blueprints/BP_ProgressionCharacter.BP_ProgressionCharacter_C"))
basic = player.get_editor_property("skills").get_editor_property("basic_attack")
assert basic and str(basic.get_editor_property("skill_id")) == "BasicAttack"
assert basic.get_editor_property("mana_cost") == 0
report["basic_attack"] = basic.get_path_name()
with open(unreal.Paths.project_saved_dir() + "CombatInputVerification.json", "w", encoding="utf-8") as stream:
    json.dump(report, stream, indent=2)
unreal.log("COMBAT_INPUT_VERIFIED: saved assets reloaded; all keys and movement match")

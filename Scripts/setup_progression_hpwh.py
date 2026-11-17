"""Run with UE 5.6 PythonScriptPlugin after compiling CrashCourseEditor.
Creates only /Game/HPWH assets; existing gameplay assets are not edited.
"""
import unreal

ROOT = "/Game/HPWH"
tools = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)


def data(name, cls, folder):
    path = ROOT + "/" + folder
    asset = unreal.load_asset(path + "/" + name)
    if asset:
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return tools.create_asset(name, path, cls, factory)


def blueprint(name, parent, folder="Blueprints"):
    path = ROOT + "/" + folder
    asset = unreal.load_asset(path + "/" + name)
    if asset:
        return asset
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    return tools.create_asset(name, path, unreal.Blueprint, factory)


def defaults(bp):
    return unreal.get_default_object(bp.generated_class())


behaviors = ["MELEE", "COMBO", "SWEEP", "DASH", "PROJECTILE", "SPREAD", "HOMING", "PIERCING", "BURST", "GROUND_STRIKE", "DAMAGE_ZONE", "LINE_STRIKE", "HEAL", "AREA_HEAL", "SHIELD", "DEFENSE_BUFF", "ATTACK_BUFF", "SPEED_BUFF", "SLOW", "KNOCKBACK"]
skills = []
for index, behavior in enumerate(behaviors, 1):
    skill = data("DA_Skill_%02d" % index, unreal.CC_SkillDefinition, "Skills")
    skill.set_editor_property("skill_id", "Skill_%02d" % index)
    skill.set_editor_property("display_name", "Skill_%02d" % index)
    skill.set_editor_property("behavior", getattr(unreal.CC_SkillBehavior, behavior))
    skill.set_editor_property("range", 1200.0 if index in (5, 6, 7, 8, 10, 11, 12) else 300.0)
    skill.set_editor_property("power", 150.0 if index == 18 else 30.0)
    skills.append(skill)


def loadout(name, ids, level):
    asset = data(name, unreal.CC_SkillLoadout, "Loadouts")
    slots = []
    for skill_id in ids:
        slot = unreal.CC_SkillSlot()
        slot.set_editor_property("skill", skills[skill_id - 1])
        slot.set_editor_property("level", level)
        slots.append(slot)
    asset.set_editor_property("slots", slots)
    return asset


# Native + four artifact forms. Every stage can be independently edited later.
forms = []
combinations = [[1, 5, 4, 13], [2, 3, 15, 20], [5, 6, 7, 8], [9, 10, 11, 12], [14, 16, 17, 19]]
for form_index in range(5):
    form_id = "Native" if form_index == 0 else "Artifact_%02d" % form_index
    form = data("DA_Form_" + form_id, unreal.CC_FormDefinition, "Forms")
    form.set_editor_property("form_id", form_id)
    form.set_editor_property("display_name", form_id)
    stages = []
    for tier in range(3):
        stage = unreal.CC_FormStage()
        stage.set_editor_property("cost", [0, 100, 250][tier])
        stage.set_editor_property("max_health", 100.0 + tier * 50.0 + form_index * 10.0)
        stage.set_editor_property("max_mana", 100.0 + tier * 25.0)
        stage.set_editor_property("attack", tier * 20.0)
        stage.set_editor_property("defense", tier * 10.0)
        stage.set_editor_property("loadout", loadout("DA_Loadout_%s_Tier%d" % (form_id, tier + 1), combinations[form_index], tier + 1))
        stages.append(stage)
    form.set_editor_property("stages", stages)
    forms.append(form)

upgrades = []
for kind, amount in [("MAX_HEALTH", 20.0), ("MAX_MANA", 15.0), ("ATTACK", 10.0), ("DEFENSE", 10.0), ("MOVE_SPEED", 25.0), ("SKILL_LEVEL", 1.0)]:
    upgrade = data("DA_Upgrade_" + kind, unreal.CC_UpgradeDefinition, "Upgrades")
    upgrade.set_editor_property("upgrade_id", kind)
    upgrade.set_editor_property("display_name", kind)
    upgrade.set_editor_property("type", getattr(unreal.CC_UpgradeType, kind))
    upgrade.set_editor_property("prices", [25, 50, 100, 150, 250])
    upgrade.set_editor_property("bonus_per_level", amount)
    if kind == "SKILL_LEVEL":
        upgrade.set_editor_property("skill_id", "Skill_01")
    upgrades.append(upgrade)

base_ps = unreal.load_class(None, "/Game/THAT/Player/BP_CC_PlayerState.BP_CC_PlayerState_C")
ps = blueprint("BP_ProgressionPlayerState", base_ps)
progression = defaults(ps).get_editor_property("progression")
progression.set_editor_property("native_form", forms[0])
progression.set_editor_property("artifact_forms", forms[1:])
progression.set_editor_property("upgrade_catalog", upgrades)

base_pc = unreal.load_class(None, "/Game/THAT/Player/BP_CC_PlayController.BP_CC_PlayController_C")
pc = blueprint("BP_ProgressionController", base_pc)
defaults(pc).set_editor_property("show_progression_panel", True)

enemy = blueprint("BP_ProgressionEnemy", unreal.load_class(None, "/Game/THAT/BP_EnemyCharacter.BP_EnemyCharacter_C"))
defaults(enemy).get_editor_property("skills").set_editor_property("default_loadout", loadout("DA_EnemyLoadout", [1, 5, 9, 20], 1))
initialize_effect = unreal.CC_InitialAttributesEffect
defaults(enemy).set_editor_property("initialize_attributes_effect", initialize_effect)
defaults(enemy).set_editor_property("reset_attributes_effect", initialize_effect)
defaults(enemy).set_editor_property("ai_controller_class", unreal.AIController)
defaults(enemy).set_editor_property("auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
defaults(enemy).set_editor_property("startup_abilities", [unreal.CC_SearchForTarget])
player = blueprint("BP_ProgressionCharacter", unreal.load_class(None, "/Game/THAT/BP_PlayerCharacter.BP_PlayerCharacter_C"))
defaults(player).set_editor_property("initialize_attributes_effect", initialize_effect)
defaults(player).set_editor_property("reset_attributes_effect", initialize_effect)

for index in range(4):
    bp = blueprint("BP_Artifact_%02d" % (index + 1), unreal.CC_FormArtifact)
    defaults(bp).get_editor_property("artifact").set_editor_property("form", forms[index + 1])
    defaults(bp).get_editor_property("mesh").set_editor_property("static_mesh", unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))

gm = blueprint("BP_ProgressionGameMode", unreal.GameModeBase)
defaults(gm).set_editor_property("player_state_class", ps.generated_class())
defaults(gm).set_editor_property("player_controller_class", pc.generated_class())
defaults(gm).set_editor_property("default_pawn_class", player.generated_class())
defaults(gm).set_editor_property("hud_class", unreal.CC_ProgressionHUD)

for bp in [ps, pc, enemy, player, gm]:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)

unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
unreal.log("PROGRESSION_SETUP_OK: 20 skills, native + 4 artifact forms, 15 loadouts, upgrades and demo blueprints")

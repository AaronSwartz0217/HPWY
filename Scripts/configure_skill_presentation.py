"""Configure editable presentation fields only; preserve skill balance and loadouts."""
import json
import pathlib
import unreal

ROOT = "/Game/HPWH"
def load(path):
    obj = unreal.load_asset(path)
    assert obj, "Missing presentation asset: " + path
    return obj

sparks = load("/Game/StarterContent/Particles/P_Sparks")
dust = load("/Game/StarterContent/Particles/P_Ambient_Dust")
explosion = load("/Game/StarterContent/Particles/P_Explosion")
steam = load("/Game/StarterContent/Particles/P_Steam_Lit")
projectile = load("/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/NS_Projectile_01")
impact = load("/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/Hits/NS_Projectile_01_Hit")
projectiles = {"PROJECTILE", "SPREAD", "HOMING", "PIERCING"}
buffs = {"HEAL", "AREA_HEAL", "SHIELD", "DEFENSE_BUFF", "ATTACK_BUFF", "SPEED_BUFF"}
uppercuts = {"SWEEP", "BURST", "GROUND_STRIKE", "KNOCKBACK"}
report = []
for suffix in ["BasicAttack"] + ["Skill_%02d" % i for i in range(1, 21)]:
    skill = load(ROOT + "/Skills/DA_" + suffix)
    behavior = skill.get_editor_property("behavior").name
    pose = "UPPERCUT" if behavior in uppercuts else "CROSS_JAB" if behavior in projectiles or behavior in {"COMBO", "LINE_STRIKE"} else "JAB"
    skill.set_editor_property("attack_pose", getattr(unreal.CC_AttackPose, pose))
    skill.set_editor_property("animation_play_rate", 1.15 if suffix == "BasicAttack" else 1.0)
    skill.set_editor_property("windup", 0.12 if suffix == "BasicAttack" else 0.22 if pose == "UPPERCUT" else 0.16)
    skill.set_editor_property("effect_scale", 0.25 if suffix == "BasicAttack" else 0.4)
    skill.set_editor_property("effect_lifetime", 0.75 if suffix == "BasicAttack" else 1.25)
    skill.set_editor_property("cast_particles", steam if behavior in buffs else dust if behavior in {"DASH", "DAMAGE_ZONE"} else sparks)
    skill.set_editor_property("impact_particles", steam if behavior in buffs else explosion if behavior in {"BURST", "GROUND_STRIKE", "DAMAGE_ZONE"} else sparks)
    skill.set_editor_property("cast_niagara", None)
    skill.set_editor_property("impact_niagara", impact if behavior in projectiles else None)
    skill.set_editor_property("projectile_niagara", projectile if behavior in projectiles else None)
    unreal.EditorAssetLibrary.save_loaded_asset(skill, False)
    report.append({"asset": skill.get_path_name(), "behavior": behavior, "pose": pose, "windup": skill.get_editor_property("windup")})

pathlib.Path(unreal.Paths.project_saved_dir(), "SkillPresentationSetup.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("SKILL_PRESENTATION_SETUP_PASS: 20 skills + basic attack")

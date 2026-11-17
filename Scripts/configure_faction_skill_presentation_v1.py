"""Configure the first presentation pass on three faction skills."""
import json
from pathlib import Path

import unreal

SKILLS = "/Game/HPWH/Skills/Design/DA_Design_"
PARTICLES = "/Game/StarterContent/Particles/"
HIT = "/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/Hits/NS_Projectile_01_Hit"


def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing presentation asset: {path}")
    return asset


sparks = load(PARTICLES + "P_Sparks")
steam = load(PARTICLES + "P_Steam_Lit")
dust = load(PARTICLES + "P_Ambient_Dust")
hit = load(HIT)
plans = {
    "bronze_charged_hit": dict(attack_pose=unreal.CC_AttackPose.UPPERCUT, animation_play_rate=1.15,
                              cast_particles=sparks, impact_niagara=hit, effect_scale=.55, effect_lifetime=1.0),
    "bronze_guard": dict(attack_pose=getattr(unreal.CC_AttackPose, "NONE"),
                        cast_particles=sparks, impact_particles=steam, effect_scale=.60, effect_lifetime=1.1),
    "field_root_prison": dict(attack_pose=unreal.CC_AttackPose.CROSS_JAB, animation_play_rate=.88,
                              cast_particles=dust, impact_particles=steam, effect_scale=.75, effect_lifetime=1.4),
}
skills = {key: load(SKILLS + key) for key in plans}
report = {}
for key, plan in plans.items():
    skill = skills[key]
    props = dict(cast_particles=None, impact_particles=None, cast_niagara=None, impact_niagara=None,
                 alternate_attack_pose=False)
    props.update(plan)
    for name, value in props.items():
        skill.set_editor_property(name, value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(skill, False):
        raise RuntimeError(f"Failed to save {skill.get_path_name()}")
    report[key] = {
        name: value.get_path_name() if isinstance(value, unreal.Object) else str(value)
        for name, value in props.items()
    }

path = Path(unreal.Paths.project_saved_dir()) / "FactionSkillPresentationV1.json"
path.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("FACTION_SKILL_PRESENTATION_V1_READY")

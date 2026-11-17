"""Dedicated enemy melee loadout and camera defaults; preserve player skills and art."""
import unreal
import json
from pathlib import Path
ROOT = '/Game/HPWH'
tools = unreal.AssetToolsHelpers.get_asset_tools()
def data(name, cls, folder):
    path = ROOT + '/' + folder
    obj = unreal.load_asset(path + '/' + name)
    if not obj:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', cls)
        obj = tools.create_asset(name, path, cls, factory)
    return obj
skills = []
for i, (behavior, pose, power, cooldown, windup) in enumerate([
    ('MELEE', 'JAB', 8., 1.2, .2),
    ('COMBO', 'CROSS_JAB', 6., 2.4, .2),
    ('SWEEP', 'UPPERCUT', 10., 3., .3),
    ('KNOCKBACK', 'UPPERCUT', 12., 4., .32)], 1):
    skill = data('DA_Enemy_Melee_%02d' % i, unreal.CC_SkillDefinition, 'Skills/Enemies')
    values = dict(skill_id='EnemyMelee_%02d' % i, display_name=['近战直拳','近战连拳','近战横扫','近战重拳'][i-1],
        behavior=getattr(unreal.CC_SkillBehavior, behavior), attack_pose=getattr(unreal.CC_AttackPose, pose),
        ability_class=None, power=power, mana_cost=0., cooldown=cooldown, windup=windup,
        range=190., radius=190., angle=120., count=2 if i == 2 else 1, interval=.25,
        knockback_force=180., cast_particles=None, cast_niagara=None, projectile_niagara=None,
        impact_niagara=None, impact_particles=unreal.load_asset('/Game/StarterContent/Particles/P_Sparks'),
        effect_scale=.12, effect_lifetime=.3)
    for field, value in values.items(): skill.set_editor_property(field, value)
    unreal.EditorAssetLibrary.save_loaded_asset(skill, False)
    skills.append(skill)
loadout = unreal.load_asset(ROOT + '/Loadouts/DA_EnemyLoadout')
slots = []
for skill in skills:
    slot = unreal.CC_SkillSlot()
    slot.set_editor_property('skill', skill)
    slot.set_editor_property('level', 1)
    slots.append(slot)
loadout.set_editor_property('slots', slots)
unreal.EditorAssetLibrary.save_loaded_asset(loadout, False)
for name in ['BP_ProgressionEnemy', 'BP_PatrolEnemy', 'BP_TestDummy']:
    bp = unreal.load_asset(ROOT + '/Blueprints/' + name)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.get_editor_property('skills').set_editor_property('default_loadout', loadout)
    if name == 'BP_PatrolEnemy': cdo.set_editor_property('combat_distance', 150.)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
bp = unreal.load_asset(ROOT + '/Blueprints/BP_ProgressionCharacter')
cdo = unreal.get_default_object(bp.generated_class())
boom = cdo.get_camera_boom()
for field, value in dict(target_arm_length=600., target_offset=unreal.Vector(0,0,65),
        socket_offset=unreal.Vector(0,45,15), do_collision_test=True, probe_size=16.,
        probe_channel=unreal.CollisionChannel.ECC_CAMERA, use_pawn_control_rotation=True,
        enable_camera_lag=False).items(): boom.set_editor_property(field, value)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(ROOT + '/Maps/L_FlatTestArena')
count = 0
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.CC_EnemyCharacter):
        actor.get_editor_property('skills').set_editor_property('default_loadout', loadout)
        if isinstance(actor, unreal.CC_PatrolEnemyCharacter): actor.set_editor_property('combat_distance', 150.)
        count += 1
assert levels.save_current_level()
Path(unreal.Paths.project_saved_dir(), 'EnemyMeleeCameraSetup.json').write_text(json.dumps(dict(enemyCount=count, skills=[s.get_path_name() for s in skills], armLength=600, collisionProbe=16), indent=2), encoding='utf-8')
unreal.log('ENEMY_MELEE_CAMERA_SETUP_PASS: %d enemies' % count)

"""Balance/skills only; preserve imported art and existing arena placements. Run after C++ build."""
import unreal
import json
from pathlib import Path

ROOT = '/Game/HPWH'
tools = unreal.AssetToolsHelpers.get_asset_tools()

def data(name, cls, folder):
    asset = unreal.load_asset(f'{ROOT}/{folder}/{name}')
    if not asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', cls)
        asset = tools.create_asset(name, f'{ROOT}/{folder}', cls, factory)
    assert asset
    return asset

def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)

# First ranged skill is explicitly equipable. #05 takes the melee template's old role.
specs = [
 ('粒子远程', 'PROJECTILE', .75, 0., 2., '发射一枚粒子弹，命中单个敌人；伤害为普攻的75%。'),
 ('连击', 'COMBO', .6, 10., 3., '连续三次近战打击，每击分别结算伤害。'),
 ('横扫', 'SWEEP', 1.2, 10., 3., '攻击面前扇形内的敌人。'),
 ('突进', 'DASH', .8, 10., 4., '向前突进并攻击附近敌人，受场景碰撞阻挡。'),
 ('近战打击', 'MELEE', 1., 5., 1., '前方近战单次打击。'),
 ('散射', 'SPREAD', .45, 12., 4., '扇形发射三枚弹，每枚独立命中。'),
 ('追踪弹', 'HOMING', .65, 10., 3., '向目标发射追踪弹；未指定目标时沿瞄准方向飞行。'),
 ('穿透弹', 'PIERCING', .8, 12., 4., '穿过最多三个不同的敌人，每个敌人只命中一次。'),
 ('范围爆发', 'BURST', 1.5, 15., 5., '以自身为中心爆发范围伤害。'),
 ('落点打击', 'GROUND_STRIKE', 1.8, 18., 6., '短暂延迟后攻击瞄准落点范围。'),
 ('持续区域', 'DAMAGE_ZONE', .25, 20., 7., '在瞄准位置生成持续三秒的伤害区域，每0.5秒结算一跳，共6跳。'),
 ('直线打击', 'LINE_STRIKE', 1.25, 12., 4., '攻击前方直线通道内的敌人。'),
 ('自身治疗', 'HEAL', 30., 15., 5., '恢复自身生命，不超过最大生命。'),
 ('范围治疗', 'AREA_HEAL', 20., 20., 7., '恢复范围内同阵营角色的生命。'),
 ('护盾', 'SHIELD', 40., 15., 8., '获得吸收伤害的护盾，持续5秒；再次施放替换现有护盾。'),
 ('防御强化', 'DEFENSE_BUFF', 25., 15., 8., '暂时增加防御值，持续5秒。'),
 ('攻击强化', 'ATTACK_BUFF', 7.5, 15., 8., '暂时增加实际攻击力，持续5秒，同时影响按攻击力缩放的技能。'),
 ('移速强化', 'SPEED_BUFF', 150., 10., 8., '暂时增加移动速度，持续4秒。'),
 ('减速', 'SLOW', 0., 12., 6., '范围内敌人移动速度降低40%，持续3秒。'),
 ('击退', 'KNOCKBACK', 1., 12., 5., '造成伤害并击退仍存活的敌人。'),
]
projectile = unreal.load_asset('/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/NS_Projectile_01')
impact = unreal.load_asset('/Game/LanTuRebuild/Effects/MixedVFX/Particles/Projectiles/Hits/NS_Projectile_01_Hit')
assert projectile and impact
skills = []
for i, (name, behavior, power, mana, cooldown, description) in enumerate(specs, 1):
    skill = data(f'DA_Skill_{i:02d}', unreal.CC_SkillDefinition, 'Skills')
    ranged = behavior in ('PROJECTILE', 'SPREAD', 'HOMING', 'PIERCING')
    values = dict(skill_id=f'Skill_{i:02d}', display_name=f'{i:02d} · {name}', description=description,
        behavior=getattr(unreal.CC_SkillBehavior, behavior), power=power,
        power_mode=unreal.CC_SkillPowerMode.ATTACK_MULTIPLIER if i <= 12 or i == 20 else unreal.CC_SkillPowerMode.FLAT,
        power_per_level=0., mana_cost=mana, cooldown=cooldown, count=3,
        duration=5. if 15 <= i <= 17 else 4. if i == 18 else 3., interval=.5,
        range=1200. if ranged or i in (10,11,12) else 400. if i == 4 else 250.,
        radius=200., angle=25. if i == 6 else 120., knockback_force=400.,
        projectile_niagara=projectile if ranged else None, impact_niagara=impact if ranged else None,
        attack_pose=unreal.CC_AttackPose.CROSS_JAB if ranged or i == 2 else unreal.CC_AttackPose.JAB,
        slow_fraction=.4, windup=.16, projectile_speed=1200.)
    for field, value in values.items(): skill.set_editor_property(field, value)
    save(skill)
    skills.append(skill)
basic = unreal.load_asset(ROOT + '/Skills/DA_BasicAttack')
for field, value in dict(power_mode=unreal.CC_SkillPowerMode.ATTACK_MULTIPLIER, power=1., power_per_level=0., mana_cost=0., description='普通攻击：当前攻击力的100%。').items():
    basic.set_editor_property(field, value)
save(basic)
melee = []
for i, coefficient in enumerate([1., .6, 1.2, 1.5], 1):
    s = unreal.load_asset(ROOT + f'/Skills/Enemies/DA_Enemy_Melee_{i:02d}')
    s.set_editor_property('power_mode', unreal.CC_SkillPowerMode.ATTACK_MULTIPLIER)
    s.set_editor_property('power', coefficient)
    s.set_editor_property('power_per_level', 0.)
    save(s)
    melee.append(s)
normal_loadout = unreal.load_asset(ROOT + '/Loadouts/DA_EnemyLoadout')
elite_loadout = data('DA_EliteLoadout', unreal.CC_SkillLoadout, 'Loadouts')
slots=[]
for s in melee[:3]+[skills[0]]:
    slot=unreal.CC_SkillSlot(); slot.set_editor_property('skill',s); slots.append(slot)
elite_loadout.set_editor_property('slots',slots); save(elite_loadout)

balance=data('DA_CombatBalance',unreal.CC_CombatBalance,'Balance')
balance.set_editor_property('base_health',100.)
balance.set_editor_property('base_attack',15.)
routes=[('Nature','Native',['竹路人','竹节人','竹拳击手'],[1.,2.5,4.5],[1.,2.5,4.5]),
        ('Bronze','Artifact_01',['铜机械小兵','铜械战士','铜械将军'],[1.4,3.,5.],[1.4,3.,5.]),
        ('Ceramic','Artifact_02',['陶童子','陶土行者','陶土僧人'],[1.6,3.5,7.],[1.,2.5,4.5])]
profiles=[]
report=[]
for route, form_id, models, hp, atk in routes:
    form=unreal.load_asset(ROOT+'/Forms/DA_Form_'+form_id)
    stages=form.get_editor_property('stages')
    for tier, model in enumerate(models):
        key=f'{route}_T{tier+1}'
        p=unreal.CC_CombatProfile(); p.set_editor_property('profile_id',key)
        p.set_editor_property('mesh',unreal.load_asset(f'/Game/SOURCE/小怪/{model}/SK_{model}'))
        p.set_editor_property('health_multiplier',hp[tier]); p.set_editor_property('attack_multiplier',atk[tier])
        p.set_editor_property('loadout',normal_loadout); profiles.append(p)
        stage=stages[tier]; stage.set_editor_property('combat_balance',balance); stage.set_editor_property('stat_profile_id',key)
        stage.set_editor_property('max_health',hp[tier]*100); stage.set_editor_property('attack',atk[tier]*15)
        stage.set_editor_property('defense',0.); stage.set_editor_property('max_mana',100.)
        # Ranged #01 is manual-only by default, replacing old melee references with new melee #05.
        loadout=stage.get_editor_property('loadout'); loadslots=loadout.get_editor_property('slots')
        originally_has_first = any(sl.get_editor_property('skill') == skills[0] for sl in loadslots)
        for j, sl in enumerate(loadslots):
            if originally_has_first and sl.get_editor_property('skill') == skills[4]:
                sl.set_editor_property('skill',skills[1]); loadslots[j]=sl
            if sl.get_editor_property('skill')==skills[0]:
                sl.set_editor_property('skill',skills[4]); loadslots[j]=sl
        loadout.set_editor_property('slots',loadslots); save(loadout)
        stages[tier]=stage
        report.append(dict(profile=key,model=model,health=100*hp[tier],attack=15*atk[tier]))
    form.set_editor_property('stages',stages); save(form)
for key,parent,model,hp,atk in [('Bronze_Elite','Bronze_T3','铜械降神',750.,112.5),('Ceramic_Elite','Ceramic_T3','陶土僧人',1050.,101.25)]:
    p=unreal.CC_CombatProfile(); p.set_editor_property('profile_id',key); p.set_editor_property('parent_profile_id',parent)
    # The ceramic elite shares the normal advanced mesh; select by explicit ID.
    p.set_editor_property('mesh',unreal.load_asset(f'/Game/SOURCE/小怪/{model}/SK_{model}') if key=='Bronze_Elite' else None)
    p.set_editor_property('elite',True); p.set_editor_property('elite_multiplier',1.5); p.set_editor_property('elite_scale',1.2)
    p.set_editor_property('loadout',elite_loadout); profiles.append(p)
    report.append(dict(profile=key,model=model,health=hp,attack=atk,projectileDamage=atk*.75,scale=1.2))
# This model stays bamboo. Its unspecified custom stats start from advanced Nature, editable independently.
p=unreal.CC_CombatProfile(); p.set_editor_property('profile_id','Nature_PotHead'); p.set_editor_property('parent_profile_id','Nature_T3')
p.set_editor_property('mesh',unreal.load_asset('/Game/SOURCE/小怪/陶罐头/SK_陶罐头')); p.set_editor_property('loadout',normal_loadout); profiles.append(p)
balance.set_editor_property('profiles',profiles); save(balance)
ps=unreal.load_asset(ROOT+'/Blueprints/BP_ProgressionPlayerState')
unreal.get_default_object(ps.generated_class()).get_editor_property('progression').set_editor_property('skill_catalog',skills)
unreal.BlueprintEditorLibrary.compile_blueprint(ps); save(ps)
for name in ('BP_ProgressionEnemy','BP_PatrolEnemy','BP_TestDummy'):
    bp=unreal.load_asset(ROOT+'/Blueprints/'+name); unreal.get_default_object(bp.generated_class()).set_editor_property('combat_balance',balance)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_FlatTestArena')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.CC_EnemyCharacter): actor.set_editor_property('combat_balance',balance)
# Two usable patrol parents for explicit elite placement, no edits to source art.
for key,model,location in [('Bronze_Elite','铜械降神',unreal.Vector(2200,1600,100)),('Ceramic_Elite','陶土僧人',unreal.Vector(2800,1600,100))]:
    name='BP_'+key
    bp=unreal.load_asset(ROOT+'/Blueprints/'+name)
    if not bp:
        factory=unreal.BlueprintFactory(); factory.set_editor_property('parent_class',unreal.load_class(None,ROOT+'/Blueprints/BP_PatrolEnemy.BP_PatrolEnemy_C'))
        bp=tools.create_asset(name,ROOT+'/Blueprints',unreal.Blueprint,factory)
    cdo=unreal.get_default_object(bp.generated_class()); cdo.set_editor_property('combat_balance',balance); cdo.set_editor_property('stat_profile_id',key)
    mesh=unreal.load_asset(f'/Game/SOURCE/小怪/{model}/SK_{model}'); comp=cdo.get_editor_property('mesh'); comp.set_editor_property('skeletal_mesh_asset',mesh)
    scale=180./(mesh.get_bounds().box_extent.z*2.); comp.set_relative_scale3d(unreal.Vector(scale,scale,scale))
    comp.set_editor_property('anim_class',unreal.load_class(None,ROOT+f'/Locomotion/ABP_{model}.ABP_{model}_C'))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
    if not any(a.get_actor_label()==key for a in actors.get_all_level_actors()):
        actor=actors.spawn_actor_from_class(bp.generated_class(),location); actor.set_actor_label(key)
assert levels.save_current_level()
Path(unreal.Paths.project_saved_dir(),'CombatBalanceSetup.json').write_text(json.dumps(dict(profiles=report,skills=[s.get_path_name() for s in skills]),ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('COMBAT_BALANCE_SETUP_PASS')

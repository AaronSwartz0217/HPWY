"""Bind the supplied skill artwork to real GAS skills; configure school gates, XP and coins.
Run after compiling C++; preserve source art, existing placements and legacy 22 IDs.
"""
import unreal,json,time
from pathlib import Path
R='/Game/HPWH'; project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
tools=unreal.AssetToolsHelpers.get_asset_tools()
def save(a):assert unreal.EditorAssetLibrary.save_loaded_asset(a,False)
def asset(name,cls,folder):
 a=unreal.load_asset(folder+'/'+name)
 if not a:
  f=unreal.DataAssetFactory();f.set_editor_property('data_asset_class',cls);a=tools.create_asset(name,folder,cls,f)
 assert a;return a
balance=unreal.load_asset(R+'/Balance/DA_CombatBalance');profiles=balance.get_editor_property('profiles');rewards={}
for i,p in enumerate(profiles):
 key=str(p.get_editor_property('profile_id'))
 reward=500 if p.get_editor_property('elite') else 10 if key.endswith('_T1') else 40 if key.endswith('_T2') else 200 if key.endswith('_T3') else -1
 p.set_editor_property('experience_reward',reward);profiles[i]=p;rewards[key]=reward
balance.set_editor_property('profiles',profiles);save(balance)
source=json.loads((project/'SourceArt/UI/SkillDesign_v0.1.json').read_text(encoding='utf-8-sig'))
gallery=unreal.load_asset('/Game/UI/MuralMenu/DA_SkillGallery');entries=gallery.get_editor_property('entries')
byid={str(e.get_editor_property('design_id')):i for i,e in enumerate(entries)}
mechanics={'basic_heal':'HEAL_PERCENT','bronze_summon':'SUMMON','bronze_evolve':'STAT_BOOST','bronze_charged_hit':'CHARGE','bronze_guard':'GUARD','temple_golden_body':'INVULNERABLE','temple_gravity':'GRAVITY','temple_lamps':'PULSE_BURST','temple_cleanse':'CLEANSE_SHIELD','field_stealth':'STEALTH','field_regrowth':'REGROWTH','field_lifesteal':'LIFESTEAL','field_root_prison':'ROOT_AREA'}
required={'青铜天卫':'Artifact_01','古刹法相':'Artifact_02','田野陶灵':'Native'}
skills={};bindings=[]
particle=unreal.load_asset(R+'/Skills/DA_Skill_01');impact=particle.get_editor_property('impact_niagara')
castfx=unreal.load_asset(R+'/Skills/DA_Skill_13').get_editor_property('cast_niagara')
for d in source['skills']:
 id=d['id'];s=particle if id=='basic_particle' else asset('DA_Design_'+id,unreal.CC_SkillDefinition,R+'/Skills/Design')
 behavior='COMBO' if id=='basic_combo' else 'SWEEP' if id=='basic_sweep' else 'HEAL' if id=='basic_heal' else 'PROJECTILE' if id=='basic_particle' else 'MELEE'
 vals=dict(required_form_id=required.get(d['faction'],'None'),design_mechanic=getattr(unreal.CC_DesignMechanic,mechanics.get(id,'NONE')))
 if id=='basic_particle':
  vals['description']=d['effect']
 else:
  damages=d.get('damage_attack_percent',[])
  vals.update(skill_id='Design_'+id,display_name=d['name'],description=d['effect'],behavior=getattr(unreal.CC_SkillBehavior,behavior),
   power_mode=unreal.CC_SkillPowerMode.ATTACK_MULTIPLIER if damages or id=='bronze_charged_hit' else unreal.CC_SkillPowerMode.FLAT,
   power=(damages[0]/100. if damages else 2. if id=='bronze_charged_hit' else 0.),power_per_level=0.,mana_cost=0.,cooldown=d['cooldown_seconds'],
   duration=d.get('duration_seconds',0),range=d.get('range_cm',d.get('cast_range_cm',250)),radius=d.get('radius_cm',200),
   angle=d.get('arc_degrees',d.get('guard_arc_degrees',90)),count=d.get('pulse_count',d.get('tick_count',len(damages) if id=='basic_combo' else d.get('summon',{}).get('count',1))),
   interval=d.get('pulse_interval_seconds',d.get('tick_interval_seconds',.6 if id=='basic_combo' else .5)),
   windup=0. if id=='bronze_charged_hit' else .12,
   damage_steps=[v/100. for v in damages] if id=='basic_combo' else [],
   heal_fraction=d.get('heal_max_health_percent',d.get('heal_per_tick_max_health_percent',d.get('lifesteal_actual_health_damage_percent',0)))/100.,
   attack_factor=d.get('attack_stat_total_percent',100)/100.,defense_factor=d.get('defense_stat_total_percent',100)/100.,speed_factor=d.get('movement_speed_total_percent',100)/100.,
   control_duration=d.get('root_seconds',d.get('float_seconds',d.get('full_charge_stagger_seconds',d.get('stagger_seconds',0)))),
   shield_health_fraction=d.get('shield_max_health_percent',0)/100.,shield_defense_factor=d.get('shield_defense_percent',0)/100.,
   max_targets=1 if id in ('basic_straight_punch','basic_combo','basic_heavy_punch','bronze_charged_hit') else 0,
   max_charge_seconds=d.get('charge_max_seconds',2.),
   attack_pose=unreal.CC_AttackPose.UPPERCUT if id in ('basic_heavy_punch','bronze_charged_hit','temple_gravity') else unreal.CC_AttackPose.CROSS_JAB if id in ('basic_combo','basic_sweep','temple_lamps') else unreal.CC_AttackPose.JAB,
   impact_niagara=impact,effect_scale=.5 if d['faction']!='通用系' else .3)
 for k,v in vals.items():s.set_editor_property(k,v)
 save(s);skills[id]=s
 i=byid[id];e=entries[i];e.set_editor_property('executable_skill',s)
 if id=='basic_particle':
  e.set_editor_property('description',d['effect'])
  e.set_editor_property('stat_labels',['伤害倍率','冷却时间','作用距离','法力消耗']);e.set_editor_property('stat_values',['75%攻击力','2秒','12米','0'])
 entries[i]=e;bindings.append({'designId':id,'skill':s.get_path_name(),'requiresPower':required.get(d['faction'],'General')})
gallery.set_editor_property('entries',entries);save(gallery)
ps=unreal.load_asset(R+'/Blueprints/BP_ProgressionPlayerState');prog=unreal.get_default_object(ps.generated_class()).get_editor_property('progression')
catalog=list(prog.get_editor_property('skill_catalog'))
for s in skills.values():
 if s not in catalog:catalog.append(s)
prog.set_editor_property('skill_catalog',catalog);unreal.BlueprintEditorLibrary.compile_blueprint(ps);save(ps)
loadouts={'InitialInk':['basic_straight_punch','basic_combo','basic_sweep','basic_heal'],'Native':['field_stealth','field_regrowth','field_lifesteal','field_root_prison'],
 'Artifact_01':['bronze_summon','bronze_evolve','bronze_charged_hit','bronze_guard'],'Artifact_02':['temple_golden_body','temple_gravity','temple_lamps','temple_cleanse']}
for fid,ids in loadouts.items():
 form=unreal.load_asset(R+'/Forms/DA_Form_'+fid);stages=form.get_editor_property('stages')
 load=asset('DA_DesignLoadout_'+fid,unreal.CC_SkillLoadout,R+'/Loadouts');slots=[]
 for id in ids:
  v=unreal.CC_SkillSlot();v.set_editor_property('skill',skills[id]);slots.append(v)
 load.set_editor_property('slots',slots);save(load)
 for i,stage in enumerate(stages):stage.set_editor_property('loadout',load);stages[i]=stage
 form.set_editor_property('stages',stages);save(form)
# Migrate old coin visuals into a classified destination; UE updates package references.
source='/Game/五铢钱动画/五铢钱动画';dest=R+'/Coins/Art'
rename={'五铢钱':'SK_WuzhuCoin','五铢钱_Skeleton':'SKEL_WuzhuCoin','五铢钱_PhysicsAsset':'PHYS_WuzhuCoin','五铢钱_Anim':'A_WuzhuCoin_Move','五铢钱待机':'A_WuzhuCoin_Idle','材质':'M_WuzhuCoin','材质_001':'M_WuzhuCoin_001','五铢钱_Normal':'T_WuzhuCoin_Normal','五铢钱_-_副本':'T_WuzhuCoin_BaseColor','钱叫声':'S_WuzhuCoin_Pickup'}
if not unreal.EditorAssetLibrary.does_asset_exist(dest+'/SK_WuzhuCoin'):
 reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.scan_paths_synchronous([source],True)
 originals={old:unreal.load_asset(source+'/'+old) for old in rename}
 assert all(originals.values()),'Source mesh/material/animation/audio must all load before batch rename'
 assert tools.rename_assets([unreal.AssetRenameData(originals[old],dest,new) for old,new in rename.items()])
 for original in originals.values():save(original)
mesh=unreal.load_asset(dest+'/SK_WuzhuCoin');anim=unreal.load_asset(dest+'/A_WuzhuCoin_Idle');sound=unreal.load_asset(dest+'/S_WuzhuCoin_Pickup')
assert mesh and anim and sound
animation_audit=[]
for name in ('A_WuzhuCoin_Idle','A_WuzhuCoin_Move'):
 clip=unreal.load_asset(dest+'/'+name);model=clip.data_model_interface;record={'name':name,'sourceRate':str(model.get_frame_rate()),'keys':model.get_number_of_keys()}
 for prop in ('platform_target_frame_rate','target_frame_rate','number_of_sampled_keys','number_of_sampled_frames'):
  try:record[prop]=str(clip.get_editor_property(prop))
  except Exception as error:record[prop]=str(error)
 animation_audit.append(record)
(project/'Saved/CoinAnimationAudit.json').write_text(json.dumps(animation_audit,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('COIN_ASSETS_LOADED_AND_ANIMATION_AUDITED')
# Verify all dependencies after rename; preserve redirectors rather than deleting by hand.
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.scan_paths_synchronous([dest],True);deps={};missing=[]
for new in rename.values():
 package=dest+'/'+new;deps[package]=[str(x) for x in (reg.get_dependencies(package,unreal.AssetRegistryDependencyOptions(True,True,True,True,True)) or [])]
 for dep in deps[package]:
  if dep.startswith('/Game/') and not unreal.EditorAssetLibrary.does_asset_exist(dep):missing.append(dep)
assert not missing,missing
bp=unreal.load_asset(R+'/Coins/BP_CopperCoin')
if not bp:
 f=unreal.BlueprintFactory();f.set_editor_property('parent_class',unreal.load_class(None,'/Script/CrashCourse.CC_CoinPickup'));bp=tools.create_asset('BP_CopperCoin',R+'/Coins',unreal.Blueprint,f)
cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('pickup_sound',sound)
comp=cdo.get_editor_property('coin_mesh');comp.set_editor_property('skeletal_mesh_asset',mesh)
height=mesh.get_bounds().box_extent.z*2.;scale=80./max(height,1);comp.set_relative_scale3d(unreal.Vector(scale,scale,scale))
coin_abp=unreal.load_asset(dest+'/ABP_WuzhuCoin')
assert coin_abp,'Run configure_coin_patrol.py to import the original animation blueprint first'
comp.set_animation_mode(unreal.AnimationMode.ANIMATION_BLUEPRINT)
comp.set_anim_instance_class(coin_abp.generated_class())
half=cdo.get_editor_property('capsule_component').get_editor_property('capsule_half_height')
comp.set_relative_location(unreal.Vector(0,0,-half),False,False)
comp.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
cdo.set_editor_property('copper_amount',5);cdo.set_editor_property('experience_amount',5)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
unreal.log('COIN_BLUEPRINT_SAVED')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level(R+'/Maps/L_FlatTestArena')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);placed=actors.get_all_level_actors()
for i in range(5):
 label='CopperCoin_Test_%02d'%i
 if not any(a.get_actor_label()==label for a in placed):
  unreal.log('COIN_SPAWN_BEGIN_'+str(i));a=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(200+i*190,-480,110));a.set_actor_label(label);unreal.log('COIN_SPAWN_OK_'+str(i))
for actor in actors.get_all_level_actors():
 component=actor.get_component_by_class(unreal.TextRenderComponent)
 if component and '10 XP' in str(component.get_editor_property('text')):
  component.set_text('TIER XP | LOW 10 / MID 40 / HIGH 200 / ELITE 500')
if not any(a.get_actor_label()=='CopperCoin_RewardHint' for a in actors.get_all_level_actors()):
 hint=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(500,-480,240),unreal.Rotator(0,180,0))
 hint.set_actor_label('CopperCoin_RewardHint');text=hint.get_component_by_class(unreal.TextRenderComponent)
 text.set_text('COPPER COINS | TOUCH +5 COPPER / +5 XP');text.set_world_size(24.)
# Keep map instances' editor values consistent with their model profile as well as runtime application.
for a in actors.get_all_level_actors():
 if isinstance(a,unreal.CC_EnemyCharacter):
  pid=str(a.get_editor_property('stat_profile_id'));profile=next((p for p in profiles if str(p.get_editor_property('profile_id'))==pid),None)
  if profile is None:
   sk=a.get_editor_property('mesh').get_editor_property('skeletal_mesh_asset');profile=next((p for p in profiles if p.get_editor_property('mesh')==sk and sk),None)
  if profile:
   value=profile.get_editor_property('experience_reward')
   if value<0:value=200 if str(profile.get_editor_property('parent_profile_id')).endswith('_T3') else 10
   a.set_editor_property('experience_reward',value)
assert levels.save_current_level()
result={'realEquipment':True,'designBindings':bindings,'catalogCount':len(catalog),'profileRewards':rewards,'coin':{'mesh':mesh.get_path_name(),'animation':anim.get_path_name(),'sound':sound.get_path_name(),'copper':5,'experience':5,'instances':5,'sourceEngine':'5.5','loadedAndSavedIn':'5.6','missingDependencies':missing},'dependencies':deps}
(project/'Saved/RealSkillsCoinSetup.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('REAL_SKILLS_COINS_SETUP_PASS')
quit_at=time.monotonic()+3

def finish(delta):
 if time.monotonic()>quit_at:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)

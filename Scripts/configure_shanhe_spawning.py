"""Install bamboo spawn variants and one population director only in Shanhe."""
import unreal,json
from pathlib import Path
ROOT='/Game/HPWH/Spawning'
tools=unreal.AssetToolsHelpers.get_asset_tools()
def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
def blueprint(name,parent):
    result=unreal.load_asset(ROOT+'/'+name)
    if not result:
        factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',parent)
        result=tools.create_asset(name,ROOT,unreal.Blueprint,factory)
    assert result
    return result

source=unreal.load_asset('/Game/HPWH/Blueprints/BP_PatrolEnemy')
assert source
stages=unreal.load_asset('/Game/HPWH/Forms/DA_Form_Native').get_editor_property('stages')
balance=unreal.load_asset('/Game/HPWH/Balance/DA_CombatBalance')
library=unreal.load_asset('/Game/HPWH/Locomotion/DA_NPCLocomotion')
entries=[]
for tier,name,weight,limit in [(0,'BP_WildBamboo_T1',72.,12),(1,'BP_WildBamboo_T2',24.,12),(2,'BP_WildBamboo_T3',4.,3)]:
    bp=unreal.load_asset(ROOT+'/'+name)
    if not bp:bp=unreal.EditorAssetLibrary.duplicate_loaded_asset(source,ROOT+'/'+name)
    obj=unreal.get_default_object(bp.generated_class());stage=stages[tier]
    obj.set_editor_property('combat_balance',balance)
    obj.set_editor_property('stat_profile_id','Nature_T'+str(tier+1))
    obj.set_editor_property('replicates',True)
    obj.set_editor_property('patrol_radius',500.)
    obj.set_editor_property('detection_radius',900.)
    obj.set_editor_property('max_chase_distance',1800.)
    obj.set_editor_property('start_patrolling',True)
    obj.set_editor_property('detect_players',True)
    obj.get_editor_property('locomotion').set_editor_property('library',library)
    mesh=obj.get_editor_property('mesh')
    mesh.set_editor_property('skeletal_mesh_asset',stage.get_editor_property('mesh'))
    mesh.set_editor_property('override_materials',[])
    mesh.set_editor_property('relative_scale3d',stage.get_editor_property('mesh_scale'))
    mesh.set_editor_property('relative_location',unreal.Vector(0,0,-96))
    mesh.set_editor_property('relative_rotation',unreal.Rotator(pitch=0,yaw=-90,roll=0))
    mesh.set_editor_property('anim_class',stage.get_editor_property('anim_class'))
    obj.get_editor_property('capsule_component').set_capsule_size(42.,96.,False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    entry=unreal.CC_AmbientSpawnEntry()
    for prop,value in [('enemy_class',bp.generated_class()),('weight',weight),('minimum_difficulty_tier',tier),('class_limit',limit)]:entry.set_editor_property(prop,value)
    entries.append(entry)

bp=blueprint('BP_ShanheSpawnDirector',unreal.CC_AmbientSpawnDirector)
cdo=unreal.get_default_object(bp.generated_class())
cdo.set_editor_property('entries',entries)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/HPWH/Maps/L_FlatTestArena')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
directors=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.CC_AmbientSpawnDirector)]
if directors:
    director=directors[0]
    for duplicate in directors[1:]:actors.destroy_actor(duplicate)
else:director=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,0))
director.set_actor_label('山河社稷_刷怪管理器')
director.set_editor_property('entries',entries)
legacy=[]
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.CC_EnemyCharacter):
        legacy.append({'name':actor.get_actor_label(),'class':actor.get_class().get_name(),'patrol':isinstance(actor,unreal.CC_PatrolEnemyCharacter)})
levels.save_current_level()
Path(unreal.Paths.project_saved_dir(),'ShanheSpawnSetup.json').write_text(json.dumps({'director':director.get_path_name(),'entries':[{'class':str(e.get_editor_property('enemy_class')),'weight':e.get_editor_property('weight')} for e in entries],'existingEnemies':legacy},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('SHANHE_SPAWNING_CONFIGURED')

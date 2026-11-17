import unreal,json,hashlib
from pathlib import Path
p=Path(unreal.Paths.project_dir()).resolve();root='/Game/LanTuRebuild/Maps/初始房间/场景优化流送蓝图/开关门加载地图';level='/Game/LanTuRebuild/Maps/初始房间/场景优化'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level(level)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def snapshot():
 out={}
 for a in sub.get_all_level_actors():
  if '开关门加载地图' not in a.get_class().get_name():continue
  t=a.get_actor_transform();loc=a.get_actor_location();rot=a.get_actor_rotation();scale=a.get_actor_scale3d()
  row={'location':[loc.x,loc.y,loc.z],'rotation':[rot.pitch,rot.yaw,rot.roll],'scale':[scale.x,scale.y,scale.z],'components':{}}
  for c in a.get_components_by_class(unreal.StaticMeshComponent):
   loc=c.get_editor_property('relative_location');rot=c.get_editor_property('relative_rotation')
   row['components'][c.get_name()]={'location':[loc.x,loc.y,loc.z],'rotation':[rot.pitch,rot.yaw,rot.roll]}
  out[a.get_actor_label()]=row
 return out
before=snapshot();assert len(before)==3
original=p/'Content/LanTuRebuild/Maps/初始房间/restartlife.umap';original_hash=hashlib.sha256(original.read_bytes()).hexdigest()
bp=unreal.load_asset(root);assert bp
unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.CC_InteractiveDoubleDoor)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
after=snapshot();assert before==after,'Door placement or component transforms changed'
for a in sub.get_all_level_actors():
 if not isinstance(a,unreal.CC_InteractiveDoubleDoor):continue
 a.set_editor_property('left_open_angle',90.);a.set_editor_property('right_open_angle',-90.);a.set_editor_property('open_duration',.8)
 for c in a.get_components_by_class(unreal.StaticMeshComponent):c.set_mobility(unreal.ComponentMobility.MOVABLE)
assert levels.save_current_level()
assert original_hash==hashlib.sha256(original.read_bytes()).hexdigest()
report={'map':level,'blueprint':root,'doors':before,'placement_unchanged':True,'relative_offsets':[90,-90],'duration_seconds':.8,'restartlife_unchanged':True,'parent':'CC_InteractiveDoubleDoor'}
(p/'Saved/DoorInteractionSetup.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(p/'资源备份/开关门交互_20261103/修改清单.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('DOOR_SETUP_PASS')

import unreal,json,hashlib,shutil
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve();src='/Game/LanTuRebuild/Maps/初始房间/restartlife';dst='/Game/关卡优化/测试/L_RestartRoomVisibility_Test'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);lib=unreal.EditorAssetLibrary
assert not lib.does_asset_exist(dst),'Trial map already exists; refusing overwrite'
original=project/'Content/LanTuRebuild/Maps/初始房间/restartlife.umap';before=hashlib.sha256(original.read_bytes()).hexdigest()
backup=project/'资源备份/房间可见性试装_20261102';backup.mkdir(parents=True,exist_ok=True)
shutil.copy2(original,backup/'restartlife_原始未改动.umap')
assert levels.new_level_from_template(dst,src)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors=sub.get_all_level_actors()
names=['ccha_yi_hu','ccha_yi_hu_001','cha_bei004_005','家具2','家具1_002','家具1_003','家具1_004','家具1_005','家具1_006','家具1_007','家具1_008']
bylabel={a.get_actor_label():a for a in actors};assert all(n in bylabel for n in names)
items=[bylabel[n] for n in names];lights=[bylabel[n] for n in ['P1','P2']]
for a in items:
 p=a.get_actor_location();assert 450<p.x<1550 and -650<p.y<1400,a.get_actor_label()
klass=lib.load_blueprint_class('/Game/关卡优化/BP_RoomVisibility');assert klass
c=sub.spawn_actor_from_class(klass,unreal.Vector(1200,350,70));assert c
c.set_actor_label('RoomVisibility_WestRoom_Trial');c.set_folder_path('关卡优化/西侧房间试装')
c.get_editor_property('trigger_box').set_box_extent(unreal.Vector(1000,1300,450),False)
c.set_editor_property('interior_actors',items);c.set_editor_property('room_lights',lights);c.set_editor_property('room_effects',[])
c.set_editor_property('manage_interior_actor_tick',False)
checks={}
original_hidden={a.get_name():a.get_editor_property('hidden') for a in items}
c.set_room_active(False)
checks['all_11_interiors_hidden']=all(a.get_editor_property('hidden') for a in items)
checks['both_local_lights_hidden']=all(not a.get_component_by_class(unreal.LightComponent).get_editor_property('visible') for a in lights)
c.set_room_active(True)
checks['original_visibility_restored']=all(a.get_editor_property('hidden')==original_hidden[a.get_name()] for a in items)
assert all(checks.values()),checks
assert levels.save_current_level()
assert hashlib.sha256(original.read_bytes()).hexdigest()==before
report={'original':src,'trial':dst,'original_sha256':before,'original_unchanged':True,'controller':c.get_path_name(),'interiors':[a.get_actor_label() for a in items],'lights':['P1','P2'],'effects':[],'trigger_center':[1200,350,70],'trigger_half_extent':[1000,1300,450],'checks':checks,'backup':str(backup)}
(project/'Saved/RoomVisibilityTrial/installation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(backup/'试装清单.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(backup/'回退说明.txt').write_text('原始restartlife没有修改，可直接继续打开原关卡。\n测试关卡：'+dst+'\n只在测试副本加入一个控制器，管理11个西侧房间内饰和P1/P2灯光。\n在测试关卡移除RoomVisibility_WestRoom_Trial即可取消试装。\n没有修改材质、网格、外墙、地面、门窗、碰撞或原关卡。\n备份umap保留原文件，仅在原路径发生意外变动时恢复至原路径；正常回退无需覆盖文件。',encoding='utf-8')
unreal.log('ROOM_TRIAL_INSTALL_PASS')

import unreal,json,shutil,hashlib
from pathlib import Path
p=Path(unreal.Paths.project_dir()).resolve();b=p/'资源备份/开关门交互_20261103';b.mkdir(parents=True,exist_ok=True)
files=['Content/LanTuRebuild/Maps/初始房间/场景优化.umap','Content/LanTuRebuild/Maps/初始房间/场景优化流送蓝图/开关门加载地图.uasset','Source/CrashCourse/Private/Player/CC_PlayerController.cpp']
for rel in files:
 dst=b/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p/rel,dst)
level='/Game/LanTuRebuild/Maps/初始房间/场景优化';assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(level)
r={'doors':[],'blueprint':{}}
bp=unreal.load_asset('/Game/LanTuRebuild/Maps/初始房间/场景优化流送蓝图/开关门加载地图')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if '开关门加载地图' not in a.get_class().get_name():continue
 row={'name':a.get_name(),'label':a.get_actor_label(),'transform':str(a.get_actor_transform()),'components':[]}
 for c in a.get_components_by_class(unreal.SceneComponent):
  item={'name':c.get_name(),'class':c.get_class().get_name(),'relative_location':str(c.get_editor_property('relative_location')),'relative_rotation':str(c.get_editor_property('relative_rotation')),'parent':c.get_attach_parent().get_name() if c.get_attach_parent() else None}
  if isinstance(c,unreal.StaticMeshComponent):item['mesh']=str(c.get_editor_property('static_mesh'))
  row['components'].append(item)
 r['doors'].append(row)
(p/'Saved/DoorSetupInspection.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')


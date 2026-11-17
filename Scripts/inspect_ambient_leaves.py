import unreal,json
from pathlib import Path
bp='/Game/场景资源/China_townnight/AmbientFx/Blueprints/BP_Ambient_Game_Fx'
c=unreal.EditorAssetLibrary.load_blueprint_class(bp)
names=['Ambient_Type','Air_Leaf_SpawnRate','Air_Leaf_Lifetime','Air_Leaf_Gravity','Air_Leaf_WindStrength','Air_Leaf_StormStrength','Ground_Leaf_SpawnRate','Ground_Leaf_Lifetime','Ground_Leaf_WindStrength','Ground_Leaf_StormStrength','Scan Visualization?','Scan_Visualization']
def props(a):
 d={}
 for n in names:
  try:d[n]=str(a.get_editor_property(n))
  except:pass
 return d
r={'defaults':props(unreal.get_default_object(c)),'actors':[]}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LanTuRebuild/Maps/初始房间/restartlife')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if a.get_class()==c:r['actors'].append({'name':a.get_name(),'label':a.get_actor_label(),'props':props(a)})
Path(unreal.Paths.project_saved_dir(),'AmbientLeafAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')

import unreal,json
from pathlib import Path
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
rows=[];unreal._cc_probe_lights=[]
for a in actors:
 for c in a.get_components_by_class(unreal.LightComponentBase):
  row={'actor':a.get_actor_label(),'component':c.get_class().get_name()}
  for k in ['intensity','indirect_lighting_intensity','light_color','visible','mobility']:
   try:row[k]=str(c.get_editor_property(k))
   except Exception:pass
  rows.append(row)
  if isinstance(c,unreal.PointLightComponent):
   unreal._cc_probe_lights.append((c,c.get_editor_property('visible')))
   c.set_visibility(False)
Path(unreal.Paths.project_saved_dir(),'LiveCourtyardLightProbe.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,'ShowFlag.Specular 2')
unreal.SystemLibrary.execute_console_command(world,'ShowFlag.DirectLighting 2')
unreal.log('LIVE_COURTYARD_POINT_LIGHTS_TEMP_DISABLED')

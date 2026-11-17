import unreal,json,collections
from pathlib import Path
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LanTuRebuild/Maps/初始房间/restartlife')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors();r={'actor_count':len(actors),'classes':dict(collections.Counter(a.get_class().get_name() for a in actors)),'meshes':[],'lights':[],'fx':[]}
for a in actors:
 for c in a.get_components_by_class(unreal.StaticMeshComponent):
  mesh=c.get_editor_property('static_mesh')
  if not mesh:continue
  origin,extent=a.get_actor_bounds(False)
  row={'actor':a.get_actor_label(),'mesh':mesh.get_path_name(),'extent':[extent.x,extent.y,extent.z],'cast_shadow':c.get_editor_property('cast_shadow'),'visible':c.get_editor_property('visible')}
  try:row['nanite']=mesh.get_editor_property('nanite_settings').get_editor_property('enabled')
  except Exception:pass
  r['meshes'].append(row)
 for c in a.get_components_by_class(unreal.LightComponentBase):
  row={'actor':a.get_actor_label(),'class':c.get_class().get_name()}
  for k in ['cast_shadows','mobility','attenuation_radius','intensity','visible']: 
   try:row[k]=str(c.get_editor_property(k))
   except Exception:pass
  r['lights'].append(row)
 for typename in ['NiagaraComponent','ParticleSystemComponent']:
  cls=getattr(unreal,typename,None)
  if cls:
   for c in a.get_components_by_class(cls):r['fx'].append({'actor':a.get_actor_label(),'class':typename,'active':c.is_active()})
r['meshes'].sort(key=lambda x:max(x['extent']),reverse=True)
Path(unreal.Paths.project_saved_dir(),'RestartPerformanceAudit.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')

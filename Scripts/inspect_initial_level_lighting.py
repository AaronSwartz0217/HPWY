import unreal,json,runpy
from pathlib import Path
level='/Game/LanTuRebuild/Maps/初始房间/restartlife'
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(level)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={'map':level,'lights':[],'postprocess':[],'targets':[],'cameras':[]}
def val(x):
 if isinstance(x,(int,float,bool,str)):return x
 return str(x)
def props(o,names):
 d={}
 for n in names:
  try:d[n]=val(o.get_editor_property(n))
  except Exception:pass
 return d
ppnames=['override_auto_exposure_method','auto_exposure_method','override_auto_exposure_bias','auto_exposure_bias','override_auto_exposure_min_brightness','auto_exposure_min_brightness','override_auto_exposure_max_brightness','auto_exposure_max_brightness','override_bloom_intensity','bloom_intensity','override_indirect_lighting_intensity','indirect_lighting_intensity','override_indirect_lighting_color','indirect_lighting_color','override_color_gain','color_gain','override_white_temp','white_temp','override_film_white_point','film_white_point']
for a in sub.get_all_level_actors():
 info={'label':a.get_actor_label(),'name':a.get_name(),'class':a.get_class().get_name(),'position':str(a.get_actor_location())}
 for c in a.get_components_by_class(unreal.LightComponentBase):
  row=dict(info);row['component']=c.get_class().get_name();row['properties']=props(c,['intensity','intensity_units','light_color','use_temperature','temperature','indirect_lighting_intensity','volumetric_scattering_intensity','attenuation_radius','source_radius','source_length','mobility','affects_world','visible','specular_scale','cast_shadows','use_inverse_squared_falloff','light_falloff_exponent','real_time_capture','lower_hemisphere_is_black','lower_hemisphere_color']);report['lights'].append(row)
 if isinstance(a,unreal.PostProcessVolume):
  row=dict(info);row.update(props(a,['enabled','unbound','blend_weight','priority']));row['settings']=props(a.get_editor_property('settings'),ppnames);report['postprocess'].append(row)
 for c in a.get_components_by_class(unreal.CameraComponent):
  row=dict(info);row.update(props(c,['post_process_blend_weight']));row['settings']=props(c.get_editor_property('post_process_settings'),ppnames);report['cameras'].append(row)
 for c in a.get_components_by_class(unreal.MeshComponent):
  materials=[m.get_path_name() for m in c.get_materials() if m]
  if any('摊位布料_001' in m or '0草_001' in m for m in materials):
   row=dict(info);row['materials']=materials;row['bounds']=str(a.get_actor_bounds(False));report['targets'].append(row)
Path(unreal.Paths.project_saved_dir(),'InitialLevelLightingAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
runpy.run_path(str(Path(unreal.Paths.project_dir(),'Scripts/inspect_cloth_grass_materials.py')))
unreal.log('INITIAL_LEVEL_LIGHTING_AUDIT_DONE')

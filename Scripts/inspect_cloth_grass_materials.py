import unreal,json
from pathlib import Path
M=unreal.MaterialEditingLibrary
paths=['/Game/LanTuRebuild/Materials/WPQT/模型/杂物模型/摊位布料_001','/Game/LanTuRebuild/Materials/WPQT/模型/院子/0草_001']
report=[]
for path in paths:
 a=unreal.load_asset(path);entry={'path':path,'class':a.get_class().get_name() if a else None,'chain':[]};report.append(entry)
 while a:
  row={'path':a.get_path_name(),'class':a.get_class().get_name()};entry['chain'].append(row)
  for prop in ['shading_model','blend_mode','two_sided','use_material_attributes','scalar_parameter_values','vector_parameter_values','texture_parameter_values','base_property_overrides']:
   try:row[prop]=str(a.get_editor_property(prop))
   except Exception:pass
  if isinstance(a,unreal.Material):
   row['outputs']={};seen=set();nodes=[]
   def walk(n):
    if not n or n in seen:return
    seen.add(n);v={'name':n.get_name(),'type':n.get_class().get_name()}
    for p in ['parameter_name','default_value','constant','r','texture','material_function','code','a','b','const_a','const_b']:
     try:v[p]=str(n.get_editor_property(p))
     except Exception:pass
    inputs=M.get_inputs_for_material_expression(a,n);v['inputs']=[x.get_name() if x else None for x in inputs];nodes.append(v)
    for x in inputs:walk(x)
   for p in ['MP_BASE_COLOR','MP_EMISSIVE_COLOR','MP_SPECULAR','MP_ROUGHNESS','MP_METALLIC','MP_SUBSURFACE_COLOR','MP_NORMAL','MP_MATERIAL_ATTRIBUTES']:
    try:n=M.get_material_property_input_node(a,getattr(unreal.MaterialProperty,p));row['outputs'][p]=n.get_name() if n else None;walk(n)
    except Exception:pass
   row['nodes']=nodes;break
  try:a=a.get_editor_property('parent')
  except Exception:break
Path(unreal.Paths.project_saved_dir(),'ClothGrassMaterialAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CLOTH_GRASS_AUDIT_DONE')

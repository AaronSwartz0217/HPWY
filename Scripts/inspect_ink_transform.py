import unreal,json
from pathlib import Path
bp=unreal.load_asset('/Game/HPWH/Blueprints/BP_ProgressionCharacter')
mesh=unreal.get_default_object(bp.generated_class()).get_editor_property('mesh')
asset=mesh.get_editor_property('skeletal_mesh_asset')
report={p:str(mesh.get_editor_property(p)) for p in ['relative_location','relative_rotation','relative_scale3d']}
report['bounds']=str(asset.get_bounds())
report['assetMaterials']=[str(slot.get_editor_property('material_interface')) for slot in asset.get_editor_property('materials')]
report['componentMaterials']=[str(mesh.get_material(i)) for i in range(mesh.get_num_materials())]
Path(unreal.Paths.project_saved_dir(),'InkMeshTransform.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

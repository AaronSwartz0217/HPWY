"""Verify saved sketch/assets and actual mesh replacement in an editor world.
All changed geometry is in memory only. Does not start PIE or save changes.
"""
import copy
import json
import runpy
from pathlib import Path
import unreal

PROJECT=Path(unreal.Paths.project_dir()).resolve()
ROOT='/SpiritWFC/Settlement'
LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT],True)
assert LEVELS.load_level(ROOT+'/Maps/L_WFC_GridEditorExample')
gen=next(a for a in ACTORS.get_all_level_actors() if isinstance(a,unreal.WFCMapGenerator))
plan=unreal.load_asset(ROOT+'/Data/DA_WFC_PaintedExample')
assert isinstance(plan,unreal.WFCLayoutAsset)
cfg=copy.copy(gen.get_editor_property('settings'))
saved=copy.copy(gen.get_editor_property('displayed_result'))
assert len(cfg.painted_cells)==59 and len(plan.settings.painted_cells)==59
assert gen.get_editor_property('displayed_tile_set')==plan.tile_set
assert gen.get_generated_instance_count()==246
assert unreal.WFCBlueprintLibrary.validate_wfc(plan.tile_set,cfg,saved.cells) is not None
report={'scope':'editor only; no PIE/game, no test changes saved','painted_cells':59,'saved_cells':len(saved.cells),
    'saved_instances':246,'plan_and_level_reload':True,'seed_checks':0}
for seed in range(24):
    cfg.set_editor_property('seed',seed)
    result=unreal.WFCBlueprintLibrary.solve_wfc(plan.tile_set,cfg)
    assert result.status==unreal.WFCResultStatus.SUCCESS,(seed,result.message)
    assert unreal.WFCBlueprintLibrary.validate_wfc(plan.tile_set,cfg,result.cells) is not None
    report['seed_checks']+=1

# Complete-prefab replacement must reach the actual generated component,
# not just change UI text or the stored rule data.
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
updated=[]
for original in cfg.painted_cells:
    c=copy.copy(original)
    if c.coordinate.x==10 and c.coordinate.y==9:
        c.set_editor_property('building_mesh',cube)
        c.set_editor_property('building_transform',unreal.Transform(location=unreal.Vector(0,0,50)))
    updated.append(c)
cfg.set_editor_property('painted_cells',updated)
gen.set_editor_property('settings',cfg)
assert gen.generate_map(),str(gen.last_result)
found=False
for component in gen.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
    if component.get_editor_property('static_mesh')!=cube:continue
    for index in range(component.get_instance_count()):
        t=component.get_instance_transform(index,True)
        if t and abs(t.translation.x-6000)<.1 and abs(t.translation.y-5400)<.1 and abs(t.translation.z-50)<.1:found=True
assert found,'Requested prefab was not instantiated at the painted cell: '+str([
    (str(c.get_editor_property('static_mesh')),c.get_instance_count()) for c in gen.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)])
report['actual_prefab_replacement']=True
before=gen.get_generated_instance_count()
updated=[]
for original in cfg.painted_cells:
    c=copy.copy(original)
    if c.coordinate.x==10 and c.coordinate.y==9:c.set_editor_property('stories',2)
    updated.append(c)
cfg.set_editor_property('painted_cells',updated)
gen.set_editor_property('settings',cfg)
assert not gen.generate_map()
assert gen.get_generated_instance_count()==before,'Invalid complete-prefab settings destroyed the visible map'
report['invalid_replacement_keeps_previous']=True

# Reuse the saved plan and test strict/manual mode.
cfg=copy.copy(plan.settings);cfg.set_editor_property('fill_unpainted_buildings',False)
gen.set_editor_property('settings',cfg);gen.set_editor_property('tile_set',plan.tile_set)
assert gen.generate_map()
tiles=plan.tile_set.tiles
base_count=sum(1 for c in gen.displayed_result.cells if c.coordinate.z==0
    and any(str(tag) in ('House','Base') for tag in tiles[c.tile_index].tags))
assert base_count==4,base_count
report['strict_mode_buildings']=base_count

# Confirm no compatibility regressions in the original and automatic examples.
runpy.run_path(str(PROJECT/'Scripts/WFC/verify_settlement_samples.py'),run_name='__main__')
runpy.run_path(str(PROJECT/'Scripts/WFC/verify_wfc_samples.py'),run_name='__main__')
report['automatic_seed_cases']=80
report['legacy_seed_cases']=96
(PROJECT/'Saved/WFC/GridEditorVerification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GRID_EDITOR_VERIFIED '+json.dumps(report))

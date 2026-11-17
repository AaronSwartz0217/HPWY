"""Editor-only checks for actual improved WFC assets; never starts/saves gameplay."""
import collections
import copy
import json
from pathlib import Path
import unreal

ROOT='/SpiritWFC/Settlement'
PROJECT=Path(unreal.Paths.project_dir()).resolve()
LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT],True)
REPORT={'scope':'editor world/data only; no PIE/game','maps':[]}


def require(value,message):
    if not value:raise AssertionError(message)


for suffix,layers in (('Planar',1),('ThreeLayers',3)):
    path=ROOT+'/Maps/L_WFC_Settlement_'+suffix
    require(LEVELS.load_level(path),path)
    actors=[a for a in ACTORS.get_all_level_actors() if isinstance(a,unreal.WFCMapGenerator)]
    require(len(actors)==1,'Expected one generator')
    a=actors[0];data=a.get_editor_property('tile_set');s=copy.copy(a.get_editor_property('settings'))
    saved=copy.copy(a.get_editor_property('displayed_result'));tiles=data.get_editor_property('tiles')
    visible=sum(tiles[c.tile_index].render_mesh for c in saved.cells)
    require(len(saved.cells)==14*14*layers,'Incomplete saved layout')
    require(a.get_generated_instance_count()==visible,'Saved visible components missing')
    require(layers==1 or visible<len(saved.cells),'Upper air incorrectly rendered as full floor slabs')
    checked=unreal.WFCBlueprintLibrary.validate_wfc(data,s,saved.cells)
    require(checked is not None and checked[1]==1,'Invalid saved layout')
    stats=[]
    for seed in range(40):
        s.set_editor_property('seed',seed)
        result=unreal.WFCBlueprintLibrary.solve_wfc(data,s)
        require(result.status==unreal.WFCResultStatus.SUCCESS,f'{suffix} seed {seed}: {result.message}')
        require(result.walkable_components==1,'Disconnected walkable layout')
        counts=collections.Counter(str(tiles[c.tile_index].tile_id).split('_')[0] for c in result.cells)
        require(counts['House']+counts['Base']>0,'No complete buildings')
        require(counts['Plaza']==9,'Central square is not preserved')
        require(layers==1 or counts['Air']>0,'No vertical open space')
        again=unreal.WFCBlueprintLibrary.solve_wfc(data,s)
        require([(c.tile_id,c.quarter_turns) for c in result.cells]==[(c.tile_id,c.quarter_turns) for c in again.cells],'Seed differs')
        stats.append({'seed':seed,'visible':sum(tiles[c.tile_index].render_mesh for c in result.cells),
            'buildings':counts['House']+counts['Base'],'air':counts['Air'],'ms':result.elapsed_milliseconds})
    # Regenerating must replace components, including when density is changed.
    s.set_editor_property('building_density',0.0);a.set_editor_property('settings',s)
    require(a.generate_map(),'Zero-density preview failed')
    require(a.get_generated_instance_count()==14*14,'Zero-density should only render ground layer')
    s.set_editor_property('building_density',0.82);a.set_editor_property('settings',s)
    require(a.generate_map(),'Density restore failed')
    active=a.get_editor_property('displayed_result')
    require(a.get_generated_instance_count()==sum(tiles[c.tile_index].render_mesh for c in active.cells),'Regeneration leaked instances')
    REPORT['maps'].append({'map':path,'saved_cells':len(saved.cells),'saved_visible':visible,
        'walkable_components':saved.walkable_components,'seed_cases':stats,'density_change':True,
        'tile_counts':dict(collections.Counter(str(c.tile_id) for c in saved.cells))})
REPORT['seed_cases']=sum(len(m['seed_cases']) for m in REPORT['maps'])
(PROJECT/'Saved/WFC/SettlementVerification.json').write_text(json.dumps(REPORT,indent=2),encoding='utf-8')
unreal.log('SETTLEMENT_VERIFIED '+json.dumps({k:v for k,v in REPORT.items() if k!='maps'}))

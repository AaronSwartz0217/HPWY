from pathlib import Path
import copy,json,unreal
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={'maps':[],'seedCases':[],'scope':'read-only editor verification; no game saves'}
maps=['/SpiritWFC/Settlement/Maps/L_WFC_Settlement_Planar','/SpiritWFC/Settlement/Maps/L_WFC_Settlement_ThreeLayers',
      '/SpiritWFC/Settlement/Maps/L_WFC_CompoundHouses','/SpiritWFC/Settlement/Maps/L_WFC_GridEditorExample','/Game/LanTuRebuild/Maps/万象天宫/万象天宫']
for map_path in maps:
    assert levels.load_level(map_path),map_path
    generators=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.WFCMapGenerator)]
    assert len(generators)==1
    a=generators[0];stairs=0
    for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
        mesh=c.get_editor_property('static_mesh')
        if mesh and any(role in mesh.get_name() for role in ('_Base_','_Middle_','_CompoundBase_')):
            slope=c.get_editor_property('body_instance').get_editor_property('walkable_slope_override')
            assert abs(slope.get_editor_property('walkable_slope_angle')-55)<.01,(map_path,mesh.get_name())
            stairs+=1
    assert a.get_generated_instance_count()>0
    cells=a.get_editor_property('displayed_result').get_editor_property('cells')
    tags={str(c.get_editor_property('tile_id')).split('_')[0] for c in cells}
    if 'CompoundHouses' in map_path:assert 'CompoundBase' in tags and 'CompoundWing' in tags
    report['maps'].append({'map':map_path,'instances':a.get_generated_instance_count(),'stairComponentsWithSavedOverride':stairs,'tileRoles':sorted(tags)})
data=unreal.load_asset('/SpiritWFC/Settlement/Data/DA_WFC_Settlement')
tiles=data.get_editor_property('tiles')
for planar in (True,False):
    s=unreal.WFCGenerationSettings()
    for k,v in {'mode':unreal.WFCMapMode.PLANAR if planar else unreal.WFCMapMode.VOLUME,'layout_preset':unreal.WFCLayoutPreset.SETTLEMENT,
                'width':14,'depth':14,'layers':3,'max_building_stories':2,'compound_building_chance':1.,'building_density':1.,'constrain_horizontal_boundary':False}.items():s.set_editor_property(k,v)
    for seed in range(8):
        s.set_editor_property('seed',seed);r=unreal.WFCBlueprintLibrary.solve_wfc(data,s)
        assert r.status==unreal.WFCResultStatus.SUCCESS,str(r.message)
        roles=[str(tiles[c.tile_index].tile_id).split('_')[0] for c in r.cells]
        assert 'CompoundWing' in roles and ('CompoundHouse' if planar else 'CompoundBase') in roles
        assert r.walkable_components==1
        assert all(str(tiles[c.tile_index].tile_id)=='Air' for c in r.cells if c.coordinate.z==2)
        report['seedCases'].append({'mode':'planar' if planar else 'volume','seed':seed,'main':roles.count('CompoundHouse')+roles.count('CompoundBase'),'wings':roles.count('CompoundWing'),'ms':r.elapsed_milliseconds})
report['passed']=True
(project/'Saved/WFC/BuildingPlayabilityVerification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('BUILDING_PLAYABILITY_VERIFIED_PASS')

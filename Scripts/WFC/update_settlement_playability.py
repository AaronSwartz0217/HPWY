"""Update owned meshes/data and sample generators; preserve hand-painted gameplay layouts."""
from pathlib import Path
import copy,json,shutil,unreal,re
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source=project/'SourceArt/ProceduralGeneration/WFC/SettlementKit'
backup=project/'Saved/Backups/PCGAndBlackUI_20261028/Assets'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
roots=[('/SpiritWFC/Settlement','M_Settlement_'),('/Game/LanTuRebuild/Maps/万象天宫','M_Heaven_')]
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
def asset_file(package):
    if package.startswith('/SpiritWFC/'):return Path('Plugins/SpiritWFC/Content')/(package.removeprefix('/SpiritWFC/')+'.uasset')
    return Path('Content')/(package.removeprefix('/Game/')+'.uasset')
def preserve(package):
    relative=asset_file(package);src=project/relative;dst=backup/relative
    if src.is_file() and not dst.exists():dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
def save(asset):assert lib.save_loaded_asset(asset,False),asset.get_path_name()
report={'meshes':[],'tileSets':[],'sampleMaps':[]}
def compound_tile(root,role,variant):
    t=unreal.WFCTileDefinition();s=unreal.WFCSockets()
    for direction in ('positive_x','negative_x','positive_y','negative_y'):s.set_editor_property(direction,'Ground')
    s.set_editor_property('negative_x' if role=='CompoundWing' else 'positive_x','CompoundDoor')
    s.set_editor_property('up','Stair' if role=='CompoundBase' else 'Sky');s.set_editor_property('down','Ground')
    for key,value in {'tile_id':role+'_'+str(variant),'tags':{role},'sockets':s,'mesh':unreal.load_asset(root+'/Meshes/SM_Settlement_'+role+'_'+str(variant)),
                      'allowed_yaw_quarter_turns':[0,1,2,3],'max_layer':0}.items():t.set_editor_property(key,value)
    return t
for root,prefix in roots:
    if '-WFCPluginOnly' in unreal.SystemLibrary.get_command_line() and not root.startswith('/SpiritWFC'):continue
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([root],True)
    palette=[unreal.load_asset(root+'/Materials/'+prefix+str(i)) for i in range(6)]
    assert all(palette)
    for role in ('House','Base','Middle','Top','CompoundHouse','CompoundBase','CompoundWing'):
        if '-WFCImportStairsOnly' in unreal.SystemLibrary.get_command_line() and role not in ('Base','Middle','CompoundBase'):continue
        if '-WFCImportChangedOnly' in unreal.SystemLibrary.get_command_line() and role not in ('Base','Middle','CompoundBase','CompoundWing'):continue
        for variant in (0,1):
            name='SM_Settlement_'+role+'_'+str(variant);package=root+'/Meshes/'+name;preserve(package)
            only_mesh=re.search(r'-WFCOnlyMesh=([^\s]+)',unreal.SystemLibrary.get_command_line())
            if only_mesh and name!=only_mesh.group(1):continue
            ui=unreal.FbxImportUI()
            for key,value in {'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,
                              'import_mesh':True,'import_as_skeletal':False,'import_materials':False,'import_textures':False,'import_animations':False}.items():ui.set_editor_property(key,value)
            data=ui.get_editor_property('static_mesh_import_data')
            for key,value in {'combine_meshes':True,'auto_generate_collision':False,'convert_scene':True,'convert_scene_unit':True}.items():data.set_editor_property(key,value)
            task=unreal.AssetImportTask();task.filename=str(source/(name+'.fbx'));task.destination_path=root+'/Meshes';task.destination_name=name
            task.automated=True;task.options=ui;task.factory=unreal.FbxFactory();task.replace_existing=True;task.save=False
            tools.import_asset_tasks([task]);mesh=unreal.load_asset(package);assert isinstance(mesh,unreal.StaticMesh),package
            slots=[]
            for old in mesh.get_editor_property('static_materials'):
                slot=copy.copy(old);index=int(str(slot.get_editor_property('imported_material_slot_name')).rsplit('_',1)[-1])
                slot.set_editor_property('material_interface',palette[index]);slots.append(slot)
            mesh.set_editor_property('static_materials',slots)
            body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            body.set_editor_property('double_sided_geometry',True)
            if role in ('Base','Middle','CompoundBase'):
                slope=unreal.WalkableSlopeOverride()
                slope.set_editor_property('walkable_slope_behavior',unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_INCREASE)
                slope.set_editor_property('walkable_slope_angle',55.)
                body.set_editor_property('walkable_slope_override',slope)
            save(mesh);report['meshes'].append(package)
    package=root+('/Data/DA_WFC_Settlement' if root.startswith('/SpiritWFC') else '/Data/DA_HeavenTiles')
    preserve(package);data=unreal.load_asset(package);assert data
    tiles=[t for t in data.get_editor_property('tiles') if not str(t.get_editor_property('tile_id')).startswith('Compound')]
    tiles.extend(compound_tile(root,role,variant) for role in ('CompoundHouse','CompoundBase','CompoundWing') for variant in (0,1))
    data.set_editor_property('tiles',tiles);data.set_editor_property('traversable_sockets',{'Ground','Road','Stair','CompoundDoor'});save(data)
    report['tileSets'].append(package)
    if root.startswith('/SpiritWFC'):
        for suffix in ('Planar','ThreeLayers'):
            bp_path=root+'/Blueprints/BP_WFC_Settlement_'+suffix;preserve(bp_path);bp=unreal.load_asset(bp_path)
            cdo=unreal.get_default_object(bp.generated_class());settings=copy.copy(cdo.get_editor_property('settings'))
            settings.set_editor_property('max_building_stories',2);settings.set_editor_property('compound_building_chance',.3)
            cdo.set_editor_property('settings',settings);unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
            map_path=root+'/Maps/L_WFC_Settlement_'+suffix
            # Map is saved only to refresh the existing generator's own instances, with other actors untouched.
            original_map=project/'Plugins/SpiritWFC/Content/Settlement/Maps'/('L_WFC_Settlement_'+suffix+'.umap')
            map_backup=backup/'Plugins/SpiritWFC/Content/Settlement/Maps'/original_map.name
            if not map_backup.exists():map_backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(original_map,map_backup)
            assert levels.load_level(map_path)
            generators=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.WFCMapGenerator)]
            assert len(generators)==1
            generator=generators[0];current=copy.copy(generator.get_editor_property('settings'))
            current.set_editor_property('max_building_stories',2);current.set_editor_property('compound_building_chance',.3)
            generator.set_editor_property('settings',current);assert generator.generate_map(),str(generator.get_editor_property('last_result'))
            assert levels.save_current_level();report['sampleMaps'].append(map_path)

# A distinct example provides a clear main-house/wing preview without replacing hand-painted Heaven.
example='/SpiritWFC/Settlement/Maps/L_WFC_CompoundHouses'
if not lib.does_asset_exist(example):
    assert lib.duplicate_asset('/SpiritWFC/Settlement/Maps/L_WFC_Settlement_ThreeLayers',example)
    assert levels.load_level(example)
    generator=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.WFCMapGenerator))
    settings=copy.copy(generator.get_editor_property('settings'));settings.set_editor_property('compound_building_chance',1.0)
    generator.set_editor_property('settings',settings);assert generator.generate_map();assert levels.save_current_level()
    report['sampleMaps'].append(example)
# Persist slope inheritance on previously serialized HISMs too, without regenerating Heaven's artist layout.
for map_path in ['/SpiritWFC/Settlement/Maps/L_WFC_CompoundHouses','/SpiritWFC/Settlement/Maps/L_WFC_GridEditorExample','/Game/LanTuRebuild/Maps/万象天宫/万象天宫']:
    assert levels.load_level(map_path),map_path
    map_file=(project/'Content'/ (map_path.removeprefix('/Game/')+'.umap')) if map_path.startswith('/Game/') else (project/'Plugins/SpiritWFC/Content'/(map_path.removeprefix('/SpiritWFC/')+'.umap'))
    relative=map_file.relative_to(project);destination=backup/relative
    if map_file.is_file() and not destination.exists():destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(map_file,destination)
    for actor in actors.get_all_level_actors():
        if not isinstance(actor,unreal.WFCMapGenerator):continue
        for component in actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            mesh=component.get_editor_property('static_mesh')
            if mesh:component.set_walkable_slope_override(mesh.get_editor_property('body_setup').get_editor_property('walkable_slope_override'))
    assert levels.save_current_level()
out=project/'Saved/WFC/BuildingPlayabilityUpgrade.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('BUILDING_PLAYABILITY_ASSETS_PASS '+str(len(report['meshes'])))

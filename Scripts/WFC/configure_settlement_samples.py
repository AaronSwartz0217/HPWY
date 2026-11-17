"""Author improved WFC examples in a new owned folder; editor only, no PIE.

Default preserves existing assets/maps. -WFCRebuildSettlement explicitly replaces
owned v2 sample actors; actors without our tag abort replacement.
"""
import json
import copy
from pathlib import Path
import unreal

PROJECT=Path(unreal.Paths.project_dir()).resolve()
SOURCE=PROJECT/'SourceArt/ProceduralGeneration/WFC/SettlementKit'
ROOT='/SpiritWFC/Settlement'
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
ACTORS=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT],True)
REBUILD='-WFCRebuildSettlement' in unreal.SystemLibrary.get_command_line()
REIMPORT='-WFCReimportKit' in unreal.SystemLibrary.get_command_line()
MANIFEST=json.loads((SOURCE/'kit_manifest.json').read_text())
REPORT={'editor_only':True,'maps':[],'axis_probe':None}


def save(obj):
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj,False)


def get_asset(name,folder,cls,factory):
    return unreal.load_asset(f'{ROOT}/{folder}/{name}') or TOOLS.create_asset(name,f'{ROOT}/{folder}',cls,factory)


palette=[]
for i,rgba in enumerate(MANIFEST['palette']):
    mat=get_asset('M_Settlement_'+str(i),'Materials',unreal.Material,unreal.MaterialFactoryNew())
    if not unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR):
        node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property('constant',unreal.LinearColor(*rgba))
        unreal.MaterialEditingLibrary.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant)
        rough.set_editor_property('r',0.86)
        unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        mat.set_editor_property('two_sided',True)
    unreal.MaterialEditingLibrary.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(mat);save(mat);palette.append(mat)

unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
models={}
for row in MANIFEST['models']:
    name=row['mesh'];mesh=unreal.load_asset(f'{ROOT}/Meshes/{name}')
    if not mesh or REIMPORT:
        ui=unreal.FbxImportUI()
        for key,value in {'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,
            'import_mesh':True,'import_as_skeletal':False,'import_materials':False,'import_textures':False,'import_animations':False}.items():
            ui.set_editor_property(key,value)
        data=ui.get_editor_property('static_mesh_import_data')
        for key,value in {'combine_meshes':True,'auto_generate_collision':False,'convert_scene':True,'convert_scene_unit':True}.items():
            data.set_editor_property(key,value)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task=unreal.AssetImportTask();task.filename=str(SOURCE/(name+'.fbx'));task.destination_path=ROOT+'/Meshes'
        task.destination_name=name;task.automated=True;task.options=ui;task.factory=unreal.FbxFactory();task.save=True;task.replace_existing=REIMPORT
        TOOLS.import_asset_tasks([task]);mesh=unreal.load_asset(f'{ROOT}/Meshes/{name}')
    assert mesh,name
    slots=[]
    for old_slot in mesh.get_editor_property('static_materials'):
        # Slot names survive FBX import; do not rely on importer reordering.
        slot_name=str(old_slot.get_editor_property('imported_material_slot_name'))
        index=int(slot_name.rsplit('_',1)[-1])
        slot=copy.copy(old_slot)
        slot.set_editor_property('material_interface',palette[index])
        slots.append(slot)
    mesh.set_editor_property('static_materials',slots)
    body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    body.set_editor_property('double_sided_geometry',True);save(mesh);models[name]=mesh

box=models['SM_Settlement_AxisProbe'].get_bounding_box()
probe=[box.min.x,box.min.y,box.min.z,box.max.x,box.max.y,box.max.z]
REPORT['axis_probe']=probe
# FBX UE import mirrors Blender Y. Compensate at the geometry preparation stage
# if needed, so tile rotations/road ports remain in native UE directions.
expected=(100,200,300,120,220,320)
assert all(abs(a-b)<0.2 for a,b in zip(probe,expected)), 'Axis conversion must be corrected before creating rules: '+str(probe)


def tile(name,tag,mesh=None,ports=None,up='Sky',down='Ground',walkable=True,turns=(0,),min_layer=0,max_layer=2):
    t=unreal.WFCTileDefinition();s=unreal.WFCSockets()
    for key,value in zip(('positive_x','negative_x','positive_y','negative_y'),ports or ('Ground',)*4):s.set_editor_property(key,value)
    s.set_editor_property('up',up);s.set_editor_property('down',down)
    for key,value in {'tile_id':name,'tags':{tag},'sockets':s,'weight':1.,'walkable':walkable,
        'allowed_yaw_quarter_turns':list(turns),'min_layer':min_layer,'max_layer':max_layer,
        'render_mesh':mesh is not None}.items():t.set_editor_property(key,value)
    if mesh:t.set_editor_property('mesh',models[mesh])
    return t


definitions=[tile('Ground','Ground','SM_Settlement_Ground',max_layer=0),
    tile('Air','Air',ports=('Closed',)*4,up='Sky',down='Sky',walkable=False)]
for mask in range(16):
    ports=tuple('Road' if mask&(1<<d) else 'Ground' for d in range(4))
    for kind in ('Road','Plaza'):
        key=f'{kind}_{mask:02d}'
        definitions.append(tile(key,key,'SM_Settlement_'+('Plaza' if kind=='Plaza' else key),ports=ports,max_layer=0))
for role in ('House','Base','Middle','Top'):
    for variant in (0,1):
        definitions.append(tile(role+'_'+str(variant),role,'SM_Settlement_'+role+'_'+str(variant),
            ports=('Ground',)*4 if role in ('House','Base') else ('Closed',)*4,
            up='Stair' if role in ('Base','Middle') else 'Sky',down='Stair' if role in ('Middle','Top') else 'Ground',
            turns=(0,1,2,3),min_layer=1 if role in ('Middle','Top') else 0,max_layer=2 if role in ('Middle','Top') else 0))
for role in ('CompoundHouse','CompoundBase','CompoundWing'):
    for variant in (0,1):
        ports=list(('Ground',)*4);ports[1 if role=='CompoundWing' else 0]='CompoundDoor'
        definitions.append(tile(role+'_'+str(variant),role,'SM_Settlement_'+role+'_'+str(variant),ports=ports,
            up='Stair' if role=='CompoundBase' else 'Sky',turns=(0,1,2,3),max_layer=0))
factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.WFCTileSet)
dataset=get_asset('DA_WFC_Settlement','Data',unreal.WFCTileSet,factory)
if REBUILD or not dataset.get_editor_property('tiles'):
    dataset.set_editor_property('tiles',definitions);dataset.set_editor_property('cell_size',unreal.Vector(600,600,360))
    dataset.set_editor_property('traversable_sockets',{'Ground','Road','Stair','CompoundDoor'});save(dataset)


def config(volume):
    s=unreal.WFCGenerationSettings()
    for key,value in {'mode':unreal.WFCMapMode.VOLUME if volume else unreal.WFCMapMode.PLANAR,
        'layout_preset':unreal.WFCLayoutPreset.SETTLEMENT,'width':14,'depth':14,'layers':3,
        'seed':20261026,'building_density':0.82,'max_building_stories':2,'compound_building_chance':0.3,'constrain_horizontal_boundary':False,'pinned_cells':[]}.items():
        s.set_editor_property(key,value)
    return s


def owned(actor):
    actor.set_editor_property('tags',['SpiritWFC.Settlement.v2']);return actor


for volume in (False,True):
    suffix='ThreeLayers' if volume else 'Planar'
    bp_name='BP_WFC_Settlement_'+suffix
    bp=unreal.load_asset(f'{ROOT}/Blueprints/{bp_name}')
    if not bp:
        f=unreal.BlueprintFactory();f.set_editor_property('parent_class',unreal.WFCMapGenerator)
        bp=get_asset(bp_name,'Blueprints',unreal.Blueprint,f)
    cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('tile_set',dataset);cdo.set_editor_property('settings',config(volume))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    path=f'{ROOT}/Maps/L_WFC_Settlement_{suffix}'
    exists=unreal.EditorAssetLibrary.does_asset_exist(path)
    if exists and not REBUILD:continue
    if exists:
        assert LEVELS.load_level(path)
        items=[]
        for a in ACTORS.get_all_level_actors():
            if isinstance(a,(unreal.WorldSettings,unreal.Brush)):continue
            assert 'SpiritWFC.Settlement.v2' in [str(t) for t in a.get_editor_property('tags')],a.get_actor_label()
            items.append(a)
        for a in items:assert ACTORS.destroy_actor(a)
    else:assert LEVELS.new_level(path)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase)
    sun=owned(ACTORS.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2500),unreal.Rotator(-40,-25,0)))
    light=sun.get_component_by_class(unreal.DirectionalLightComponent);light.set_mobility(unreal.ComponentMobility.MOVABLE);light.set_editor_property('intensity',6)
    sky=owned(ACTORS.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,2000)))
    skycomp=sky.get_component_by_class(unreal.SkyLightComponent);skycomp.set_mobility(unreal.ComponentMobility.MOVABLE)
    skycomp.set_editor_property('real_time_capture',True)
    owned(ACTORS.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0)))
    generator=owned(ACTORS.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,0)))
    generator.set_actor_label('WFC Settlement - Generate Map')
    assert generator.generate_map(),str(generator.get_editor_property('last_result'))
    result=generator.get_editor_property('displayed_result')
    plaza=[c for c in result.cells if str(c.tile_id).startswith('Plaza_')]
    px=sum(c.coordinate.x for c in plaza)/len(plaza)*600
    py=sum(c.coordinate.y for c in plaza)/len(plaza)*600
    start=owned(ACTORS.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(px,py,120)))
    start.set_actor_label('WFC Plaza Spawn')
    assert LEVELS.save_current_level()
    cells=[]
    for cell in result.cells:
        definition=dataset.get_editor_property('tiles')[cell.tile_index]
        reference=definition.get_editor_property('mesh')
        mesh_name=(reference.get_name() if hasattr(reference,'get_name') else reference.get_asset_name()) if definition.render_mesh else None
        cells.append({'xyz':[cell.coordinate.x,cell.coordinate.y,cell.coordinate.z],'tile':str(cell.tile_id),
            'yaw':cell.quarter_turns,'render':definition.render_mesh,
            'mesh':mesh_name})
    REPORT['maps'].append({'map':path,'cells':len(cells),'visible_instances':generator.get_generated_instance_count(),
        'components':result.walkable_components,'seed':result.effective_seed,'layout':cells})
output=PROJECT/'Saved/WFC/SettlementCreation.json';output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(REPORT,indent=2),encoding='utf-8')
unreal.log('SETTLEMENT_CREATED '+str([(r['map'],r['visible_instances']) for r in REPORT['maps']]))

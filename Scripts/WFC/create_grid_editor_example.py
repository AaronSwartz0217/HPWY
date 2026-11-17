"""Create an independent painted-plan example and reusable layout asset.

No PIE/game. Existing example is preserved unless -WFCRebuildGridExample is set.
Only /SpiritWFC/Settlement owned demo packages are authored.
"""
import json
from pathlib import Path
import unreal

ROOT='/SpiritWFC/Settlement'
PROJECT=Path(unreal.Paths.project_dir()).resolve()
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT],True)
data=unreal.load_asset(ROOT+'/Data/DA_WFC_Settlement')
assert data,'Create the settlement kit first'
settings=unreal.WFCGenerationSettings()
for key,value in {'width':14,'depth':14,'mode':unreal.WFCMapMode.VOLUME,'layers':3,
    'layout_preset':unreal.WFCLayoutPreset.PAINTED,'seed':20261027,'building_density':.55,
    'constrain_horizontal_boundary':False,'fill_unpainted_buildings':True}.items():
    settings.set_editor_property(key,value)
paint={}


def cell(x,y,kind,stories=0,model=None):
    c=unreal.WFCPaintedCell()
    for key,value in {'coordinate':unreal.IntPoint(x,y),'kind':kind,'stories':stories}.items():c.set_editor_property(key,value)
    if model:c.set_editor_property('building_mesh',model)
    paint[(x,y)]=c


for start,end in [((1,2),(11,2)),((11,2),(11,11)),((11,11),(7,11)),((7,11),(7,7)),((7,7),(2,7)),((2,7),(2,11)),((7,7),(11,7))]:
    for p in unreal.WFCBlueprintLibrary.rasterize_road_stroke(unreal.IntPoint(*start),unreal.IntPoint(*end),14,14):
        cell(p.x,p.y,unreal.WFCPaintKind.ROAD)
for x in range(8,11):
    for y in range(3,6):cell(x,y,unreal.WFCPaintKind.PLAZA)
cell(3,3,unreal.WFCPaintKind.BUILDING,2)
cell(6,3,unreal.WFCPaintKind.BUILDING,3)
cell(3,8,unreal.WFCPaintKind.BUILDING,1)
cell(10,9,unreal.WFCPaintKind.BUILDING,1,unreal.load_asset(ROOT+'/Meshes/SM_Settlement_House_1'))
for x in range(3,6):
    for y in range(4,6):cell(x,y,unreal.WFCPaintKind.GROUND)
settings.set_editor_property('painted_cells',[paint[key] for key in sorted(paint,key=lambda p:(p[1],p[0]))])
plan_path=ROOT+'/Data/DA_WFC_PaintedExample'
plan=unreal.load_asset(plan_path)
rebuild='-WFCRebuildGridExample' in unreal.SystemLibrary.get_command_line()
if not plan:
    f=unreal.DataAssetFactory();f.set_editor_property('data_asset_class',unreal.WFCLayoutAsset)
    plan=TOOLS.create_asset('DA_WFC_PaintedExample',ROOT+'/Data',unreal.WFCLayoutAsset,f)
    update_plan=True
else:update_plan=rebuild
if update_plan:
    plan.set_editor_property('settings',settings);plan.set_editor_property('tile_set',data)
    plan.set_editor_property('palette',{unreal.WFCPaintKind.ROAD:unreal.LinearColor(.85,.85,.8,1),
        unreal.WFCPaintKind.BUILDING:unreal.LinearColor(.18,.48,.25,1),
        unreal.WFCPaintKind.PLAZA:unreal.LinearColor(.8,.62,.12,1),
        unreal.WFCPaintKind.GROUND:unreal.LinearColor(.62,.18,.17,1)})
    assert unreal.EditorAssetLibrary.save_loaded_asset(plan,False)
path=ROOT+'/Maps/L_WFC_GridEditorExample'
exists=unreal.EditorAssetLibrary.does_asset_exist(path)
if exists and not rebuild:
    unreal.log('GRID_EXAMPLE_PRESERVED '+path)
else:
    if exists:
        assert LEVELS.load_level(path)
        owned=[]
        for a in ACTORS.get_all_level_actors():
            if isinstance(a,(unreal.WorldSettings,unreal.Brush)):continue
            assert 'SpiritWFC.GridEditor.v3' in [str(t) for t in a.get_editor_property('tags')],a.get_actor_label()
            owned.append(a)
        for a in owned:assert ACTORS.destroy_actor(a)
    else:assert LEVELS.new_level(path)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase)
    def own(a):a.set_editor_property('tags',['SpiritWFC.GridEditor.v3']);return a
    sun=own(ACTORS.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(-40,-25,0)))
    light=sun.get_component_by_class(unreal.DirectionalLightComponent);light.set_mobility(unreal.ComponentMobility.MOVABLE);light.set_editor_property('intensity',6)
    own(ACTORS.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector()))
    sky=own(ACTORS.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,2000)))
    comp=sky.get_component_by_class(unreal.SkyLightComponent);comp.set_mobility(unreal.ComponentMobility.MOVABLE);comp.set_editor_property('real_time_capture',True)
    gen=own(ACTORS.spawn_actor_from_class(unreal.WFCMapGenerator,unreal.Vector()))
    gen.set_actor_label('WFC Grid Editor Example - Select me')
    gen.set_editor_property('tile_set',data);gen.set_editor_property('settings',settings)
    assert gen.generate_map(),str(gen.get_editor_property('last_result'))
    own(ACTORS.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(5400,2400,120)))
    assert LEVELS.save_current_level()
    result=gen.get_editor_property('displayed_result')
    visual=[];tiles=data.get_editor_property('tiles')
    for c in result.cells:
        t=tiles[c.tile_index];key=(c.coordinate.x,c.coordinate.y);custom=paint.get(key) if c.coordinate.z==0 else None
        ref=custom.get_editor_property('building_mesh') if custom else None
        if not ref:ref=t.get_editor_property('mesh')
        mesh_name=(ref.get_name() if hasattr(ref,'get_name') else ref.get_asset_name()) if t.render_mesh else None
        visual.append({'xyz':[c.coordinate.x,c.coordinate.y,c.coordinate.z],'tile':str(c.tile_id),'yaw':c.quarter_turns,
            'render':t.render_mesh,'mesh':mesh_name})
    report={'editor_only':True,'maps':[{'map':path,'preview_name':'GridEditor_Painted','cells':len(result.cells),
        'visible_instances':gen.get_generated_instance_count(),'layout':visual}],
        'painted':[[x,y,str(c.kind),c.stories] for (x,y),c in paint.items()]}
    (PROJECT/'Saved/WFC/GridEditorCreation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('GRID_EXAMPLE_CREATED '+str((path,gen.get_generated_instance_count(),len(paint))))

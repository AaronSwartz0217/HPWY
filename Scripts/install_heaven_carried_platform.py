"""Install the owned physical elevator beside Heaven's plaza spawn; keep authored layout."""
from pathlib import Path
import shutil,json,unreal
p=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
map_path='/Game/LanTuRebuild/Maps/万象天宫/万象天宫'
map_file=p/'Content/LanTuRebuild/Maps/万象天宫/万象天宫.umap'
backup=p/'Saved/Backups/FirstPersonAndLevitation_20261028/Content/LanTuRebuild/Maps/万象天宫/万象天宫.umap'
if not backup.exists():backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(map_file,backup)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(map_path)
existing=actors.get_all_level_actors();before={a.get_name() for a in existing}
generator=next(a for a in existing if isinstance(a,unreal.WFCMapGenerator))
original_cells=[(str(c.tile_id),c.quarter_turns,c.coordinate.x,c.coordinate.y,c.coordinate.z) for c in generator.get_editor_property('displayed_result').cells]
root='/Game/LanTuRebuild/Maps/万象天宫/浮空平台';lib=unreal.EditorAssetLibrary;lib.make_directory(root)
tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
def material(name,color,metallic,roughness,glow=0):
    m=unreal.load_asset(root+'/'+name)
    if not m:
        m=tools.create_asset(name,root,unreal.Material,unreal.MaterialFactoryNew())
        rgb=mel.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-400,0);rgb.set_editor_property('constant',unreal.LinearColor(*color,1))
        mel.connect_material_property(rgb,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for y,value,prop in [(100,metallic,unreal.MaterialProperty.MP_METALLIC),(200,roughness,unreal.MaterialProperty.MP_ROUGHNESS)]:
            n=mel.create_material_expression(m,unreal.MaterialExpressionConstant,-200,y);n.set_editor_property('r',value);mel.connect_material_property(n,'',prop)
        if glow:
            n=mel.create_material_expression(m,unreal.MaterialExpressionMultiply,-160,-150);n.set_editor_property('const_b',glow);mel.connect_material_expressions(rgb,'',n,'A');mel.connect_material_property(n,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mel.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);mel.recompile_material(m);lib.save_loaded_asset(m,False)
    return m
stone=material('M_FloatStone',(.105,.115,.125),.2,.8)
slab=material('M_FloatSlabs',(.21,.23,.25),.25,.65)
gold=material('M_FloatGold',(.6,.43,.16),.65,.35,.2)
owned=[a for a in existing if isinstance(a,unreal.CC_LevitationPlatform)]
assert len(owned)<=1
if owned:
    platform=owned[0];assert 'SpiritTales.CarriedReassemblyPlatform.v1' in [str(t) for t in platform.tags]
else:
    result=generator.get_editor_property('displayed_result');size=generator.get_editor_property('tile_set').cell_size
    plaza=[c for c in result.cells if str(c.tile_id).startswith('Plaza_')];assert len(plaza)>=9
    center=unreal.Vector(sum(c.coordinate.x for c in plaza)/len(plaza)*size.x+450,sum(c.coordinate.y for c in plaza)/len(plaza)*size.y,0)
    center=unreal.MathLibrary.transform_location(generator.get_actor_transform(),center)
    hit=unreal.SystemLibrary.line_trace_single(generator,unreal.Vector(center.x,center.y,2000),unreal.Vector(center.x,center.y,-500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,False)
    if hit:floor_z=hit.to_tuple()[4].z
    else:
        # Headless editor worlds may not create query bodies. Use the actual authored plaza mesh bounds instead.
        mesh=unreal.load_asset('/Game/LanTuRebuild/Maps/万象天宫/Meshes/SM_Settlement_Plaza')
        assert mesh
        bounds=mesh.get_bounding_box();floor_z=unreal.MathLibrary.transform_location(generator.get_actor_transform(),unreal.Vector(0,0,bounds.max.z)).z
    center.z=floor_z+25
    platform=actors.spawn_actor_from_class(unreal.CC_LevitationPlatform,center)
    platform.set_actor_label('万象天宫 · 重组浮台')
    platform.tags=['SpiritTales.CarriedReassemblyPlatform.v1']
platform.deck.set_material(0,stone);platform.tiles.set_material(0,slab);platform.console.set_material(0,gold);platform.edges.set_material(0,gold)
assert platform.tiles.get_instance_count()==9
assert original_cells==[(str(c.tile_id),c.quarter_turns,c.coordinate.x,c.coordinate.y,c.coordinate.z) for c in generator.get_editor_property('displayed_result').cells]
after={a.get_name() for a in actors.get_all_level_actors()};assert before<=after
assert levels.save_current_level()
report={'map':map_path,'actor':platform.get_path_name(),'location':str(platform.get_actor_location()),'tiles':platform.tiles.get_instance_count(),'liftHeight':platform.lift_height,'liftSpeed':platform.lift_speed,'footprintScale':platform.footprint_scale,'existingActorsPreserved':True,'paintedLayoutPreserved':True,'firstPersonKey':'T','rebuildDefaultKey':'E','mode':'physical platform carries a grounded character; no flying mode'}
out=p/'Saved/FirstPerson';out.mkdir(exist_ok=True,parents=True);(out/'CarriedPlatformInstallation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('HEAVEN_CARRIED_PLATFORM_INSTALLED '+json.dumps(report,ensure_ascii=False))

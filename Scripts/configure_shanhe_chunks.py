"""Install a deterministic nine-chunk terrain window only in the Shanhe map."""
import unreal,json
from pathlib import Path
ROOT='/Game/HPWH/Spawning'
tools=unreal.AssetToolsHelpers.get_asset_tools()
M=unreal.MaterialEditingLibrary
def material(name,color,bamboo=False):
    asset=unreal.load_asset(ROOT+'/Materials/'+name)
    if not asset:
        asset=tools.create_asset(name,ROOT+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        tint=M.create_material_expression(asset,unreal.MaterialExpressionVectorParameter)
        tint.set_editor_property('parameter_name','BaseTint');tint.set_editor_property('default_value',unreal.LinearColor(*color,1))
        pos=M.create_material_expression(asset,unreal.MaterialExpressionWorldPosition)
        width=M.create_material_expression(asset,unreal.MaterialExpressionScalarParameter)
        width.set_editor_property('parameter_name','ChunkWidth');width.set_editor_property('default_value',8000.)
        custom=M.create_material_expression(asset,unreal.MaterialExpressionCustom)
        custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        pins=[]
        for name_ in ['Tint','P','Width']:
            pin=unreal.CustomInput();pin.set_editor_property('input_name',name_);pins.append(pin)
        custom.set_editor_property('inputs',pins)
        code='float ring=smoothstep(.03,.10,abs(frac(P.z/38)-.5));return Tint*lerp(.35,1.,ring);' if bamboo else '''
float noise=.9+.06*sin(P.x*.11)*sin(P.y*.09)+.04*sin(P.x*.031+P.y*.04);
float2 local=abs(frac((P.xy+Width*.5)/Width)*Width-Width*.5);
float road=1-smoothstep(230.,290.,min(local.x,local.y));
return lerp(Tint*noise,float3(.095,.067,.032),road);
'''
        custom.set_editor_property('code',code)
        for name_,node_ in [('Tint',tint),('P',pos),('Width',width)]:M.connect_material_expressions(node_,'',custom,name_)
        M.connect_material_property(custom,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=M.create_material_expression(asset,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.92)
        M.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        M.recompile_material(asset);M.layout_material_expressions(asset)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
    return asset

grounds=[material('M_ShanheGround_'+str(i),color) for i,color in enumerate([(.037,.072,.033),(.044,.079,.038),(.052,.069,.029)])]
bamboo=material('M_ShanheBamboo',(.035,.13,.045),True)
leaves=material('M_ShanheLeaves',(.022,.095,.032),True)
bp=unreal.load_asset(ROOT+'/BP_ShanheChunkWorld')
if not bp:
    factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.CC_ShanheChunkWorld)
    bp=tools.create_asset('BP_ShanheChunkWorld',ROOT,unreal.Blueprint,factory)
cdo=unreal.get_default_object(bp.generated_class())
for prop,value in [('ground_materials',grounds),('bamboo_material',bamboo),('leaf_material',leaves),('chunk_size',8000.),('world_seed',29717)]:cdo.set_editor_property(prop,value)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/HPWH/Maps/L_FlatTestArena')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing=actors.get_all_level_actors()
floors=[]
for actor in existing:
    if isinstance(actor,unreal.StaticMeshActor):
        origin,extent=actor.get_actor_bounds(False)
        if extent.x>4000 and extent.y>4000 and extent.z<600:
            tags=list(actor.tags)
            if unreal.Name('CC.ChunkLegacyGeometry') not in tags:tags.append(unreal.Name('CC.ChunkLegacyGeometry'))
            actor.set_editor_property('tags',tags);floors.append((actor,origin.z+extent.z))
ground_height=floors[0][1] if floors else 0.
worlds=[a for a in existing if isinstance(a,unreal.CC_ShanheChunkWorld)]
if worlds:
    generator=worlds[0]
    for duplicate in worlds[1:]:actors.destroy_actor(duplicate)
else:generator=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,0))
generator.set_actor_label('山河社稷_九宫地块生成器')
for prop,value in [('ground_height',ground_height),('ground_materials',grounds),('bamboo_material',bamboo),('leaf_material',leaves),('chunk_size',8000.)]:generator.set_editor_property(prop,value)
navs=[]
for actor in existing:
    if isinstance(actor,unreal.NavMeshBoundsVolume):
        origin,extent=actor.get_actor_bounds(False);scale=actor.get_actor_scale3d()
        assert extent.x>0 and extent.y>0
        actor.set_actor_scale3d(unreal.Vector(scale.x*12200/extent.x,scale.y*12200/extent.y,scale.z*600/max(1.,extent.z)))
        actor.set_actor_location(unreal.Vector(0,0,ground_height+150),False,False)
        tags=list(actor.tags)
        if unreal.Name('CC.ChunkNavigation') not in tags:tags.append(unreal.Name('CC.ChunkNavigation'))
        actor.set_editor_property('tags',tags);navs.append(actor.get_actor_label())
    if isinstance(actor,unreal.RecastNavMesh):
        actor.set_editor_property('runtime_generation',unreal.RuntimeGenerationType.DYNAMIC)
        actor.set_editor_property('force_rebuild_on_load',True)
assert navs,'Shanhe needs its existing navigation bounds'
levels.save_current_level()
Path(unreal.Paths.project_saved_dir(),'ShanheChunksSetup.json').write_text(json.dumps({'generator':generator.get_path_name(),'chunkSize':8000,'groundHeight':ground_height,'legacyFloors':[a.get_actor_label() for a,z in floors],'navigation':navs,'activeChunks':9},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('SHANHE_NINE_CHUNKS_CONFIGURED')

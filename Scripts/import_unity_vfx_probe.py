"""UE 5.6 editor script: import one Unity shockwave's art and reconstruct a mesh effect.
Run with -ExecutePythonScript; isolated assets, no edits to combat data or source Unity files.
"""
from pathlib import Path
import json, time, hashlib
import unreal

PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ROOT = '/Game/UnityVFXProbe'
manifest = json.loads((PROJECT/'SourceArt/UnityVFXProbe/manifest.json').read_text(encoding='utf-8'))
tools = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
skills = PROJECT/'Content/HPWH/Skills'
before = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in skills.glob('*.uasset')}
imported = {}
for row in manifest['dependencies']:
    if row['kind'] not in {'.tga','.png','.fbx'}:
        continue
    folder = ROOT + ('/Meshes' if row['kind']=='.fbx' else '/Textures')
    existing = unreal.load_asset(folder+'/'+row['asset_name']) if unreal.EditorAssetLibrary.does_asset_exist(folder+'/'+row['asset_name']) else None
    if existing:
        imported[row['asset_name']] = existing
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', row['staged'])
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', row['asset_name'])
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', False)
    task.set_editor_property('save', True)
    if row['kind']=='.fbx':
        options = unreal.FbxImportUI()
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_as_skeletal',False)
        options.set_editor_property('import_materials',False)
        options.set_editor_property('import_textures',False)
        options.set_editor_property('automated_import_should_detect_type',False)
        options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
        task.set_editor_property('options',options)
    tools.import_asset_tasks([task])
    assets = [unreal.load_asset(p) for p in task.get_editor_property('imported_object_paths')]
    asset = next((a for a in assets if isinstance(a,unreal.StaticMesh if row['kind']=='.fbx' else unreal.Texture2D)),None)
    assert asset, row['staged']
    if isinstance(asset,unreal.Texture2D):
        asset.set_editor_property('srgb',False)
        # Source channels are sampled explicitly as linear data, including the distortion normal.
        asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    imported[row['asset_name']] = asset

def texture(prefix):
    return next(a for n,a in imported.items() if n.startswith(prefix))

matpath = ROOT+'/Materials/M_Shockwave_Prototype'
mat = unreal.load_asset(matpath)
if not mat:
    mat = tools.create_asset('M_Shockwave_Prototype',ROOT+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
if mat:
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    def node(cls): return M.create_material_expression(mat,cls)
    def param(name,value):
        n=node(unreal.MaterialExpressionScalarParameter)
        n.set_editor_property('parameter_name',name)
        n.set_editor_property('default_value',value)
        return n
    uv=node(unreal.MaterialExpressionTextureCoordinate)
    clock=node(unreal.MaterialExpressionTime)
    color=node(unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name','Tint')
    color.set_editor_property('default_value',unreal.LinearColor(1,1,1,1))
    inputs={'UV':uv,'Clock':clock,'Tint':color,'StartTime':param('StartTime',0),
            'Duration':param('Duration',1),'LoopPreview':param('LoopPreview',1),'Brightness':param('Brightness',4)}
    for name,prefix in [('Base','T_VFX_Easy_Shockwaves_Noise_01_470'),('Noise','T_VFX_Easy_Shockwaves_Noise_03'),
                        ('Mask','T_VFX_Easy_Shockwaves_Linear_Mask'),('OpacityMask','T_VFX_Linear_Gradient_TD'),
                        ('Distortion','T_VFX_Easy_Shockwaves_Noise_01_Normal')]:
        t=node(unreal.MaterialExpressionTextureObject)
        t.set_editor_property('texture',texture(prefix))
        inputs[name]=t
    custom=node(unreal.MaterialExpressionCustom)
    custom.set_editor_property('description','Unity shockwave approximation: noise, distortion, erosion and one-second fade')
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    def custom_inputs(names):
        result=[]
        for name in names:
            entry=unreal.CustomInput();entry.set_editor_property('input_name',name);result.append(entry)
        return result
    custom.set_editor_property('inputs',custom_inputs(inputs))
    custom.set_editor_property('code','''
float age = max(0, Clock-StartTime)/max(Duration,0.01);
if (LoopPreview>0.5) age = fmod(max(0,Clock),Duration+1)/max(Duration,0.01);
float p=saturate(age);
float2 distortion=Texture2DSample(Distortion,DistortionSampler,UV+Clock*float2(.02,-.1)).xy*2-1;
float2 baseUV=UV+Clock*float2(-.05,-.2)+distortion*.1;
float3 base=Texture2DSample(Base,BaseSampler,baseUV).rgb;
float noise=Texture2DSample(Noise,NoiseSampler,UV+Clock*float2(-.07,-.3)).r;
float mask=Texture2DSample(Mask,MaskSampler,float2(UV.y,UV.y)+noise*.25+p*.3).g;
float intensity=saturate(pow(smoothstep(p*.6,p*.6+1,mask),5)*5);
float opacity=Texture2DSample(OpacityMask,OpacityMaskSampler,UV).g;
float fade=(1-p)*step(age,1.0);
return float4(base*Tint.rgb*Brightness*intensity,saturate(intensity*opacity*fade));
''')
    for name,expression in inputs.items(): M.connect_material_expressions(expression,'',custom,name)
    rgb=node(unreal.MaterialExpressionComponentMask)
    for channel in ['r','g','b']: rgb.set_editor_property(channel,True)
    assert M.connect_material_expressions(custom,'',rgb,'')
    alpha=node(unreal.MaterialExpressionComponentMask);alpha.set_editor_property('a',True)
    assert M.connect_material_expressions(custom,'',alpha,'')
    M.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    M.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    # Object-centred expansion in centimetres. Runtime: LoopPreview=0, StartTime=world time.
    pos=node(unreal.MaterialExpressionWorldPosition)
    center=node(unreal.MaterialExpressionObjectPositionWS)
    delta=node(unreal.MaterialExpressionSubtract)
    M.connect_material_expressions(pos,'',delta,'A');M.connect_material_expressions(center,'',delta,'B')
    expansion=node(unreal.MaterialExpressionCustom)
    expansion.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    expansion.set_editor_property('inputs',custom_inputs(['Clock','StartTime','Duration','LoopPreview']))
    expansion.set_editor_property('code','float a=max(0,Clock-StartTime)/max(Duration,.01); if(LoopPreview>.5) a=fmod(max(0,Clock),Duration+1)/max(Duration,.01); return lerp(.05,1,saturate(a))-1;')
    for name in ['Clock','StartTime','Duration','LoopPreview']:M.connect_material_expressions(inputs[name],'',expansion,name)
    wpo=node(unreal.MaterialExpressionMultiply)
    M.connect_material_expressions(delta,'',wpo,'A');M.connect_material_expressions(expansion,'',wpo,'B')
    M.connect_material_property(wpo,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    M.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
mesh=next(a for a in imported.values() if isinstance(a,unreal.StaticMesh))
mesh.set_material(0,mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)

factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.StaticMeshActor)
bp=unreal.load_asset(ROOT+'/Blueprints/BP_Shockwave_Prototype') or tools.create_asset('BP_Shockwave_Prototype',ROOT+'/Blueprints',unreal.Blueprint,factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=unreal.get_default_object(bp.generated_class())
component=cdo.get_editor_property('static_mesh_component')
component.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
component.set_static_mesh(mesh);component.set_material(0,mat)
component.set_editor_property('bounds_scale',4)
component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path=ROOT+'/Maps/L_Shockwave_Probe'
if unreal.EditorAssetLibrary.does_asset_exist(map_path):
    assert level.load_level(map_path)
    # This map is owned exclusively by this probe.
    for old in actors.get_all_level_actors():
        if isinstance(old,(unreal.StaticMeshActor,unreal.DirectionalLight,unreal.SkyLight)):
            actors.destroy_actor(old)
else:
    assert level.new_level(map_path)
effect=actors.spawn_actor_from_class(bp.generated_class(),unreal.Vector(0,0,100))
effect.set_actor_label('Unity_Shockwave_Prototype_Looping')
extent=mesh.get_bounds().box_extent
preview_scale=300/max(extent.x,extent.y,extent.z,1)
effect.set_actor_scale3d(unreal.Vector(preview_scale,preview_scale,preview_scale))
floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-10))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(unreal.Vector(20,20,.1))
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,500),unreal.Rotator(-50,0,0))
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,400))
view_location=unreal.Vector(700,-900,600)
view_rotation=unreal.MathLibrary.find_look_at_rotation(view_location,unreal.Vector(0,0,100))
unreal.EditorLevelLibrary.set_level_viewport_camera_info(view_location,view_rotation)
assert level.save_current_level()
after={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in skills.glob('*.uasset')}
assert before==after,'Skill data changed unexpectedly'
result={'status':'ASSETS_SAVED','textures':sum(isinstance(a,unreal.Texture2D) for a in imported.values()),
        'meshes':sum(isinstance(a,unreal.StaticMesh) for a in imported.values()),'material':matpath,'blueprint':ROOT+'/Blueprints/BP_Shockwave_Prototype',
        'map':map_path,'skill_data_unchanged':before==after,'unresolved_source_guids':manifest['unresolved_guids'],
        'limitations':['Approximate mesh/material reconstruction; not an automatic Niagara conversion.',
                       'Unity particle curves, custom streams and depth fade are not fully reproduced.',
                       'No gameplay binding or in-game visual parity validation.']}
out=PROJECT/'Saved/UnityVFXProbe';out.mkdir(parents=True,exist_ok=True)
(out/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('UNITY_VFX_PROBE_PASS '+json.dumps(result))
unreal.SystemLibrary.quit_editor()

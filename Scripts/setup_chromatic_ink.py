"""Preserve the migrated MatCap and add reversible surface/outline variants."""
import unreal, json
from pathlib import Path

ROOT='/Game/HPWH/Effects/ChromaticInk'
M=unreal.MaterialEditingLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
saved=[]

def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
    saved.append(asset.get_path_name())

def node(mat,cls):
    return M.create_material_expression(mat,cls)

def param(mat,name,value):
    result=node(mat,unreal.MaterialExpressionScalarParameter)
    result.set_editor_property('parameter_name',name)
    result.set_editor_property('default_value',value)
    return result

def custom(mat,label,code,inputs,kind=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    result=node(mat,unreal.MaterialExpressionCustom)
    result.set_editor_property('description',label)
    result.set_editor_property('output_type',kind)
    pins=[]
    for name in inputs:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
    result.set_editor_property('inputs',pins)
    result.set_editor_property('code',code)
    for name,source in inputs.items():
        assert M.connect_material_expressions(source,'',result,name)
    return result

def common(mat):
    clock=node(mat,unreal.MaterialExpressionTime)
    pos=node(mat,unreal.MaterialExpressionWorldPosition)
    center=node(mat,unreal.MaterialExpressionObjectPositionWS)
    relative=node(mat,unreal.MaterialExpressionSubtract)
    M.connect_material_expressions(pos,'',relative,'A');M.connect_material_expressions(center,'',relative,'B')
    normal=node(mat,unreal.MaterialExpressionVertexNormalWS)
    strength=param(mat,'DistortionStrength',1.)
    return {'P':relative,'Clock':clock,'N':normal,'Strength':strength}

def finish(mat):
    M.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    M.recompile_material(mat)
    M.layout_material_expressions(mat)
    save(mat)

source=unreal.load_asset('/Game/场景资源特效/流光特效/matcap测试')
assert isinstance(source,unreal.Material)
body=unreal.load_asset(ROOT+'/M_InkBlack')
if not body:
    body=tools.create_asset('M_InkBlack',ROOT,unreal.Material,unreal.MaterialFactoryNew())
    body.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    black=node(body,unreal.MaterialExpressionVectorParameter)
    black.set_editor_property('parameter_name','InkBlack')
    black.set_editor_property('default_value',unreal.LinearColor(.003,.003,.003,1))
    M.connect_material_property(black,'',unreal.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(param(body,'Roughness',.72),'',unreal.MaterialProperty.MP_ROUGHNESS)
    M.connect_material_property(param(body,'Specular',.18),'',unreal.MaterialProperty.MP_SPECULAR)
finish(body)

outline=unreal.load_asset(ROOT+'/M_InkOutline')
if not outline:
    outline=tools.create_asset('M_InkOutline',ROOT,unreal.Material,unreal.MaterialFactoryNew())
# Rebuild the same parent in place so both existing variants lose emission.
M.delete_all_material_expressions(outline)
outline.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
outline.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
outline.set_editor_property('two_sided',True)
outline.set_editor_property('opacity_mask_clip_value',.5)
outline.set_editor_property('max_world_position_offset_displacement',8.)
inputs=common(outline)
inputs['Width']=param(outline,'OutlineWidth',1.25)
inputs['FlameHeight']=param(outline,'FlameHeight',2.1)
inputs['ColorStrength']=param(outline,'OutlineColorStrength',.9)
wpo=custom(outline,'Inverted hull: normal expansion + rising organic flame tongues', '''
float t=Clock*2.1;
float a=sin(P.z*.12-t*2.7+sin(P.x*.17+t*.7));
float b=sin(P.x*.21+P.y*.17+sin(P.z*.11-t*1.6));
float flame=pow(saturate(.5+.5*a*b),2.4);
float ripple=sin(P.z*.32-t*4+P.y*.15);
float thickness=Width+Strength*(FlameHeight*flame+.25*ripple);
return N*max(.15,thickness)+Strength*float3(.35*a,.30*b,FlameHeight*.65*flame);
''',{k:inputs[k] for k in ['P','Clock','N','Strength','Width','FlameHeight']})
M.connect_material_property(wpo,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
color=custom(outline,'Non-emissive spectral edge color', '''
float phase=P.z*.009+P.x*.003-Clock*.065;
float3 spectrum=.52+.48*cos(6.2831853*(phase+float3(0,.333333,.666667)));
float shimmer=.85+.15*sin(P.z*.19-Clock*4+sin(P.x*.12));
return saturate(spectrum*ColorStrength*shimmer);
''',{k:inputs[k] for k in ['P','Clock','ColorStrength']})
M.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
zero_emission=node(outline,unreal.MaterialExpressionConstant)
zero_emission.set_editor_property('r',0.)
M.connect_material_property(zero_emission,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
M.connect_material_property(param(outline,'OutlineRoughness',.75),'',unreal.MaterialProperty.MP_ROUGHNESS)
M.connect_material_property(param(outline,'OutlineSpecular',.1),'',unreal.MaterialProperty.MP_SPECULAR)
facing=node(outline,unreal.MaterialExpressionTwoSidedSign)
mask=custom(outline,'Only expanded backfaces draw: crisp exterior contour', 'return saturate(-Facing);',{'Facing':facing},unreal.CustomMaterialOutputType.CMOT_FLOAT1)
M.connect_material_property(mask,'',unreal.MaterialProperty.MP_OPACITY_MASK)
finish(outline)
assert M.get_material_property_input_node(outline,unreal.MaterialProperty.MP_EMISSIVE_COLOR).get_editor_property('r')==0.

for suffix,strength in [('Clean',0.),('Burning',1.)]:
    for prefix,parent in [('InkMatCap',body),('InkOutline',outline)]:
        name='MI_'+prefix+'_'+suffix
        instance=unreal.load_asset(ROOT+'/'+name)
        if not instance: instance=tools.create_asset(name,ROOT,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        M.set_material_instance_parent(instance,parent)
        M.set_material_instance_scalar_parameter_value(instance,'DistortionStrength',strength)
        if parent==outline:M.set_material_instance_scalar_parameter_value(instance,'OutlineColorStrength',.9)
        save(instance)

health=unreal.load_asset(ROOT+'/M_ChromaticHealth')
if not health:
    health=tools.create_asset('M_ChromaticHealth',ROOT,unreal.Material,unreal.MaterialFactoryNew())
    health.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
    health.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
    uv=node(health,unreal.MaterialExpressionTextureCoordinate)
    clock=node(health,unreal.MaterialExpressionTime)
    gradient=custom(health,'Rainbow health fill: real horizontal spectrum, not a single cycling tint', '''
float h=UV.x*.85-Clock*.035;
return .52+.48*cos(6.2831853*(h+float3(0,.333333,.666667)));
''',{'UV':uv,'Clock':clock})
    M.connect_material_property(gradient,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    M.recompile_material(health);M.layout_material_expressions(health)
save(health)

bp=unreal.load_asset('/Game/HPWH/Blueprints/BP_ProgressionCharacter')
cdo=unreal.get_default_object(bp.generated_class())
cdo.set_editor_property('chromatic_body_material',unreal.load_asset(ROOT+'/MI_InkMatCap_Burning'))
cdo.set_editor_property('chromatic_outline_material',unreal.load_asset(ROOT+'/MI_InkOutline_Burning'))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save(bp)
ink_mesh=unreal.load_asset('/Game/HPWH/Characters/SK_InitialInk_Template')
slots=ink_mesh.get_editor_property('materials')
for index,slot in enumerate(slots):
    slot.set_editor_property('material_interface',body)
    slots[index]=slot
ink_mesh.set_editor_property('materials',slots)
save(ink_mesh)
# The native HUD has an editable soft asset reference for cooking the UI material.
Path(unreal.Paths.project_saved_dir(),'ChromaticInkSetup.json').write_text(json.dumps({'assets':saved,'defaultVariant':'Burning','sourceMatCap':source.get_path_name(),'outlineShading':'DefaultLit','outlineEmissive':0.},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CHROMATIC_INK_SETUP_COMPLETE')

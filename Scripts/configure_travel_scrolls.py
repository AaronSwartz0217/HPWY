"""Add paper-shop travel scrolls and small procedural UI icons; retain potion assets."""
import unreal,json
from pathlib import Path
M=unreal.MaterialEditingLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
root='/Game/HPWH/Items'
material=unreal.load_asset(root+'/Icons/M_TravelScrollIcon')
if not material:material=tools.create_asset('M_TravelScrollIcon',root+'/Icons',unreal.Material,unreal.MaterialFactoryNew())
M.delete_all_material_expressions(material)
material.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
uv=M.create_material_expression(material,unreal.MaterialExpressionTextureCoordinate)
realm=M.create_material_expression(material,unreal.MaterialExpressionScalarParameter)
realm.set_editor_property('parameter_name','Heaven')
shape=M.create_material_expression(material,unreal.MaterialExpressionCustom)
shape.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
pins=[]
for name in ['UV','Heaven']:
    pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
shape.set_editor_property('inputs',pins)
shape.set_editor_property('code',r'''
struct Shapes {
 float box(float2 p,float2 b,float r){float2 q=abs(p)-b+r;return length(max(q,0))+min(max(q.x,q.y),0)-r;}
 float segment(float2 p,float2 a,float2 b){float2 v=b-a;return length(p-a-v*saturate(dot(p-a,v)/dot(v,v)));}
} s;
float2 p=UV*2-1;
float aa=max(fwidth(p.x),.008);
float body=1-smoothstep(-aa,aa,s.box(p,float2(.49,.65),.035));
float rolls=1-smoothstep(-aa,aa,s.box(float2(p.x,abs(p.y)-.65),float2(.64,.12),.075));
float alpha=max(body,rolls);
float3 accent=lerp(float3(.12,.38,.30),float3(.38,.23,.52),Heaven);
float paperLight=.9+.1*cos(p.x*2.8);
float3 col=float3(.83,.71,.49)*paperLight;
float rollLight=.7+.3*cos((abs(p.y)-.65)*14);
col=lerp(col,float3(.70,.50,.25)*rollLight,rolls);
float caps=smoothstep(.53,.55,abs(p.x))*rolls;
col=lerp(col,accent*.65+float3(.12,.08,.02),caps);
float frame=(1-smoothstep(.012,.026,abs(abs(p.x)-.42)))*body*(1-step(.53,abs(p.y)));
col=lerp(col,float3(.53,.36,.13),frame);
float d=min(s.segment(p,float2(-.31,.21),float2(-.08,-.20)),s.segment(p,float2(-.08,-.20),float2(.15,.21)));
d=min(d,s.segment(p,float2(.05,.21),float2(.22,-.07)));
d=min(d,s.segment(p,float2(.22,-.07),float2(.34,.21)));
float mountains=1-smoothstep(.016,.029,d);
float roof=1;
for(int i=0;i<3;i++){
 float y=-.27+i*.19;float w=.17+i*.075;
 roof=min(roof,s.segment(p,float2(-w,y+.08),float2(0,y-.05)));
 roof=min(roof,s.segment(p,float2(0,y-.05),float2(w,y+.08)));
 roof=min(roof,s.segment(p,float2(-w+.04,y+.10),float2(w-.04,y+.10)));
}
float palace=1-smoothstep(.014,.027,roof);
col=lerp(col,accent,lerp(mountains,palace,Heaven)*body);
float ribbon=(1-smoothstep(.075,.09,abs(p.x)))*(1-smoothstep(.08,.1,abs(p.y-.49)))*body;
col=lerp(col,accent,ribbon);
return float4(col,alpha);
''')
assert M.connect_material_expressions(uv,'',shape,'UV')
assert M.connect_material_expressions(realm,'',shape,'Heaven')
for name,channels,prop in [('Color','rgb',unreal.MaterialProperty.MP_EMISSIVE_COLOR),('Alpha','a',unreal.MaterialProperty.MP_OPACITY)]:
    mask=M.create_material_expression(material,unreal.MaterialExpressionComponentMask)
    for channel in 'rgba':mask.set_editor_property(channel,channel in channels)
    assert M.connect_material_expressions(shape,'',mask,'')
    assert M.connect_material_property(mask,'',prop)
M.recompile_material(material);M.layout_material_expressions(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material,False)
catalog=[]
for region,name,price,heaven in [('Arena','山河社稷传送卷轴',20,0.),('Heaven','万象天宫传送卷轴',30,1.)]:
    icon_name='MI_TravelScroll_'+region
    icon=unreal.load_asset(root+'/Icons/'+icon_name)
    if not icon:icon=tools.create_asset(icon_name,root+'/Icons',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    M.set_material_instance_parent(icon,material);M.set_material_instance_scalar_parameter_value(icon,'Heaven',heaven)
    unreal.EditorAssetLibrary.save_loaded_asset(icon,False)
    key='Travel_'+region
    item=unreal.load_asset(root+'/DA_Item_'+key)
    if not item:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.CC_ConsumableDefinition)
        item=tools.create_asset('DA_Item_'+key,root,unreal.CC_ConsumableDefinition,factory)
    for field,value in dict(item_id=key,display_name=name,effect=unreal.CC_ConsumableEffect.TRAVEL_SCROLL,destination_region=region,copper_price=price,max_stack=99,icon_material=icon,
        description='释放与家园传送门相同的传送球，留在球附近等待8秒。成功抵达消耗1张，远离取消不消耗。所有传送共用30秒冷却；也可在N行旅中选择目的地使用。').items():item.set_editor_property(field,value)
    unreal.EditorAssetLibrary.save_loaded_asset(item,False);catalog.append(item)
bp=unreal.load_asset('/Game/HPWH/Blueprints/BP_ProgressionPlayerState')
progression=unreal.get_default_object(bp.generated_class()).get_editor_property('progression')
existing=list(progression.get_editor_property('item_catalog'))
for item in catalog:
    if item not in existing:existing.append(item)
progression.set_editor_property('item_catalog',existing)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
Path(unreal.Paths.project_saved_dir(),'TravelScrollSetup.json').write_text(json.dumps({'catalogCount':len(existing),'scrolls':[{'path':x.get_path_name(),'region':str(x.get_editor_property('destination_region')),'price':x.get_editor_property('copper_price')}for x in catalog]},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TRAVEL_SCROLL_SETUP_COMPLETE')

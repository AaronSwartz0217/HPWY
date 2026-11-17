import unreal,json,time
from pathlib import Path
r=unreal.AssetRegistryHelpers.get_asset_registry(); r.search_all_assets(True)
report={}
for name in ['GA_Primary','GA_Secondary','GA_Tertiary']:
    bp=unreal.load_asset('/Game/THAT/Player/'+name)
    text=unreal.CC_AnimationSetupLibrary.describe_blueprint_graphs(bp)
    Path(unreal.Paths.project_saved_dir(),name+'_GraphAudit.txt').write_text(text,encoding='utf-8')
    obj=unreal.get_default_object(bp.generated_class())
    values={}
    for field in ['hit_box_radius','hit_box_forward_offset','hit_box_elevation_offset','ability_tags','instancing_policy','net_execution_policy','montage','damage','damage_effect','cooldown_gameplay_effect_class','cost_gameplay_effect_class']:
        try: values[field]=str(obj.get_editor_property(field))
        except Exception: pass
    report[name]=values
for path in ['/Game/THAT/Player/BP_CC_PlayController','/Game/HPWH/Blueprints/BP_ProgressionController']:
    cdo=unreal.get_default_object(unreal.load_asset(path).generated_class())
    report[path]=[]
    for context in cdo.get_editor_property('input_mapping_contexts'):
        report[path].append(dict(context=context.get_path_name(),mappings=[dict(action=m.get_editor_property('action').get_path_name(),key=m.get_editor_property('key').export_text()) for m in context.get_editor_property('mappings')]))
for path in ['/Game/THAT/BP_PlayerCharacter','/Game/HPWH/Blueprints/BP_ProgressionCharacter']:
    cdo=unreal.get_default_object(unreal.load_asset(path).generated_class())
    report[path]=[str(c) for c in cdo.get_editor_property('startup_abilities')]
Path(unreal.Paths.project_saved_dir(),'LegacyAbilityAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('LEGACY_ABILITY_AUDIT_PASS')
quit_at=time.monotonic()+3
def done(delta):
    if time.monotonic()>quit_at:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(done)

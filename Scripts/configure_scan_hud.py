import unreal
folder='/Game/UI/Scan'
def asset(name,cls):
 obj=unreal.load_asset(folder+'/'+name)
 if not obj:
  factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',cls)
  obj=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,cls,factory)
 return obj
rows=[
 ('Platform','浮空石台','古老的浮台会托举站在上面的人。离开后恢复下落，交互可重组台面。','重组浮台',unreal.CC_LevitationPlatform.static_class(),None),
 ('Artifact','物魄信物','器物中封存着物魄之力。靠近交互释放力量，再收取浮现的灵光。','唤醒信物',None,unreal.CC_ArtifactComponent.static_class()),
 ('Experience','经验灵物','蕴藏阅历的灵物。靠近交互后释放经验灵光。','汲取灵光',unreal.CC_ExperiencePack.static_class(),None),
 ('Enemy','敌对物魄','受物魄驱动的敌对存在。观察其动作，寻找出手时机。','',unreal.CC_EnemyCharacter.static_class(),None),
 ('Example','无名器物','这里填写这件器物的来历、用途或探索提示。','',None,None)]
rules=[]
for key,title,body,action,actor,component in rows:
 info=asset('DA_Scan_'+key,unreal.CC_ScanDescription)
 # Re-running setup preserves designer edits to existing descriptions.
 if not str(info.get_editor_property('display_name')):
  info.set_editor_property('display_name',title);info.set_editor_property('description',body);info.set_editor_property('action_text',action);info.set_editor_property('max_distance',600.)
 unreal.EditorAssetLibrary.save_loaded_asset(info)
 if actor or component:
  rule=unreal.CC_ScanRule();rule.set_editor_property('actor_class',actor);rule.set_editor_property('component_class',component);rule.set_editor_property('info',info);rules.append(rule)
catalog=asset('DA_ScanCatalog',unreal.CC_ScanCatalog)
if not list(catalog.get_editor_property('rules')):catalog.set_editor_property('rules',rules)
unreal.EditorAssetLibrary.save_loaded_asset(catalog)
unreal.log('FIRST_PERSON_SCAN_DATA_SETUP_PASS: UI/Scan contains catalog, four default profiles and an editable example')

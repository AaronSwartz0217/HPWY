"""Import 18 design icons + 9 frame components. Presentation only; never edits GAS skill assets."""
from pathlib import Path
import json,hashlib,struct,time
import unreal
SRC=Path(r'C:\Users\29717\Desktop\物魄奇谭2d资源')
ROOT='/Game/UI/MuralMenu'
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source_json=SRC/'技能配置/技能配置_v0.1.json'
design=json.loads(source_json.read_text(encoding='utf-8-sig'))
assert len(design['skills'])==18
skills_dir=project/'Content/HPWH/Skills'
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in skills_dir.glob('*.uasset')}
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
records=[]
tasks=[]
components=[('顶部栏位01_空白','T_Skill_Tab_01'),('顶部栏位02_空白','T_Skill_Tab_02'),('顶部栏位03_空白','T_Skill_Tab_03'),('顶部栏位04_空白','T_Skill_Tab_04'),('技能卡框_普通','T_Skill_Card_Normal'),('技能卡框_选中','T_Skill_Card_Selected'),('键位框_普通','T_Skill_Slot_Normal'),('键位框_选中','T_Skill_Slot_Selected'),('右侧大面板_空白','T_Skill_Detail_Panel')]
for stem,name in components: records.append((SRC/'技能界面空白组件'/(stem+'.png'),ROOT+'/Textures/Skills/Components',name))
for row in design['skills']: records.append((SRC/'技能图标'/row['faction']/(row['name']+'.png'),ROOT+'/Textures/Skills/Icons','T_Skill_'+row['id']))
report=[]
for source,folder,name in records:
 assert source.is_file(),source
 task=unreal.AssetImportTask(); task.filename=str(source);task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;tasks.append(task)
 blob=source.read_bytes();w,h=struct.unpack('>II',blob[16:24]);report.append({'source':str(source),'asset':folder+'/'+name,'width':w,'height':h,'sha256':hashlib.sha256(blob).hexdigest()})
TOOLS.import_asset_tasks(tasks)
textures={}
for source,folder,name in records:
 t=unreal.load_asset(folder+'/'+name); assert t
 t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
 t.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
 t.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
 t.set_editor_property('srgb',True);t.set_editor_property('compression_no_alpha',False);t.set_editor_property('never_stream',True)
 assert unreal.EditorAssetLibrary.save_loaded_asset(t,False)
 textures[name]=t
cls=unreal.load_class(None,'/Script/CrashCourse.CC_SkillGalleryData');assert cls
asset=unreal.load_asset(ROOT+'/DA_SkillGallery')
if not asset:
 f=unreal.DataAssetFactory();f.set_editor_property('data_asset_class',cls);asset=TOOLS.create_asset('DA_SkillGallery',ROOT,cls,f)
bound_skills={str(e.get_editor_property('design_id')):e.get_editor_property('executable_skill') for e in asset.get_editor_property('entries')}
categories=['通用系','青铜天卫','古刹法相','田野陶灵']
asset.set_editor_property('categories',categories)
asset.set_editor_property('category_tabs',[textures['T_Skill_Tab_%02d'%i] for i in range(1,5)])
for prop,name in [('detail_panel','T_Skill_Detail_Panel'),('card_normal','T_Skill_Card_Normal'),('card_selected','T_Skill_Card_Selected'),('slot_normal','T_Skill_Slot_Normal'),('slot_selected','T_Skill_Slot_Selected')]: asset.set_editor_property(prop,textures[name])
def n(v): return '%g'%v
def stats(r):
 if r['id']=='bronze_charged_hit': return [('伤害倍率','200%—400%攻击力'),('最大蓄力','2秒'),('冷却时间','12秒'),('满蓄力硬直','1.2秒')]
 if 'summon' in r: return [('召唤数量',str(r['summon']['count'])+'名'),('持续时间',n(r['duration_seconds'])+'秒'),('攻击间隔',n(r['summon']['attack_interval_seconds'])+'秒'),('冷却时间',n(r['cooldown_seconds'])+'秒')]
 values=[]
 damage=r.get('damage_attack_percent',[])
 if damage: values.append(('伤害倍率',n(sum(damage))+'%攻击力'))
 if 'heal_max_health_percent' in r:values.append(('回复生命',n(r['heal_max_health_percent'])+'%最大生命'))
 if 'heal_per_tick_max_health_percent' in r:values.append(('每秒回复',n(r['heal_per_tick_max_health_percent'])+'%最大生命'))
 if 'lifesteal_actual_health_damage_percent' in r:values.append(('生命汲取',n(r['lifesteal_actual_health_damage_percent'])+'%实际生命伤害'))
 if r.get('invulnerable'):values.append(('金身效果','免疫伤害与控制'))
 if 'attack_stat_total_percent' in r:values.append(('攻击力',n(r['attack_stat_total_percent'])+'%基准值'))
 if 'defense_stat_total_percent' in r:values.append(('防御力',n(r['defense_stat_total_percent'])+'%基准值'))
 if 'movement_speed_total_percent' in r:values.append(('移动速度',n(r['movement_speed_total_percent'])+'%基准值'))
 if 'shield_max_health_percent' in r:values.append(('护盾数值',n(r['shield_max_health_percent'])+'%生命 + '+n(r['shield_defense_percent'])+'%防御'))
 if r.get('duration_seconds',0):values.append(('持续时间',n(r['duration_seconds'])+'秒'))
 values.append(('冷却时间',n(r['cooldown_seconds'])+'秒'))
 if 'range_cm' in r and len(values)<4:values.append(('作用距离',n(r['range_cm']/100)+'米'))
 if 'radius_cm' in r and len(values)<4:values.append(('作用半径',n(r['radius_cm']/100)+'米'))
 if 'stagger_seconds' in r and len(values)<4:values.append(('硬直时间',n(r['stagger_seconds'])+'秒'))
 return values[:4]
entries=[]
for r in design['skills']:
 row=unreal.CC_SkillGalleryEntry();row.set_editor_property('design_id',r['id']);row.set_editor_property('executable_skill',bound_skills.get(r['id']));row.set_editor_property('display_name',r['name']);row.set_editor_property('category',categories.index(r['faction']));row.set_editor_property('icon',textures['T_Skill_'+r['id']]);pairs=stats(r)
 row.set_editor_property('stat_labels',[x[0] for x in pairs]);row.set_editor_property('stat_values',[x[1] for x in pairs]);row.set_editor_property('description',r['effect']);entries.append(row)
asset.set_editor_property('entries',entries);assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
menu=unreal.load_asset(ROOT+'/DA_MenuArt');assert menu;menu.set_editor_property('skill_gallery',asset);assert unreal.EditorAssetLibrary.save_loaded_asset(menu,False)
after={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in skills_dir.glob('*.uasset')};assert before==after,'Combat assets changed'
source_target=project/'SourceArt/UI/SkillDesign_v0.1.json';source_target.parent.mkdir(parents=True,exist_ok=True);source_target.write_bytes(source_json.read_bytes())
result={'artImportOnly':True,'preservesExecutableBindings':True,'icons':18,'components':9,'categories':categories,'entries':[{'designId':r['id'],'name':r['name'],'category':r['faction']} for r in design['skills']],'sourceDesign':str(source_json),'textures':report,'combatAssetsUnchanged':before==after,'designSourceSha256':hashlib.sha256(source_json.read_bytes()).hexdigest()}
out=project/'Saved/UI/SkillGalleryImport.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('SKILL_GALLERY_IMPORT_PASS: 18 icons + 9 components; combat assets unchanged')
quit_at=time.monotonic()+4
def done(delta):
 if time.monotonic()>quit_at:
  unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(done)

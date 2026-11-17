"""Keep existing skill names, numerical tuning and art; update faction interaction descriptions."""
import unreal,json,shutil
from pathlib import Path
p=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
backup=p/'Saved/Backups/FactionEffectsAndHealth_20261028'
text={
'bronze_summon':'召唤3名青铜兵人协助作战60秒。兵人优先集火你直接攻击的目标，空闲时协助防卫；每2秒造成你当前攻击力25%的伤害，具有你召唤时50%最大生命及实时100%防御，最多3名。',
'bronze_evolve':'20秒内攻击力变为基准值140%，防御力变为基准值150%；兵人实时继承强化后的攻防。不叠加，不永久解锁形态。',
'bronze_guard':'立即开启5秒正面120度守御，可中断当前攻击。正面攻击按基准防御力200%结算并免疫其击退，已有更强防御不被降低；侧后方正常结算。',
'temple_golden_body':'可在受控时立即施放，进入金身2秒，免疫伤害、硬直、击退、击飞与定身。不驱散已有负面效果，结束后尚未到期的效果继续。',
'temple_gravity':'在选定地面区域形成3秒重力场，起始造成40%攻击力伤害并牵引敌人，结束造成80%，并击飞可控敌人、使其硬直1.5秒。前排敌人不阻挡范围命中，墙体仍会阻挡。',
'temple_lamps':'3秒内以自身为中心释放6轮佛光，每0.5秒一轮，每轮50%攻击力伤害，完整命中300%。可边移动边施放其他技能，前排敌人不阻挡范围命中。',
'field_root_prison':'在目标地面区域召出根藤，造成120%攻击力伤害并定身3秒，敌人仍可攻击。这次直接伤害可触发生生不息的吸血；首领受伤但不受硬控。',
'field_lifesteal':'10秒内，将自身直接攻击造成的实际生命伤害的20%转为回复，每秒最多8%最大生命；可配合藤缚地牢。召唤物、持续伤害和护盾吸收不触发。'}
def preserve(package):
 relative=Path('Content')/Path(package.removeprefix('/Game/')+'.uasset');src=p/relative;dst=backup/relative
 if src.exists() and not dst.exists():dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
source_path=p/'SourceArt/UI/SkillDesign_v0.1.json';b=backup/'SourceArt/UI/SkillDesign_v0.1.json'
if not b.exists():b.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source_path,b)
data=json.loads(source_path.read_text(encoding='utf-8-sig'));numbers=[{k:v for k,v in s.items() if k!='effect'} for s in data['skills']]
for s in data['skills']:
 if s['id'] in text:s['effect']=text[s['id']]
assert numbers==[{k:v for k,v in s.items() if k!='effect'} for s in data['skills']]
source_path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gallery_path='/Game/UI/MuralMenu/DA_SkillGallery';preserve(gallery_path);gallery=unreal.load_asset(gallery_path);entries=gallery.entries
changed=[]
for e in entries:
 key=str(e.design_id)
 if key not in text:continue
 skill=e.executable_skill
 assert skill,key
 preserve(skill.get_path_name().split('.')[0]);skill.set_editor_property('description',text[key]);e.set_editor_property('description',text[key])
 assert unreal.EditorAssetLibrary.save_loaded_asset(skill,False);changed.append(key)
gallery.set_editor_property('entries',entries);assert unreal.EditorAssetLibrary.save_loaded_asset(gallery,False)
assert len(changed)==len(text)
unreal.log('FACTION_DESCRIPTIONS_UPDATED_WITH_NUMBERS_PRESERVED '+str(changed))

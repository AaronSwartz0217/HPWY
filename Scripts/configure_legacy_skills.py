"""Preserve reusable THAT ability mechanics as extra skills without enabling old debug graphs."""
import unreal
from pathlib import Path
import json
ROOT='/Game/HPWH'
tools=unreal.AssetToolsHelpers.get_asset_tools()
extras=[]
for i, source, values in [
    (21,'DA_Skill_05',dict(display_name='21 · 交替拳击（原 Primary）',description='沿用原 Primary 的左右拳交替机制，每次施放一拳，伤害为当前攻击力的100%。使用当前形态对应的拳击动画。',alternate_attack_pose=True,power=1.,mana_cost=0.,cooldown=.6,range=220.,windup=.12)),
    (22,'DA_Skill_18',dict(display_name='22 · 短时疾跑（原 Tertiary）',description='沿用原 Tertiary 的短时加速机制：移速增加500，持续1秒，结束后自动恢复。基础移速500时达到1000。',alternate_attack_pose=False,power=500.,power_per_level=0.,mana_cost=5.,cooldown=5.,duration=1.,windup=0.))]:
    name=f'DA_Skill_{i:02d}'
    asset=unreal.load_asset(ROOT+'/Skills/'+name) or unreal.EditorAssetLibrary.duplicate_asset(ROOT+'/Skills/'+source,ROOT+'/Skills/'+name)
    assert asset
    values['skill_id']=f'Skill_{i:02d}'
    for k,v in values.items(): asset.set_editor_property(k,v)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
    extras.append(asset)
bp=unreal.load_asset(ROOT+'/Blueprints/BP_ProgressionPlayerState')
p=unreal.get_default_object(bp.generated_class()).get_editor_property('progression')
catalog=[unreal.load_asset(ROOT+f'/Skills/DA_Skill_{i:02d}') for i in range(1,21)]+extras
p.set_editor_property('skill_catalog',catalog)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
Path(unreal.Paths.project_saved_dir(),'LegacySkillsSetup.json').write_text(json.dumps(dict(count=len(catalog),extras=[s.get_path_name() for s in extras]),ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('LEGACY_SKILLS_SETUP_PASS: 22 skills')

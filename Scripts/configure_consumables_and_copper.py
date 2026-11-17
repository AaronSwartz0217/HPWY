"""Configure the five requested potions and shared enemy copper rewards after a C++ build."""
import unreal, json
from pathlib import Path
ROOT='/Game/HPWH'
tools=unreal.AssetToolsHelpers.get_asset_tools()
def save(asset):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False), asset
specs=[
 ('XP_Small','小经验药水','EXPERIENCE',5,50.,0.,'使用后获得 50 经验。'),
 ('XP_Medium','中经验药水','EXPERIENCE',15,180.,0.,'使用后获得 180 经验。'),
 ('XP_Large','大经验药水','EXPERIENCE',40,500.,0.,'使用后获得 500 经验。'),
 ('CooldownHaste','CD加速药水','COOLDOWN_HASTE',15,2.,10.,'10 秒内，四个技能与普攻的冷却以 2 倍速度恢复；不改变技能持续时间。'),
 ('CooldownReset','一键刷新CD药水','RESET_COOLDOWNS',30,0.,0.,'立即刷新四个技能与普攻的冷却；没有技能冷却时不消耗。'),
]
catalog=[]
for key,name,effect,price,magnitude,duration,desc in specs:
    asset=unreal.load_asset(ROOT+'/Items/DA_Item_'+key)
    if not asset:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.CC_ConsumableDefinition)
        asset=tools.create_asset('DA_Item_'+key,ROOT+'/Items',unreal.CC_ConsumableDefinition,factory)
    assert asset
    for field,value in dict(item_id=key,display_name=name,description=desc+' 所有药水共用 15 秒道具冷却。',effect=getattr(unreal.CC_ConsumableEffect,effect),copper_price=price,max_stack=99,magnitude=magnitude,duration=duration).items():asset.set_editor_property(field,value)
    save(asset);catalog.append(asset)
bp=unreal.load_asset(ROOT+'/Blueprints/BP_ProgressionPlayerState')
cdo=unreal.get_default_object(bp.generated_class());progression=cdo.get_editor_property('progression')
for extra in progression.get_editor_property('item_catalog'):
    if extra and extra not in catalog:catalog.append(extra)
progression.set_editor_property('item_catalog',catalog);progression.set_editor_property('item_cooldown_seconds',15.)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
balance=unreal.load_asset(ROOT+'/Balance/DA_CombatBalance');profiles=balance.get_editor_property('profiles')
rewards={f'{family}_T{i+1}':base*scale for family,base in [('Nature',3),('Bronze',5),('Ceramic',4)] for i,scale in enumerate([1,2,4])}
rewards.update(Nature_PotHead=12)
for i,profile in enumerate(profiles):
    key=str(profile.get_editor_property('profile_id'))
    if key in rewards:profile.set_editor_property('copper_reward',rewards[key]);profiles[i]=profile
balance.set_editor_property('profiles',profiles);save(balance)
coin=unreal.load_asset(ROOT+'/Coins/BP_CopperCoin');unreal.get_default_object(coin.generated_class()).set_editor_property('copper_amount',5)
unreal.BlueprintEditorLibrary.compile_blueprint(coin);save(coin)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(ROOT+'/Maps/L_FlatTestArena')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.CC_CoinPickup):actor.set_editor_property('copper_amount',5)
    if actor.get_actor_label()=='CopperCoin_RewardHint':
        text=actor.get_component_by_class(unreal.TextRenderComponent)
        if text:text.set_text('COPPER COINS | TOUCH +5 COPPER / +5 XP')
levels.save_current_level()
Path(unreal.Paths.project_saved_dir(),'ConsumableSetup.json').write_text(json.dumps({'items':[{'id':s[0],'name':s[1],'price':s[3],'magnitude':s[4],'duration':s[5]} for s in specs],'copperRewards':rewards,'coinValue':5},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('CONSUMABLES_AND_COPPER_CONFIGURED')

"""Place five shop entrances on the largest existing residential buildings; keep the art intact."""
import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/LanTuRebuild/Maps/古风小镇/古风小镇')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors=actors.get_all_level_actors()
buildings={actor.get_actor_label():actor for actor in all_actors}
specs=[
 ('Inn','归灯客栈','楼',unreal.CC_TownShopId.INN,(0,-1)),
 ('Bronze','问斤铜铺','平房2',unreal.CC_TownShopId.BRONZE,(0,-1)),
 ('Ceramic','留火陶坊','平房2江景2',unreal.CC_TownShopId.CERAMIC,(0,-1)),
 ('Bamboo','听雨竹行','平房2江景',unreal.CC_TownShopId.BAMBOO,(0,1)),
 ('Books','旧纸摊','平房2江景3',unreal.CC_TownShopId.BOOKS,(1,0)),
]
report=[]
for key,name,label,shop_id,side in specs:
    building=buildings.get(label);assert building, label
    center,extent=building.get_actor_bounds(False)
    base=building.get_actor_location()
    # Roof bounds include the eaves: an 85cm offset places the trigger on clear pavement.
    position=unreal.Vector(center.x+side[0]*(extent.x+85),center.y+side[1]*(extent.y+85),base.z+96)
    existing=[actor for actor in all_actors if isinstance(actor,unreal.CC_TownShop) and actor.get_editor_property('shop_id')==shop_id]
    entrance=existing[0] if existing else actors.spawn_actor_from_class(unreal.CC_TownShop,position)
    for duplicate in existing[1:]:actors.destroy_actor(duplicate)
    entrance.set_actor_location(position,False,False);entrance.set_actor_label('店铺_'+name)
    entrance.set_editor_property('shop_id',shop_id);entrance.set_editor_property('enabled',True)
    entrance.get_editor_property('interaction_zone').set_box_extent(unreal.Vector(220,220,180),False)
    entrance.set_editor_property('tags',[unreal.Name('CC.TownShop.'+key)])
    report.append(dict(id=key,name=name,building=label,buildingActor=building.get_name(),entrance=[position.x,position.y,position.z],halfExtent=[220,220,180]))
assert levels.save_current_level()
Path(unreal.Paths.project_saved_dir(),'TownShopSetup.json').write_text(json.dumps({'remoteShopPrice':100,'restPrice':5,'shops':report},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('TOWN_SHOPS_CONFIGURED_FIVE_BUILDINGS')

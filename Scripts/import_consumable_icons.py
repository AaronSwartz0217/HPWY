"""Import the generated low-resolution icon set and bind it to the existing item assets."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()/'SourceArt/UI/Items'
tools=unreal.AssetToolsHelpers.get_asset_tools()
names=['XP_Small','XP_Medium','XP_Large','CooldownHaste','CooldownReset']
for name in names:
    task=unreal.AssetImportTask();task.set_editor_property('filename',str(root/f'T_Item_{name}.png'));task.set_editor_property('destination_path','/Game/HPWH/Items/Icons');task.set_editor_property('destination_name','T_Item_'+name);task.set_editor_property('automated',True);task.set_editor_property('replace_existing',True);task.set_editor_property('save',True)
    tools.import_asset_tasks([task])
    texture=unreal.load_asset('/Game/HPWH/Items/Icons/T_Item_'+name);assert texture
    for prop,value in [('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI),('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),('srgb',True),('never_stream',True),('max_texture_size',128)]:texture.set_editor_property(prop,value)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
    item=unreal.load_asset('/Game/HPWH/Items/DA_Item_'+name);assert item
    item.set_editor_property('icon',texture);assert unreal.EditorAssetLibrary.save_loaded_asset(item,False)
unreal.log('CONSUMABLE_ICONS_IMPORTED_128PX')

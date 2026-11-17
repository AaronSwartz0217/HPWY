"""创建地形分类预制体数据资产（如果不存在）。"""

import unreal

ASSET_PATH = "/Game/HPWH/Prefabs/DA_HPWH_TerrainPrefabs"
EAL = unreal.EditorAssetLibrary

if EAL.does_asset_exist(ASSET_PATH):
    unreal.log("HPWH: prefab data asset already exists")
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = tools.create_asset("DA_HPWH_TerrainPrefabs", "/Game/HPWH/Prefabs",
        unreal.HPWH_TerrainPrefabSet, None)
    if asset:
        EAL.save_loaded_asset(asset)
        unreal.log("HPWH: created prefab data asset %s" % ASSET_PATH)
    else:
        unreal.log_error("HPWH: failed to create prefab data asset")

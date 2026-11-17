"""Read-only source skeleton inventory; writes its report under Saved only."""
import json
from pathlib import Path
import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
report = {"clips": [], "candidateMeshes": []}
for name in ["Anim_Jump_Action", "Anim_Falling_Reaction", "Anim_Land_Reaction", "Anim_Death_Reaction", "Anim_Jab_Action", "Anim_CrossJab_Action", "Anim_Uppercut_Action"]:
    asset = unreal.load_asset("/Game/GameAnimsPack/Animations/" + name)
    assert asset, name
    report["clips"].append({"asset": asset.get_path_name(), "skeleton": asset.get_editor_property("skeleton").get_path_name()})
for root in ["/Game/GameAnimsPack/Demo", "/Game/Characters"]:
    for data in registry.get_assets_by_path(root, recursive=True):
        if str(data.asset_class_path.asset_name) == "SkeletalMesh":
            asset = data.get_asset()
            report["candidateMeshes"].append({"asset": asset.get_path_name(), "skeleton": asset.get_editor_property("skeleton").get_path_name()})
Path(unreal.Paths.project_saved_dir() + "THATSourceSkeletons.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("THAT_SOURCE_SKELETONS: " + json.dumps(report, ensure_ascii=False))

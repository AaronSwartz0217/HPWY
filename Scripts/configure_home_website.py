"""Update only the homepage link. Editor asset operation; never starts a game."""
import json
from pathlib import Path
import unreal

path = "/Game/UI/MainMenu/DA_HomeArt"
url = "https://aaronswartz0217.github.io/Spirit-Tales/"
asset = unreal.load_asset(path)
assert asset, path
previous = asset.get_editor_property("website_url")
asset.set_editor_property("website_url", url)
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
assert asset.get_editor_property("website_url") == url
project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
report = project / "Saved/UI/HomeWebsiteSetup.json"
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps({"asset": path, "previous": previous, "websiteURL": url,
                             "gameStarted": False}, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("HOME_WEBSITE_UPDATED " + url)
unreal.SystemLibrary.quit_editor()

import unreal
from pathlib import Path
import shutil
p=Path(unreal.Paths.project_dir());backup=p/'Saved/Backups/PlayerParticleFix_20261028/Content/HPWH/Skills/DA_Skill_01.uasset'
if not backup.exists():backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p/'Content/HPWH/Skills/DA_Skill_01.uasset',backup)
folder='/Game/HPWH/Effects/PlayerParticle'
core=unreal.load_asset(folder+'/M_PlayerParticleCore')
if not core:
 core=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_PlayerParticleCore',folder,unreal.Material,unreal.MaterialFactoryNew())
 core.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
 color=unreal.MaterialEditingLibrary.create_material_expression(core,unreal.MaterialExpressionConstant3Vector)
 color.set_editor_property('constant',unreal.LinearColor(12.,5.,.2,1.))
 unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 unreal.MaterialEditingLibrary.recompile_material(core);unreal.EditorAssetLibrary.save_loaded_asset(core)
unreal.SystemLibrary.execute_console_command(None,'CC.BuildGoldParticleFX')

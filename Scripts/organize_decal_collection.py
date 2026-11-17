import unreal,json,zipfile,hashlib
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
root='/Game/场景资源'
target=root+'/贴花资源合集'
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(synchronous_search=True)
lib=unreal.EditorAssetLibrary
selected={}
for a in reg.get_assets_by_path(root,recursive=True):
 p=str(a.package_name)
 if p.startswith(target+'/'):continue
 name=p.rsplit('/',1)[-1]
 if '/乌云庙宇/aaa/Decal/' in p:
  group='01_现成贴花/材质' if '/text/' not in p else '01_现成贴花/配套纹理与材质'
 elif '/乌云庙宇/aaa/' in p and name=='MI_DirtPatch':group='02_泥土斑块'
 elif any(k in name for k in ['Rock_Leak_Overlay','T_RainLeak_','T_Grunge_']) or name=='T_Leak':
  source=p[len(root)+1:].split('/')[0];group='03_污渍雨痕纹理/'+source
 else:continue
 selected[p]=target+'/'+group+'/'+name
# Collect the original packages recursively for a restorable backup.
opts=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
closure=set();pending=list(selected)
while pending:
 p=pending.pop()
 if p in closure:continue
 closure.add(p)
 for d in reg.get_dependencies(p,opts):
  d=str(d)
  if d.startswith('/Game/') and d not in closure:pending.append(d)
backup=project/'资源备份'/'贴花资源_20261101';backup.mkdir(parents=True,exist_ok=True)
missing=[];entries=[]
with zipfile.ZipFile(backup/'贴花原始资源及依赖.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(closure):
  base=project/'Content'/p[6:];found=False
  for ext in ['.uasset','.uexp','.ubulk']:
   file=Path(str(base)+ext)
   if file.is_file():
    z.write(file,str(file.relative_to(project)));found=True
    entries.append({'file':str(file.relative_to(project)),'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
  if not found:missing.append(p)
assert not missing,missing
rows=[]
for src,dst in sorted(selected.items()):
 obj=lib.load_asset(src);assert obj,src
 if lib.does_asset_exist(dst):raise RuntimeError('Destination exists: '+dst)
 copied=lib.duplicate_asset(src,dst);assert copied,dst
 assert lib.save_loaded_asset(copied,only_if_is_dirty=False),dst
 row={'source':src,'copy':dst,'class':obj.get_class().get_name()}
 if isinstance(obj,unreal.MaterialInstanceConstant):
  parent=obj.get_editor_property('parent');row['parent']=parent.get_path_name() if parent else None
  if isinstance(parent,unreal.Material):row['material_domain']=str(parent.get_editor_property('material_domain'))
 elif isinstance(obj,unreal.Material):row['material_domain']=str(obj.get_editor_property('material_domain'))
 rows.append(row)
report={'copied_count':len(rows),'original_dependency_packages':len(closure),'backup_files':entries,'assets':rows,'missing':missing}
(backup/'资源清单.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(project/'Saved/DecalCollectionReport.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
text='''贴花资源合集\n\nUE内容浏览器路径：/Game/场景资源/贴花资源合集\n01_现成贴花：五套落叶、碎屑、焦草材质及配套资源。\n02_泥土斑块：泥土材质实例（材质类型见资源清单）。\n03_污渍雨痕纹理：按来源整理的岩石流痕、雨痕和污渍；纹理需接入材质使用。\n\n使用：优先选择01中的材质实例；若材质域为Deferred Decal，可赋给Decal Actor并调整投射方向和大小。\n复制资源保留对原目录父材质、纹理的引用，请保留原场景资源文件夹。\n此目录为方便使用的副本，原场景未修改。\n\n贴花原始资源及依赖.zip是恢复备份，内部保持Content原路径。恢复至项目根目录；跨项目使用建议在UE中迁移合集，让UE收集依赖。\n资源清单.json包含复制路径、材质类型和备份文件SHA256。\n'''
(backup/'使用说明.txt').write_text(text,encoding='utf-8')
(project/'Content/场景资源/贴花资源合集/使用说明.txt').write_text(text,encoding='utf-8')
unreal.log('DECAL_COLLECTION_SUCCESS '+str(len(rows)))

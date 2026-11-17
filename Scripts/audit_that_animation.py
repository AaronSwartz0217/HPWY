import unreal,json,pathlib
r=unreal.AssetRegistryHelpers.get_asset_registry();r.search_all_assets(True)
opts=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
roots=['/Game/THAT/Player/GA_Primary','/Game/THAT/Player/GA_Secondary','/Game/THAT/Player/GA_Tertiary','/Game/THAT/BP_PlayerCharacter','/Game/THAT/BP_EnemyCharacter']
seen=set();queue=list(roots);items=[]
while queue:
 p=queue.pop()
 if p in seen or not p.startswith('/Game/'):continue
 seen.add(p)
 data=r.get_assets_by_package_name(p)
 if data:
  kind=str(data[0].asset_class_path.asset_name)
  if kind in ['AnimSequence','AnimMontage','AnimBlueprint','NiagaraSystem','ParticleSystem','SkeletalMesh']:
   o=data[0].get_asset(); info={'path':p,'class':kind}
   items.append(info)
 queue.extend(str(d) for d in r.get_dependencies(p,opts))
pathlib.Path(unreal.Paths.project_saved_dir()+'THATAnimationAudit.json').write_text(json.dumps(items,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('THAT_ANIMATION_AUDIT_READY')

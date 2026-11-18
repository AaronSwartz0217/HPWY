import json, shutil
from pathlib import Path
import unreal as u

project=Path(u.Paths.project_dir()).resolve()
backup=project/'_UnusedContent_HPWH_LegacyMonsters'
assetlib=u.EditorAssetLibrary
data=json.loads((project/'Saved/HPWHLegacyMonsterClosure.json').read_text(encoding='utf-8'))
manny=u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
anim=assetlib.load_blueprint_class('/Game/Characters/Mannequins/Animations/ABP_Manny')
assert manny and anim
obsolete={p for p in data if '/SOURCE/小怪/' in p or ('/Locomotion/' in p and not p.endswith('/DA_NPCLocomotion'))}
assert all('/Protagonist/' not in p and '/ZichanCrawler/' not in p and 'InitialInk' not in p for p in obsolete)
def old(obj):
    return bool(obj) and obj.get_path_name().split('.')[0] in obsolete
def preserve(path, ext='.uasset'):
    rel=Path(path[6:]+ext)
    src=project/'Content'/rel
    dest=backup/'BeforeEdit'/rel
    if src.is_file() and not dest.exists():
        dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(src,dest)
def save(path,a):
    preserve(path)
    assert assetlib.save_loaded_asset(a,False),path
def fix_mesh(owner):
    if not isinstance(owner,u.Character):return False
    comp=owner.get_editor_property('mesh')
    if not comp:return False
    model=comp.get_editor_property('skeletal_mesh_asset')
    animation=comp.get_editor_property('anim_class')
    if old(model) or old(animation):
        comp.modify()
        comp.set_editor_property('skeletal_mesh_asset',manny)
        comp.set_editor_property('anim_class',anim)
        comp.set_editor_property('animation_mode',u.AnimationMode.ANIMATION_BLUEPRINT)
        comp.set_relative_scale3d(u.Vector(1,1,1))
        comp.set_editor_property('relative_location',u.Vector(0,0,-owner.get_editor_property('capsule_component').get_unscaled_capsule_half_height()))
        comp.set_editor_property('relative_rotation',u.Rotator(yaw=-90))
        comp.set_editor_property('override_materials',[])
        return True
    return False

maps=[]
for path in data:
    if path in obsolete:continue
    if '/Maps/' in path:
        maps.append(path)
        continue
    obj=u.load_asset(path)
    assert obj,path
    if path.endswith('/DA_NPCLocomotion'):
        entries=[e for e in obj.get_editor_property('entries') if not old(e.get_editor_property('mesh')) and not old(e.get_editor_property('anim_class'))]
        obj.set_editor_property('entries',entries)
    elif '/Balance/' in path:
        profiles=[]
        for e in obj.get_editor_property('profiles'):
            if old(e.get_editor_property('mesh')):e.set_editor_property('mesh',None)
            profiles.append(e)
        obj.set_editor_property('profiles',profiles)
    elif '/Forms/' in path:
        stages=[]
        for e in obj.get_editor_property('stages'):
            if old(e.get_editor_property('mesh')) or old(e.get_editor_property('anim_class')):
                e.set_editor_property('mesh',manny)
                e.set_editor_property('anim_class',anim)
                e.set_editor_property('mesh_scale',u.Vector(1,1,1))
            stages.append(e)
        obj.set_editor_property('stages',stages)
    elif isinstance(obj,u.Blueprint):
        default=u.get_default_object(obj.generated_class())
        if not fix_mesh(default):continue
        u.BlueprintEditorLibrary.compile_blueprint(obj)
    else:
        raise RuntimeError(path)
    save(path,obj)

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for path in maps:
    preserve(path,'.umap')
    assert levels.load_level(path),path
    changed=sum(fix_mesh(a) for a in actors.get_all_level_actors())
    if changed:assert levels.save_current_level(),path
    u.log(f'HPWH_REBOUND_MAP {path} {changed}')
(project/'Saved/ObsoleteLegacyMonsterPackages.json').write_text(json.dumps(sorted(obsolete),ensure_ascii=False),encoding='utf8')
u.log('HPWH_LEGACY_REBIND_COMPLETE '+str(len(obsolete)))

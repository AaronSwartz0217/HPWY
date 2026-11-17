import unreal,json,hashlib
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve();out=project/'Saved/RoomVisibilityTrial'
level='/Game/关卡优化/测试/L_RestartRoomVisibility_Test';levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level(level)
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors=sub.get_all_level_actors();byname={a.get_name():a for a in actors}
room=next(a for a in actors if isinstance(a,unreal.CC_RoomVisibilityController))
candidates=json.loads((out/'interior_candidates.json').read_text(encoding='utf-8'));items=[byname[x['name']] for x in candidates];assert len(items)==136
room.set_editor_property('interior_actors',items)
# Save only the added room setup, with scene visibility left in its original editor state.
assert levels.save_current_level()
report=json.loads((out/'installation.json').read_text(encoding='utf-8'));report['interiors']=[a.get_actor_label() for a in items];report['target_count']=len(items);report['selection']='Named props and bounded interior detail pieces; outer walls, roof, door frames and ground excluded.'
original=project/'Content/LanTuRebuild/Maps/初始房间/restartlife.umap';assert hashlib.sha256(original.read_bytes()).hexdigest()==report['original_sha256']
report['checks'].pop('all_11_interiors_hidden',None)
room.set_room_active(False);report['checks']['all_136_interiors_hidden']=all(a.get_editor_property('hidden') for a in items)
room.set_room_active(True);report['checks']['all_136_restored']=all(not a.get_editor_property('hidden') for a in items)
assert all(report['checks'].values())
# No level save after these transient verification toggles.
(out/'installation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(Path(report['backup'])/'试装清单.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('ROOM_TRIAL_136_UPDATED_PASS')

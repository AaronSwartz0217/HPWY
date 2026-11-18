import unreal as u
a=u.load_asset('/Game/HPWH/Assets/Models/四合院模型/StaticMeshes/courtyard_model')
b=a.get_editor_property('body_setup')
flag=u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
if b.get_editor_property('collision_trace_flag')!=flag:
    b.modify()
    a.modify()
    b.set_editor_property('collision_trace_flag',flag)
subsystem=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
b.set_editor_property('double_sided_geometry',True)
for section in range(a.get_num_sections(0)):
    subsystem.enable_section_collision(a,True,0,section)
    assert subsystem.is_section_collision_enabled(a,0,section)
assert u.EditorAssetLibrary.save_loaded_asset(a,False)
assert b.get_editor_property('collision_trace_flag')==flag
for name in ['Attack','SoulOut','EnemySpawn','CrawlerAppear','CardAppear','PuzzleSolve','Footstep','Win','TileLaptop','TileMirror','TileCourtyard']:
    sound=u.load_asset('/Game/HPWH/Audio/SFX/S_HPWH_'+name)
    assert isinstance(sound,u.SoundWave) and sound.get_editor_property('duration')>0,name
for i in range(1,4):
    assert isinstance(u.load_asset('/Game/HPWH/Audio/S_HPWH_Emergency'+str(i)),u.SoundBase)
u.log('HPWH_COURTYARD_COMPLEX_COLLISION_AND_AUDIO_14_OK')

import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/HPWH/Maps/L_FlatTestArena')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if isinstance(actor,unreal.TextRenderActor) and actor.get_actor_label().startswith('TARGET DUMMIES'):
  actor.get_editor_property('text_render').set_text('TARGET DUMMIES | 10 XP')
  actor.set_actor_label('TARGET DUMMIES | 10 XP')
levels.save_current_level()
unreal.log('ARENA_LABEL_10_XP_OK')

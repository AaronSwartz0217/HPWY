import unreal,json
mesh=unreal.load_asset('/Game/SOURCE/小怪/铜机械小兵/SK_铜机械小兵')
rig=unreal.load_asset('/Game/HPWH/Locomotion/IK_铜机械小兵')
ctrl=unreal.IKRigController.get_controller(rig)
clip=unreal.load_asset('/Game/HPWH/Locomotion/Animations/A_铜机械小兵_Idle')
result={'refHips':str(ctrl.get_ref_pose_transform_of_bone('Hips')), 'refLeg':str(ctrl.get_ref_pose_transform_of_bone('LeftLeg')), 'clipHips':str(unreal.AnimationLibrary.get_bone_pose_for_time(clip,'Hips',0.0,False)), 'clipLeg':str(unreal.AnimationLibrary.get_bone_pose_for_time(clip,'LeftLeg',0.0,False)), 'clipApi':[n for n in dir(clip) if 'data' in n or 'control' in n]}
open(unreal.Paths.project_saved_dir()+'AnimationScaleInspection.json','w',encoding='utf-8').write(json.dumps(result,ensure_ascii=False,indent=2))

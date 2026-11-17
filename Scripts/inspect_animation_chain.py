import unreal,json
mesh=unreal.load_asset('/Game/SOURCE/小怪/铜机械小兵/SK_铜机械小兵')
component=unreal.new_object(unreal.SkeletalMeshComponent);component.set_skeletal_mesh_asset(mesh)
rig=unreal.load_asset('/Game/HPWH/Locomotion/IK_铜机械小兵');ctrl=unreal.IKRigController.get_controller(rig)
clip=unreal.load_asset('/Game/HPWH/Locomotion/Animations/A_铜机械小兵_Idle')
result={}
for bone in ['Hips','Spine','Spine1','Spine2','Neck','Head','LeftUpLeg','LeftLeg','LeftFoot']:
 result[bone]={'parent':str(component.get_parent_bone(bone)),'ref':str(ctrl.get_ref_pose_transform_of_bone(bone)),'pose':str(unreal.AnimationLibrary.get_bone_pose_for_time(clip,bone,0.0,False))}
open(unreal.Paths.project_saved_dir()+'AnimationChainInspection.json','w',encoding='utf-8').write(json.dumps(result,ensure_ascii=False,indent=2))

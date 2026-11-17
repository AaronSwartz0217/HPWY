#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Progression/CC_GameplayData.h"
#include "CC_LocomotionComponent.generated.h"
class USkeletalMesh;
class UAnimSequence;
class UAnimInstance;
class UCC_SkillDefinition;

UENUM()
enum class ECC_SkillPresentationPhase : uint8 { Cast, ChargeStart, ChargeRelease, AttackFollowThrough };

USTRUCT(BlueprintType)
struct FCC_MeshLocomotion
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Idle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Run;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Jump;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Fall;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Land;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Death;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Jab;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> CrossJab;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> Uppercut;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> GuardStance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimSequence> CastGesture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UAnimInstance> AnimClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReferenceSpeed = 400.f;
};
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_LocomotionLibrary : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCC_MeshLocomotion> Entries;
};
// Skeleton-specific animation blueprints and replicated skill presentation.
UCLASS(ClassGroup=(Crash), meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_LocomotionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCC_LocomotionComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCC_LocomotionLibrary> Library;
	UPROPERTY(BlueprintReadOnly) ECC_AttackPose LastAttackPose = ECC_AttackPose::Jab;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	const FCC_MeshLocomotion* GetCurrentEntry() const;
    // Static mesh calibration, never animated-bounds snapping or forcing airborne actors down.
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Grounding") bool bAlignMeshToCapsule=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Grounding",meta=(Units="cm")) float VisualGroundOffset=0.f;
    void AlignMeshToGround();
	void PresentSkill(UCC_SkillDefinition* Skill, FVector TargetPoint, ECC_SkillPresentationPhase Phase = ECC_SkillPresentationPhase::Cast);
	void PresentImpact(UCC_SkillDefinition* Skill, FVector Location);
	void CancelPresentation();
private:
    float LastGroundClearance=0.f;
	bool bNextCrossJab = false;
	UFUNCTION(NetMulticast, Unreliable) void MulticastSkill(UCC_SkillDefinition* Skill, FVector_NetQuantize TargetPoint, ECC_AttackPose Pose, ECC_SkillPresentationPhase Phase);
	UFUNCTION(NetMulticast, Unreliable) void MulticastImpact(UCC_SkillDefinition* Skill, FVector_NetQuantize Location);
	UFUNCTION(NetMulticast, Reliable) void MulticastCancel();
};

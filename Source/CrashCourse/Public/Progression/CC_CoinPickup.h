#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CC_CoinPickup.generated.h"
class USphereComponent; class USkeletalMeshComponent; class USoundBase; class ACC_PlayerCharacter;
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_CoinPickup : public ACharacter {
 GENERATED_BODY()
public:
 ACC_CoinPickup();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USphereComponent> PickupSphere;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> CoinMesh;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Reward",meta=(ClampMin="1")) int32 CopperAmount=5;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Reward",meta=(ClampMin="0")) int32 ExperienceAmount=5;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation") TObjectPtr<USoundBase> PickupSound;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Patrol") bool bStartPatrolling=true;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Patrol",meta=(ClampMin="0")) float PatrolRadius=600.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Patrol",meta=(ClampMin="1")) float PatrolSpeed=100.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Patrol",meta=(ClampMin="0")) float MinPatrolWait=1.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Patrol",meta=(ClampMin="0")) float MaxPatrolWait=3.f;
 UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Patrol") void StartPatrol();
 UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Patrol") void StopPatrol();
 bool TryCollect(ACC_PlayerCharacter* Player);
 virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 bool bCollected=false;
 bool bMoving=false;
 FVector PatrolOrigin=FVector::ZeroVector;
 float NextMoveTime=0.f;
 float MoveDeadline=0.f;
 FTimerHandle PatrolTimer;
 void UpdatePatrol();
 void WaitForNextMove();
 UFUNCTION(NetMulticast,Reliable) void PlayPickupSound(FVector Location);
};

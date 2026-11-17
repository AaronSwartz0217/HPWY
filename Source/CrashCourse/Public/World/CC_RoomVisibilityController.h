#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_RoomVisibilityController.generated.h"

class UBoxComponent;
class USceneComponent;
class ALight;

/** Single-player room visibility control. Each target should belong to one controller. */
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_RoomVisibilityController : public AActor
{
 GENERATED_BODY()
public:
 ACC_RoomVisibilityController();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Room Setup") TObjectPtr<UBoxComponent> TriggerBox;
 UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Room Setup") TArray<TObjectPtr<AActor>> InteriorActors;
 UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Room Setup") TArray<TObjectPtr<ALight>> RoomLights;
 UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Room Setup") TArray<TObjectPtr<AActor>> RoomEffects;
 UPROPERTY(EditInstanceOnly, Category="Room Setup")
 bool bUseBoxSelection=false;
 UFUNCTION(CallInEditor, Category="Room Setup")
 void CollectActorsInsideBox();
 UPROPERTY(VisibleInstanceOnly, Category="Room Setup") TArray<TObjectPtr<AActor>> BoxActors;
 UPROPERTY(VisibleInstanceOnly, Category="Room Setup") TArray<TObjectPtr<ALight>> BoxLights;
 UPROPERTY(VisibleInstanceOnly, Category="Room Setup") TArray<TObjectPtr<AActor>> BoxEffects;
 /** Enable only for decorative actors whose Tick can safely stop. Collision is never changed. */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Room Setup") bool bManageInteriorActorTick=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Room Setup") bool bAutomaticPlayerDetection=true;
 UFUNCTION(BlueprintCallable,Category="Room Control") void SetAutomaticPlayerDetection(bool bEnabled);
 UPROPERTY(BlueprintReadOnly, Transient, Category="Room State") bool bRoomActive=true;
 UFUNCTION(BlueprintCallable, Category="Room Control") void SetRoomActive(bool bActive);
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 struct FActorState { bool Hidden=false; bool Tick=false; };
 TMap<TWeakObjectPtr<AActor>, FActorState> ActorStates;
 TMap<TWeakObjectPtr<USceneComponent>, bool> LightStates;
 TMap<TWeakObjectPtr<UActorComponent>, bool> EffectStates;
 FTimerHandle PollTimer;
 bool bApplied=false;
 void RefreshPlayerOverlap();
 void ApplyActorVisibility(AActor* Target, bool bActive, bool bManageTick);
};

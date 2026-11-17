#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_DoorVisibilityManager.generated.h"
class ACC_InteractiveDoubleDoor;
class ACC_RoomVisibilityController;
USTRUCT(BlueprintType)
struct FCC_DoorVisibilityZone
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RoomId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ACC_InteractiveDoubleDoor> Door;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ACC_RoomVisibilityController> Visibility;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Center=FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector HalfExtent=FVector(500,900,400);
};
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_DoorVisibilityManager : public AActor
{
 GENERATED_BODY()
public:
 ACC_DoorVisibilityManager();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Render Zones") TArray<FCC_DoorVisibilityZone> Rooms;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Render Zones") TObjectPtr<ACC_RoomVisibilityController> ExteriorDetails;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Render Zones") bool bOptimizationEnabled=true;
 UPROPERTY(BlueprintReadOnly,Transient,Category="Render State") int32 OccupiedRoom=INDEX_NONE;
 UFUNCTION(BlueprintCallable,Category="Render Zones") void SetOptimizationEnabled(bool bEnabled);
 UFUNCTION(BlueprintCallable,Category="Render Zones") void RefreshVisibility();
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UFUNCTION() void OnDoorStateChanged();
 void ShowAll();
 FTimerHandle RefreshTimer;
};

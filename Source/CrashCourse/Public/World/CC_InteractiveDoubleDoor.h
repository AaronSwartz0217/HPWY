#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_InteractiveDoubleDoor.generated.h"
class USceneComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCC_DoorStateEvent);
/** Reusable parent for artist-placed doors; all initial component transforms are retained. */
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_InteractiveDoubleDoor : public AActor
{
 GENERATED_BODY()
public:
 ACC_InteractiveDoubleDoor();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup") FName LeftDoorComponentName=TEXT("2_017");
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup") FName RightDoorComponentName=TEXT("2_018");
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup") float LeftOpenAngle=90.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup") float RightOpenAngle=-90.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup",meta=(ClampMin="0.05")) float OpenDuration=0.8f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door Setup",meta=(ClampMin="50")) float InteractionDistance=350.f;
 UPROPERTY(BlueprintReadOnly,Transient,Category="Door State") bool bOpen=false;
 UPROPERTY(BlueprintReadOnly,Transient,Category="Door State") bool bAnimating=false;
 UPROPERTY(BlueprintAssignable,Category="Door Events") FCC_DoorStateEvent OnDoorOpening;
 UPROPERTY(BlueprintAssignable,Category="Door Events") FCC_DoorStateEvent OnDoorClosed;
 UFUNCTION(BlueprintPure,Category="Door Interaction") bool CanInteract(const APawn* Player) const;
 UFUNCTION(BlueprintCallable,Category="Door Interaction") bool Interact(APawn* Player);
 virtual void Tick(float DeltaSeconds) override;
protected:
 virtual void BeginPlay() override;
private:
 UPROPERTY(Transient) TObjectPtr<USceneComponent> LeftDoor;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> RightDoor;
 FRotator LeftClosed,RightClosed;
 float Alpha=0.f;
 void ApplyDoorRotation();
};

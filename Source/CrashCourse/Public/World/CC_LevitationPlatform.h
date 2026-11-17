#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_LevitationPlatform.generated.h"
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class ACC_PlayerCharacter;

/** A physical moving base: passengers remain normal grounded characters. */
UCLASS()
class CRASHCOURSE_API ACC_LevitationPlatform:public AActor
{
    GENERATED_BODY()
public:
    ACC_LevitationPlatform();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure,Category="Floating Platform") bool IsCarrying(const ACC_PlayerCharacter* Character) const;
    UFUNCTION(BlueprintPure,Category="Floating Platform") bool CanInteract(const ACC_PlayerCharacter* Character) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Floating Platform") bool Reassemble(ACC_PlayerCharacter* Character);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<USceneComponent> LiftRoot;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<UStaticMeshComponent> Deck;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<UInstancedStaticMeshComponent> Tiles;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<UStaticMeshComponent> Console;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<UBoxComponent> Field;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Floating Platform") TObjectPtr<UInstancedStaticMeshComponent> Edges;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="0")) float LiftHeight=600;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="0.2",ClampMax="2")) float FootprintScale=.55f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="1")) float LiftSpeed=180;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="0")) float EmptyReturnDelay=1.5f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="1")) float InteractionDistance=350;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="0.1")) float RebuildDuration=1.25f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Floating Platform",meta=(ClampMin="0.1")) float RebuildCooldown=2.f;
    UPROPERTY(EditAnywhere,ReplicatedUsing=OnRep_Layout,BlueprintReadOnly,Category="Floating Platform") int32 LayoutSeed=20261028;
    UPROPERTY(VisibleInstanceOnly,ReplicatedUsing=OnRep_Lift,BlueprintReadOnly,Category="Floating Platform") float CurrentLift=0;
private:
    TArray<FTransform> Targets,Previous;
    float RebuildStarted=-1000.f;
    float RebuildElapsed=0.f;
    float EmptyTime=1000.f;
    bool bAnimating=false;
    void BuildLayout(bool bAnimate);
    UFUNCTION() void OnRep_Layout();
    UFUNCTION() void OnRep_Lift();
};

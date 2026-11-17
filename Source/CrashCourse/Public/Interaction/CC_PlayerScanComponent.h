#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CC_PlayerScanComponent.generated.h"
class UCC_ScanCatalog;
class UCC_ScanDescription;
class ACC_EnemyCharacter;
UCLASS(ClassGroup=(Crash),meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_PlayerScanComponent:public UActorComponent
{
 GENERATED_BODY()
public:
 UCC_PlayerScanComponent();
 virtual void BeginPlay()override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)override;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan") TObjectPtr<UCC_ScanCatalog> Catalog;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Scan",meta=(ClampMin="1")) float ScanDistance=600.f;
 UFUNCTION(BlueprintPure,Category="Scan") bool IsScanActive()const;
 UFUNCTION(BlueprintCallable,Category="Scan") void ScanNow();
 bool TraceCrosshair(FHitResult& Hit)const;
 UCC_ScanDescription* ResolveInfoForActor(AActor* Actor)const;
 UFUNCTION(BlueprintPure,Category="Scan") AActor* GetTarget()const;
 UFUNCTION(BlueprintPure,Category="Scan") UCC_ScanDescription* GetInfo()const;
 UFUNCTION(BlueprintPure,Category="Scan") bool CanInteractWithTarget()const;
 UFUNCTION(BlueprintPure,Category="Scan") bool HasInteraction()const;
 UFUNCTION(BlueprintPure,Category="Scan") float GetTargetDistance()const{return TargetDistance;}
private:
 void UpdateEnemyHealthDisplays(bool bScanActive);
 TSet<TWeakObjectPtr<ACC_EnemyCharacter>> VisibleEnemyHealth;
 TWeakObjectPtr<AActor> Target;
 UPROPERTY(Transient) TObjectPtr<UCC_ScanDescription> Info;
 float TargetDistance=0;
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_CoinSpawnDirector.generated.h"
class ACC_CoinPickup;
UCLASS()
class CRASHCOURSE_API ACC_CoinSpawnDirector:public AActor
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category="Spawning") int32 PopulationCap=6;
 UPROPERTY(EditAnywhere,Category="Spawning") float SpawnInterval=12.f;
 UPROPERTY(EditAnywhere,Category="Spawning") TSoftClassPtr<ACC_CoinPickup> CoinClass{FSoftObjectPath(TEXT("/Game/HPWH/Coins/BP_CopperCoin.BP_CopperCoin_C"))};
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 FTimerHandle SpawnTimer;
 TArray<TWeakObjectPtr<ACC_CoinPickup>> Spawned;
 void UpdatePopulation();
};

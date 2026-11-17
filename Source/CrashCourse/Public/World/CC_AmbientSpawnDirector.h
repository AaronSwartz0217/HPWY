#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_AmbientSpawnDirector.generated.h"

class ACC_PlayerCharacter;
class ACC_PatrolEnemyCharacter;

USTRUCT(BlueprintType)
struct FCC_AmbientSpawnEntry
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<ACC_PatrolEnemyCharacter> EnemyClass;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0")) float Weight=1.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0",ClampMax="2")) int32 MinimumDifficultyTier=0;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="1")) int32 ClassLimit=12;
};

// Server-only ambient population, owned by the current map. Despawn never calls death/reward.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_AmbientSpawnDirector : public AActor
{
 GENERATED_BODY()
public:
 ACC_AmbientSpawnDirector();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning") bool bEnabled=true;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning") TArray<FCC_AmbientSpawnEntry> Entries;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning") bool bReplacePlacedPatrolEnemies=true;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="1")) int32 LocalPopulationCap=12;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="1")) int32 GlobalPopulationCap=18;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="1")) int32 MaxSpawnsPerCycle=3;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="1",Units="s")) float SpawnInterval=6.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="0",Units="s")) float InitialDelay=4.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float MinimumPlayerDistance=1800.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float MaximumSpawnDistance=3600.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float LocalPopulationRadius=4200.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float StartProtectionRadius=1800.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float MinimumEnemySpacing=350.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float SoftDespawnDistance=4500.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="cm")) float HardDespawnDistance=6500.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(Units="s")) float SoftDespawnDelay=30.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning") bool bAvoidVisibleSpawns=true;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Spawning",meta=(ClampMin="1")) int32 AttemptsPerCycle=24;
 UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Spawning") void RunSpawnCycle();
 void RefreshPopulationBounds();
 UFUNCTION(BlueprintPure,Category="Spawning") int32 GetManagedPopulation() const;
 UFUNCTION(BlueprintPure,Category="Spawning") int32 GetTotalSpawned() const {return TotalSpawned;}
 UFUNCTION(BlueprintPure,Category="Spawning") int32 GetTotalDespawned() const {return TotalDespawned;}
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 friend class FCC_AmbientSpawnTest;
 FTimerHandle SpawnTimer;
 TArray<TWeakObjectPtr<ACC_PatrolEnemyCharacter>> Managed;
 TMap<TWeakObjectPtr<ACC_PatrolEnemyCharacter>,float> OutsideSince;
 TArray<FVector> ProtectedStarts;
 int32 TotalSpawned=0,TotalDespawned=0;
 void GatherPlayers(TArray<ACC_PlayerCharacter*>& OutPlayers) const;
 void PrunePopulation(const TArray<ACC_PlayerCharacter*>& Players);
 bool IsVisibleToAnyPlayer(FVector Position,const TArray<ACC_PlayerCharacter*>& Players,const AActor* Subject=nullptr) const;
 bool IsPositionAllowed(FVector Position,const TArray<ACC_PlayerCharacter*>& Players) const;
 bool FindSpawnPosition(ACC_PlayerCharacter* Around,const TArray<ACC_PlayerCharacter*>& Players,FVector& OutPosition,const FVector* PackAnchor=nullptr) const;
 int32 CountNearby(FVector Position) const;
 int32 HighestOwnedTier(const TArray<ACC_PlayerCharacter*>& Players) const;
 const FCC_AmbientSpawnEntry* PickEntry(int32 Tier) const;
};

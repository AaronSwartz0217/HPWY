#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CC_ShanheChunkWorld.generated.h"

class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ANavMeshBoundsVolume;

UCLASS()
class CRASHCOURSE_API ACC_ShanheTerrainChunk : public AActor
{
 GENERATED_BODY()
public:
 ACC_ShanheTerrainChunk();
 FIntPoint Coordinates=FIntPoint::ZeroValue;
 void Generate(FIntPoint Key,int32 Seed,float Width,float GroundZ,UMaterialInterface* GroundMaterial,UMaterialInterface* BambooMaterial,UMaterialInterface* LeafMaterial,int32 Groves,int32 Rocks);
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ground;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockInstances;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BambooInstances;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LeafInstances;
};

// A single, deterministic 3x3 terrain window for the Shanhe single-player region.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_ShanheChunkWorld : public AActor
{
 GENERATED_BODY()
public:
 ACC_ShanheChunkWorld();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Replicated,Category="Chunks") int32 WorldSeed=29717;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks",meta=(ClampMin="4000",Units="cm")) float ChunkSize=8000.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks",meta=(Units="cm")) float GroundHeight=0.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks") TArray<TObjectPtr<UMaterialInterface>> GroundMaterials;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks") TObjectPtr<UMaterialInterface> BambooMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks") TObjectPtr<UMaterialInterface> LeafMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks",meta=(ClampMin="0",ClampMax="30")) int32 GrovesPerChunk=10;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Chunks",meta=(ClampMin="0",ClampMax="40")) int32 RocksPerChunk=12;
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_Center,Category="Chunks") FIntPoint CenterCoordinates=FIntPoint::ZeroValue;
 UFUNCTION(BlueprintPure,Category="Chunks") FIntPoint CoordinatesForPosition(FVector Position) const;
 UFUNCTION(BlueprintPure,Category="Chunks") int32 DifficultyRingForPosition(FVector Position) const;
 UFUNCTION(BlueprintPure,Category="Chunks") bool IsPositionLoaded(FVector Position) const;
 UFUNCTION(BlueprintPure,Category="Chunks") int32 GetLoadedChunkCount() const {return Chunks.Num();}
 UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Chunks") void RefreshForPlayer();
 static int32 SeedForCoordinates(FIntPoint Key,int32 Seed);
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 friend class FCC_ShanheChunksTest;
 UPROPERTY(Transient) TMap<FIntPoint,TObjectPtr<ACC_ShanheTerrainChunk>> Chunks;
 TWeakObjectPtr<ANavMeshBoundsVolume> NavBounds;
 FTimerHandle UpdateTimer;
 UFUNCTION() void OnRep_Center();
 void SynchronizeChunks();
 void MoveNavigationBounds();
};

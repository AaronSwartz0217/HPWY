#pragma once
#include "GameFramework/Actor.h"
#include "WFCTileSet.h"
#include "WFCMapGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWFCGenerationFinished, const FWFCGenerationResult&, Result);

/** Independent opt-in map actor. It only owns the components it generates. */
UCLASS(Blueprintable)
class SPIRITWFC_API AWFCMapGenerator : public AActor
{
	GENERATED_BODY()
  public:
 AWFCMapGenerator();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="WFC|Runtime") bool RegenerateRandomScene();
 bool RegenerateScenePreset(EWFCLayoutPreset Preset);
 UFUNCTION(BlueprintPure,Category="WFC|Runtime") bool CanRegenerateRandomScene()const;
 UPROPERTY(Replicated,VisibleInstanceOnly,BlueprintReadOnly,Category="WFC|Runtime") int32 RuntimeSeed=0;
 UPROPERTY(Replicated,VisibleInstanceOnly,BlueprintReadOnly,Category="WFC|Runtime") EWFCLayoutPreset RuntimeLayout = EWFCLayoutPreset::Settlement;
 UPROPERTY(ReplicatedUsing=OnRep_RuntimeGeneration,VisibleInstanceOnly,BlueprintReadOnly,Category="WFC|Runtime") int32 RuntimeRevision=0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") TObjectPtr<UWFCTileSet> TileSet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") FWFCGenerationSettings Settings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Rendering") bool bEnableCollision = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Rendering") bool bAffectNavigation = true;
	/** Disabled by default: editor preview/saved levels do not regenerate on play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC") bool bGenerateOnBeginPlay = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WFC") FWFCGenerationResult LastResult;
	/** Last successfully rendered solution, retained when a later attempt fails. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WFC") FWFCGenerationResult DisplayedResult;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WFC") TObjectPtr<UWFCTileSet> DisplayedTileSet;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WFC") FWFCGenerationSettings DisplayedSettings;
	UPROPERTY(BlueprintAssignable, Category = "WFC") FWFCGenerationFinished OnGenerationFinished;

	/** Solve, validate, stage instance components, then replace the old map. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "WFC") bool GenerateMap();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "WFC") void ClearGeneratedMap();
	UFUNCTION(BlueprintPure, Category = "WFC") int32 GetGeneratedInstanceCount() const;

  protected:
	virtual void BeginPlay() override;

  private:
 float LastRuntimeGeneration=-1000.f;
 UFUNCTION() void OnRep_RuntimeGeneration();
	UPROPERTY(VisibleAnywhere, Category = "WFC")
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeneratedComponents;
};

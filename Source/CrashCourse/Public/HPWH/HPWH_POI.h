#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/Actor.h"
#include "HPWH/HPWH_Types.h"
#include "HPWH_POI.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;
class AHPWH_WorldDirector;
class AHPWH_PuzzleTile;
class AHPWH_CoreItem;
class AHPWH_PlayerCharacter;
class AHPWH_LaptopPOI;
class AHPWH_CatMeme;

UINTERFACE(MinimalAPI)
class UHPWH_Interactable : public UInterface
{
	GENERATED_BODY()
};

class CRASHCOURSE_API IHPWH_Interactable
{
	GENERATED_BODY()

public:
	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const { return TEXT("交互"); }
	virtual float GetHPWHInteractRange() const { return 320.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const { return true; }
	virtual void HPWHInteract(APawn* Interactor) {}
};

/** 关键解谜器物公共实现：外部交互 → 踩方块谜题 → 生门灯芯。 */
UCLASS()
class CRASHCOURSE_API AHPWH_POIBase : public AActor, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_POIBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializePOI(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell, bool bAlreadySolved = false);
	virtual void ResetPuzzle();

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;
	virtual float GetHPWHInteractRange() const override { return 360.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "HPWH|POI") bool IsPuzzleSolved() const { return bSolved; }
	UFUNCTION(BlueprintPure, Category = "HPWH|POI") bool IsPuzzleRunning() const { return bPuzzleActive && !bSolved; }
	UFUNCTION(BlueprintPure, Category = "HPWH|POI") float GetPuzzleTimeLimit() const;
	UFUNCTION(BlueprintPure, Category = "HPWH|POI") float GetPuzzleRemaining() const;

protected:
	virtual void BuildVisuals();
	/** 用导入的静态模型替换程序化外观；原网格保留碰撞但隐藏。 */
	UStaticMeshComponent* AddImportedMesh(const TCHAR* MeshPath, const FVector& Scale,
		const FVector& Offset = FVector::ZeroVector, const FRotator& Rotation = FRotator::ZeroRotator);
	virtual FVector GetCoreSpawnLocation() const;
	virtual void StartPuzzle();
	virtual void FailPuzzle();
	void SolvePuzzle();
	void ResetTiles();
	void SpawnCoreItem();

	/** 三类解密器物的差异参数。 */
	virtual bool UsesTilePuzzle() const { return true; }
	virtual int32 GetPuzzleTileCount() const { return 3; }
	virtual bool IsReverseTileOrder() const { return false; }
	virtual float GetPuzzleBaseTime() const { return 15.f; }
	virtual EHPWH_Sfx GetTileSfx() const { return EHPWH_Sfx::TileMirror; }
	virtual FString GetStartHint() const;
	virtual FString GetTimeoutMessage() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BuiltMeshes;

	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
	UPROPERTY() FIntPoint Cell = FIntPoint::ZeroValue;
	UPROPERTY() TArray<TObjectPtr<AHPWH_PuzzleTile>> Tiles;
	UPROPERTY() TArray<bool> TileWasInside;
	UPROPERTY() TObjectPtr<AHPWH_CoreItem> Core;

	UPROPERTY() bool bPuzzleActive = false;
	UPROPERTY() bool bSolved = false;
	UPROPERTY() int32 StepIndex = 0;
	float StartTime = 0.f;
};

UCLASS()
class CRASHCOURSE_API AHPWH_MirrorPOI : public AHPWH_POIBase
{
	GENERATED_BODY()

public:
	AHPWH_MirrorPOI();
	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;

protected:
	virtual void BuildVisuals() override;
	virtual void StartPuzzle() override;
	virtual bool IsReverseTileOrder() const override { return true; }
	virtual int32 GetPuzzleTileCount() const override { return 4; }
	virtual float GetPuzzleBaseTime() const override { return 15.f; }
	virtual FString GetStartHint() const override;
};

UCLASS()
class CRASHCOURSE_API AHPWH_CourtyardPOI : public AHPWH_POIBase
{
	GENERATED_BODY()

public:
	AHPWH_CourtyardPOI();
	virtual void Tick(float DeltaSeconds) override;
	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;
	virtual float GetHPWHInteractRange() const override { return 680.f; }

protected:
	virtual void BuildVisuals() override;
	virtual void StartPuzzle() override;
	virtual void FailPuzzle() override;
	virtual FVector GetCoreSpawnLocation() const override;
	virtual int32 GetPuzzleTileCount() const override { return 0; }
	virtual float GetPuzzleBaseTime() const override { return 120.f; }
	virtual EHPWH_Sfx GetTileSfx() const override { return EHPWH_Sfx::TileCourtyard; }
	virtual FString GetStartHint() const override;
};

/** 笔记本电脑解密：附近两格内找回三只猫 meme。 */
UCLASS()
class CRASHCOURSE_API AHPWH_LaptopPOI : public AHPWH_POIBase
{
	GENERATED_BODY()

public:
	AHPWH_LaptopPOI();

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;
	virtual void ResetPuzzle() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void CollectCat(AHPWH_CatMeme* Cat);

protected:
	virtual void BuildVisuals() override;
	virtual void StartPuzzle() override;
	virtual void FailPuzzle() override;
	virtual bool UsesTilePuzzle() const override { return false; }
	virtual float GetPuzzleBaseTime() const override { return 45.f; }
	virtual FString GetStartHint() const override;

private:
	void DestroyCats();

	UPROPERTY() TArray<TObjectPtr<AHPWH_CatMeme>> CatMemes;
	int32 CollectedCats = 0;
};

/** 猫 meme 收集物，交互后计入笔记本电脑谜题。 */
UCLASS()
class CRASHCOURSE_API AHPWH_CatMeme : public AActor, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_CatMeme();

	void InitializeCat(AHPWH_LaptopPOI* InOwner, int32 InIndex);

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override { return TEXT("捡起猫猫"); }
	virtual float GetHPWHInteractRange() const override { return 240.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CatBody;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EarLeft;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> EarRight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> CatGlow;

private:
	UPROPERTY() TObjectPtr<AHPWH_LaptopPOI> OwnerPOI;
	bool bCollected = false;
};

/** 三种解密完成后在玩家附近生成的异层传送门。 */
UCLASS()
class CRASHCOURSE_API AHPWH_NextFloorPortal : public AActor, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_NextFloorPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitializePortal(AHPWH_WorldDirector* InDirector);

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override { return TEXT("进入传送门"); }
	virtual float GetHPWHInteractRange() const override { return 320.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PortalRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PortalDisc;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PortalRing;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> PortalLight;

private:
	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
};

UCLASS()
class CRASHCOURSE_API AHPWH_PuzzleTile : public AActor
{
	GENERATED_BODY()

public:
	AHPWH_PuzzleTile();

	void Configure(int32 InIndex, AHPWH_POIBase* InOwner, const FVector& WorldLocation);
	void SetLit(bool bInLit);
	int32 GetTileIndex() const { return TileIndex; }
	bool IsPlayerInside(const FVector& PlayerLocation) const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Plate;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;

private:
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PlateMaterial;
	UPROPERTY() TObjectPtr<AHPWH_POIBase> OwnerPOI;
	int32 TileIndex = 0;
	bool bLit = false;
};

UCLASS()
class CRASHCOURSE_API AHPWH_CoreItem : public AActor, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_CoreItem();
	virtual void Tick(float DeltaSeconds) override;

	void InitializeCore(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell);

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override { return TEXT("拾取生门灯芯"); }
	virtual float GetHPWHInteractRange() const override { return 260.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CoreMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> CoreLight;

private:
	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
	FIntPoint Cell = FIntPoint::ZeroValue;
};

UCLASS()
class CRASHCOURSE_API AHPWH_ExitGate : public AActor, public IHPWH_Interactable
{
	GENERATED_BODY()

public:
	AHPWH_ExitGate();

	void InitializePOI(AHPWH_WorldDirector* InDirector, const FIntPoint& InCell);

	virtual FString GetHPWHInteractPrompt(APawn* Interactor) const override;
	virtual float GetHPWHInteractRange() const override { return 420.f; }
	virtual bool CanHPWHInteract(APawn* Interactor) const override;
	virtual void HPWHInteract(APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DoorGlow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> DoorLight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BuiltMeshes;

private:
	UPROPERTY() TObjectPtr<AHPWH_WorldDirector> Director;
};

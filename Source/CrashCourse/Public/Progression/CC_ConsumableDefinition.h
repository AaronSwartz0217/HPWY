#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CC_ConsumableDefinition.generated.h"

UENUM(BlueprintType)
enum class ECC_ConsumableEffect : uint8 { Experience,CooldownHaste,ResetCooldowns,TravelScroll };
UENUM(BlueprintType)
enum class ECC_ItemResult : uint8 { Success,NotEnoughCoins,OutOfStock,OnCooldown,InvalidState,InvalidItem,StackFull,AlreadyFull };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_ItemActionFinished,ECC_ItemResult,Result);

UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_ConsumableDefinition : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ItemId;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Description;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<class UTexture2D> Icon;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<class UMaterialInterface> IconMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName DestinationRegion;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) ECC_ConsumableEffect Effect=ECC_ConsumableEffect::Experience;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="1")) int32 CopperPrice=5;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="1",ClampMax="999")) int32 MaxStack=99;
 // Experience: integer amount. CooldownHaste: cooldown progression multiplier.
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0")) float Magnitude=50.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0.1")) float Duration=8.f;
 bool IsValidItem() const;
};

USTRUCT(BlueprintType)
struct FCC_ConsumableStack
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) TObjectPtr<UCC_ConsumableDefinition> Item;
 UPROPERTY(BlueprintReadOnly) int32 Count=0;
};


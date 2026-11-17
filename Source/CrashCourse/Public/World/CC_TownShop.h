#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "CC_TownShop.generated.h"
class UBoxComponent;class UWidgetComponent;class UTextBlock;class UCC_ConsumableDefinition;class UCC_UpgradeDefinition;class ACC_PlayerCharacter;
UENUM(BlueprintType)
enum class ECC_TownShopId:uint8 {None,Inn,Bronze,Ceramic,Bamboo,Books};

namespace CCTownShops
{
 CRASHCOURSE_API bool IsShop(ECC_TownShopId Id);
 CRASHCOURSE_API FText Name(ECC_TownShopId Id);
 CRASHCOURSE_API FText Description(ECC_TownShopId Id);
 CRASHCOURSE_API ECC_TownShopId ForItem(const UCC_ConsumableDefinition* Item);
 CRASHCOURSE_API ECC_TownShopId ForUpgrade(const UCC_UpgradeDefinition* Upgrade);
}
UCLASS()
class CRASHCOURSE_API UCC_TownShopSign:public UUserWidget
{
 GENERATED_BODY()
public:
 void SetShop(class ACC_TownShop* Shop);
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
private:
 TWeakObjectPtr<class ACC_TownShop> BoundShop;
 UPROPERTY() TObjectPtr<UTextBlock> Label;
};
// A small doorway/porch volume, independent of the imported building mesh.
UCLASS(Blueprintable)
class CRASHCOURSE_API ACC_TownShop:public AActor
{
 GENERATED_BODY()
public:
 ACC_TownShop();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Shop") ECC_TownShopId ShopId=ECC_TownShopId::Inn;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Shop") bool bEnabled=true;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UBoxComponent> InteractionZone;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UWidgetComponent> Sign;
 UFUNCTION(BlueprintPure,Category="Shop") bool CanInteract(const ACC_PlayerCharacter* Player) const;
 UFUNCTION(BlueprintPure,Category="Shop") FText GetShopName() const{return CCTownShops::Name(ShopId);}
protected:
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
};

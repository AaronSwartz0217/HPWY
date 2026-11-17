#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Framework/Commands/InputChord.h"
#include "Progression/CC_ProgressionComponent.h"
#include "CC_MuralMenuWidget.generated.h"
class UCC_MenuArtData;
class UCC_SkillGalleryWidget;
class UCC_SkillComponent;
class UCanvasPanel;
class UVerticalBox;
class UTextBlock;
class UButton;
class UImage;
class UCC_OptionsWidget;
class UTexture2D;
class UScrollBox;
class UBorder;
class UUniformGridPanel;
class UCC_PowerTokenWidget;
UENUM(BlueprintType)
enum class ECC_MuralMenuPage : uint8 { Inventory, Skills, Trade, Settings, Overview };
UCLASS()
class CRASHCOURSE_API UCC_MuralMenuWidget : public UUserWidget {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Menu|Shops") void OpenTownShop(ECC_TownShopId Shop);
 UFUNCTION(BlueprintPure,Category="Menu|Shops") ECC_TownShopId GetBrowsingShop() const{return SelectedTownShop;}
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Menu") TObjectPtr<UCC_MenuArtData> ArtData;
 UFUNCTION(BlueprintCallable, Category="Menu") void ShowPage(ECC_MuralMenuPage InPage);
 UFUNCTION(BlueprintCallable, Category="Menu") void SelectSkillSlot(int32 Index);
 UFUNCTION(BlueprintPure, Category="Menu") UCC_SkillGalleryWidget* GetSkillGallery() const { return SkillGalleryWidget; }
 UFUNCTION(BlueprintPure, Category="Menu") UCC_OptionsWidget* GetOptionsPanel() const { return OptionsPanel; }
 UFUNCTION(BlueprintCallable,Category="Menu|Inventory") void SelectBagForm(UCC_FormDefinition* Form);
 UFUNCTION(BlueprintPure,Category="Menu|Inventory") UCC_FormDefinition* GetSelectedBagForm() const {return SelectedBagForm;}
 UFUNCTION(BlueprintPure, Category="Menu") UTexture2D* GetDisplayedPortraitTexture() const { return DisplayedPortrait; }
 UFUNCTION(BlueprintPure, Category="Menu") UTexture2D* GetDisplayedTitleTexture() const { return DisplayedTitle; }
 UFUNCTION(BlueprintPure, Category="Menu") FName GetDisplayedFormId() const { return DisplayedFormId; }
 UFUNCTION(BlueprintPure, Category="Menu") int32 GetDisplayedTier() const { return DisplayedTier; }
 UFUNCTION(BlueprintPure, Category="Menu") ECC_MuralMenuPage GetMenuPage() const { return Page; }
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeConstruct() override;
 virtual void NativeDestruct() override;
 virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
 UPROPERTY() TObjectPtr<UComboBoxString> TownShopChoice;
 UPROPERTY() TObjectPtr<UTextBlock> TownShopDetails;
 UPROPERTY() TObjectPtr<UTextBlock> ShopGoodsHeading;
 UPROPERTY() TObjectPtr<UTextBlock> ShopUpgradeHeading;
 UPROPERTY() TObjectPtr<UTextBlock> RemoteShopHint;
 UPROPERTY() TObjectPtr<UButton> RemoteShopShortcut;
 UPROPERTY() TObjectPtr<UButton> RemoteShopPurchaseButton;
 UPROPERTY() TObjectPtr<UButton> InnRestButton;
 UPROPERTY() TObjectPtr<UVerticalBox> GrowthShopSection;
 UPROPERTY() TArray<TObjectPtr<UCC_UpgradeDefinition>> ShopUpgradeOptions;
 ECC_TownShopId SelectedTownShop=ECC_TownShopId::None;
 bool bRemoteBrowsing=false;
 void RefreshTownShopContext();
 void PopulateShopUpgrades();
 UCC_UpgradeDefinition* GetSelectedUpgrade() const;
 UFUNCTION() void TownShopChanged(FString Item,ESelectInfo::Type Type);
 UFUNCTION() void OpenRemoteShop();
 UFUNCTION() void BuyRemoteShop();
 UFUNCTION() void RestAtInn();
 UPROPERTY() TObjectPtr<UComboBoxString> ItemCategoryChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> ShopItemChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> BagItemChoice;
 UPROPERTY() TObjectPtr<UTextBlock> ShopItemDetails;
 UPROPERTY() TObjectPtr<UTextBlock> BagItemDetails;
 UPROPERTY() TObjectPtr<UImage> ShopItemIcon;
 UPROPERTY() TObjectPtr<UImage> BagItemIcon;
 UFUNCTION() UWidget* GenerateItemChoice(FString Item);
 UPROPERTY() TObjectPtr<UButton> BuyItemButton;
 UPROPERTY() TObjectPtr<UButton> EquipShopItemButton;
 UPROPERTY() TObjectPtr<UButton> EquipBagItemButton;
 UPROPERTY() TArray<TObjectPtr<UCC_ConsumableDefinition>> ShopItemOptions;
 UPROPERTY() TArray<TObjectPtr<UCC_ConsumableDefinition>> BagItemOptions;
 bool bUpdatingItems=false;
 FString PendingItemAction;
 void BuildItemShop();
 void PopulateItems();
 void RefreshItems();
 UCC_ConsumableDefinition* SelectedItem(bool bShop) const;
 UFUNCTION() void ItemCategoryChanged(FString Item,ESelectInfo::Type Type);
 UFUNCTION() void BuySelectedItem();
 UFUNCTION() void EquipShopItem();
 UFUNCTION() void EquipBagItem();
 UFUNCTION() void ItemResult(ECC_ItemResult Result);
 UPROPERTY() TObjectPtr<UCC_ProgressionComponent> Progression;
 UPROPERTY() TObjectPtr<UCC_SkillComponent> Skills;
 UPROPERTY() TObjectPtr<UTexture2D> DisplayedPortrait;
 UPROPERTY() TObjectPtr<UTexture2D> DisplayedTitle;
 UPROPERTY() TObjectPtr<UCC_SkillGalleryWidget> SkillGalleryWidget;
 UPROPERTY() TObjectPtr<UImage> LogoImage;
 UPROPERTY() TObjectPtr<UImage> PortraitImage;
 UPROPERTY() TObjectPtr<UImage> TitleImage;
 UPROPERTY() TObjectPtr<UTextBlock> ShapeCaption;
 UPROPERTY() TObjectPtr<UTextBlock> PageTitle;
 UPROPERTY() TObjectPtr<UTextBlock> PageSubtitle;
 UPROPERTY() TObjectPtr<UTextBlock> WalletText;
 UPROPERTY() TObjectPtr<UTextBlock> Details;
 UPROPERTY() TObjectPtr<UTextBlock> Feedback;
 UPROPERTY() TObjectPtr<UScrollBox> Scroll;
 UPROPERTY() TArray<TObjectPtr<UButton>> NavigationButtons;
 UPROPERTY() TArray<TObjectPtr<UVerticalBox>> PageBoxes;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotTexts;
 UPROPERTY() TArray<TObjectPtr<UButton>> SlotButtons;
 UPROPERTY() TObjectPtr<UComboBoxString> SkillChoice;
 UPROPERTY() TObjectPtr<UUniformGridPanel> BagGrid;
 UPROPERTY() TArray<TObjectPtr<UCC_PowerTokenWidget>> BagCells;
 UPROPERTY() TObjectPtr<UCC_FormDefinition> SelectedBagForm;
 UPROPERTY() TObjectPtr<UTextBlock> BagSelectionTitle;
 UPROPERTY() TObjectPtr<UTextBlock> BagDetails;
 UPROPERTY() TObjectPtr<UButton> BagUseButton;
 UPROPERTY() TObjectPtr<UButton> BagEvolveButton;
 UPROPERTY() TObjectPtr<UBorder> ContentPanel;
 UPROPERTY() TObjectPtr<UButton> OverviewButton;
 UPROPERTY() TArray<TObjectPtr<UCC_FormDefinition>> BagOptions;
 UPROPERTY() TObjectPtr<UComboBoxString> RouteChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> UpgradeChoice;
 UPROPERTY() TObjectPtr<UCC_OptionsWidget> OptionsPanel;
 UPROPERTY() TObjectPtr<UButton> EquipButton;
 UPROPERTY() TObjectPtr<UButton> ResetSkillsButton;
 UPROPERTY() TObjectPtr<UButton> SwitchButton;
 UPROPERTY() TObjectPtr<UButton> EvolveButton;
 UPROPERTY() TObjectPtr<UButton> PurchaseButton;
 UPROPERTY() TArray<TObjectPtr<UCC_FormDefinition>> RouteOptions;
 ECC_MuralMenuPage Page=ECC_MuralMenuPage::Overview;
 FName DisplayedFormId;
 int32 DisplayedTier=INDEX_NONE;
 int32 SelectedSlot=0;
 float RefreshElapsed=0;
 bool bListsInitialized=false;
 FString PendingPurchase;
 UTextBlock* AddText(UVerticalBox* Box,const FString& Text,int32 Size=20);
 UButton* AddAction(UVerticalBox* Box,const FString& Text);
 UComboBoxString* AddChoice(UVerticalBox* Box);
 void BindSources();
 void ReleaseSources();
 void PopulateLists();
 void RefreshBag();
 void RefreshData();
 void RefreshArtwork();
 void RefreshNavigation();
 void SetFeedback(const FString& Message,bool bError=false);
 UCC_FormDefinition* GetSelectedRoute() const;
 UCC_SkillDefinition* GetSelectedSkill() const;
 UFUNCTION() UWidget* GenerateChoice(FString Item);
 UFUNCTION() void SourceChanged();
 UFUNCTION() void Inventory();
 UFUNCTION() void Overview();
 UFUNCTION() void UseBagPower();
 UFUNCTION() void EvolveBagPower();
 UFUNCTION() void SkillsPage();
 UFUNCTION() void Trade();
 UFUNCTION() void Settings();
 UFUNCTION() void Close();
 UFUNCTION() void SlotOne();
 UFUNCTION() void SlotTwo();
 UFUNCTION() void SlotThree();
 UFUNCTION() void SlotFour();
 UFUNCTION() void Equip();
 UFUNCTION() void ResetSkills();
 UFUNCTION() void SwitchRoute();
 UFUNCTION() void Evolve();
 UFUNCTION() void Purchase();
 UFUNCTION() void ChoiceChanged(FString Item,ESelectInfo::Type Type);
 UFUNCTION() void PurchaseResult(ECC_PurchaseResult Result);
 UFUNCTION() void SkillResult(ECC_SkillConfigurationResult Result);
};

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Progression/CC_ProgressionComponent.h"
#include "CC_SkillGalleryWidget.generated.h"
class UCC_SkillGalleryData;
class UCanvasPanel;
class UImage;
class UButton;
class UTextBlock;
class UScrollBox;
UCLASS()
class CRASHCOURSE_API UCC_SkillGalleryWidget : public UUserWidget {
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadOnly, Category="Presentation") TObjectPtr<UCC_SkillGalleryData> PresentationData;
 void InitializeGallery(UCC_SkillGalleryData* Data);
 void RefreshFromPlayer();
 void HandleEquipmentResult(ECC_SkillConfigurationResult Result);
 UFUNCTION(BlueprintCallable, Category="Presentation") void SelectCategory(int32 Index);
 UFUNCTION(BlueprintCallable, Category="Presentation") void SelectEntry(int32 Index);
 UFUNCTION(BlueprintCallable, Category="Presentation") void SelectPreviewSlot(int32 Index);
 UFUNCTION(BlueprintCallable, Category="Presentation") void PreviewEquip();
 UFUNCTION(BlueprintCallable, Category="Skills") void ResetEquipment();
 UFUNCTION(BlueprintPure, Category="Presentation") int32 GetSelectedCategory() const { return Category; }
 UFUNCTION(BlueprintPure, Category="Presentation") int32 GetSelectedEntry() const { return Entry; }
 UFUNCTION(BlueprintPure, Category="Presentation") int32 GetPreviewEntry(int32 Index) const { return PreviewSlots.IsValidIndex(Index) ? PreviewSlots[Index] : INDEX_NONE; }
 UFUNCTION(BlueprintPure, Category="Presentation") int32 GetVisibleCardCount() const { return CategoryEntries.Num(); }
protected:
 virtual void NativeOnInitialized() override;
private:
 UPROPERTY() TObjectPtr<UCanvasPanel> Root;
 UPROPERTY() TObjectPtr<UScrollBox> GridScroll;
 UPROPERTY() TObjectPtr<UTextBlock> CategoryTitle;
 UPROPERTY() TObjectPtr<UTextBlock> DetailTitle;
 UPROPERTY() TObjectPtr<UTextBlock> DetailCategory;
 UPROPERTY() TObjectPtr<UTextBlock> Description;
 UPROPERTY() TObjectPtr<UImage> Hero;
 UPROPERTY() TObjectPtr<UButton> EquipPreviewButton;
 UPROPERTY() TObjectPtr<UTextBlock> EquipmentStatus;
 UPROPERTY() TObjectPtr<UTextBlock> EquipmentHint;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotKeys;
 UPROPERTY() TArray<TObjectPtr<UButton>> CategoryButtons;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> CategoryLabels;
 UPROPERTY() TArray<TObjectPtr<UImage>> CategoryMarks;
 UPROPERTY() TArray<TObjectPtr<UButton>> CardButtons;
 UPROPERTY() TArray<TObjectPtr<UImage>> CardIcons;
 UPROPERTY() TArray<TObjectPtr<UImage>> CardFrames;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> CardLabels;
 UPROPERTY() TArray<TObjectPtr<UImage>> SlotIcons;
 UPROPERTY() TArray<TObjectPtr<UImage>> SlotFrames;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotEmpty;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> StatLabels;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> StatValues;
 UPROPERTY() TArray<int32> CategoryEntries;
 UPROPERTY() TArray<int32> PreviewSlots;
 int32 Category=0, Entry=INDEX_NONE, PreviewSlot=2;
 bool bBuilt=false;
 UTextBlock* Text(UCanvasPanel* Parent,const FString& Value,float X,float Y,float W,float H,int32 Size=24);
 UImage* Art(UCanvasPanel* Parent,UTexture2D* Texture,float X,float Y,float W,float H,bool Frame=false);
 UButton* Hit(UCanvasPanel* Parent,float X,float Y,float W,float H);
 void RefreshSelection();
 UFUNCTION() void Category0();
 UFUNCTION() void Category1();
 UFUNCTION() void Category2();
 UFUNCTION() void Category3();
 UFUNCTION() void Card0();
 UFUNCTION() void Card1();
 UFUNCTION() void Card2();
 UFUNCTION() void Card3();
 UFUNCTION() void Card4();
 UFUNCTION() void Card5();
 UFUNCTION() void Slot0();
 UFUNCTION() void Slot1();
 UFUNCTION() void Slot2();
 UFUNCTION() void Slot3();
};

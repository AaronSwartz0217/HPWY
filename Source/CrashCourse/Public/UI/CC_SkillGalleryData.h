#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CC_SkillGalleryData.generated.h"
class UTexture2D;
class UCC_SkillDefinition;
USTRUCT(BlueprintType)
struct FCC_SkillGalleryEntry {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DesignId;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCC_SkillDefinition> ExecutableSkill;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Category=0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FText> StatLabels;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FText> StatValues;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true")) FText Description;
};
// Artwork and authored descriptions reference explicit executable skills; IDs are never matched by display name.
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_SkillGalleryData : public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FText> Categories;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCC_SkillGalleryEntry> Entries;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UTexture2D>> CategoryTabs;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> DetailPanel;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> CardNormal;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> CardSelected;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> SlotNormal;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> SlotSelected;
};

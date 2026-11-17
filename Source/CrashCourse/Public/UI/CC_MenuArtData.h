#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CC_MenuArtData.generated.h"
class UTexture2D;
USTRUCT(BlueprintType)
struct FCC_MenuNavigationArt {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Normal;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Active;
};
UCLASS(BlueprintType)
class CRASHCOURSE_API UCC_MenuArtData : public UDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UCC_SkillGalleryData> SkillGallery;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Background;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Logo;
 // Ordered: inventory, skills, evolution/shop, settings.
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCC_MenuNavigationArt> Navigation;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> FallbackPortrait;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> FallbackTitle;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Home") FString WebsiteURL=TEXT("https://aaronswartz0217.github.io/Spirit-Tales/");
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Home") TSoftObjectPtr<UTexture2D> StartupLogo;
};

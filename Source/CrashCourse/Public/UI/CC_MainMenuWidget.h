#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_MainMenuWidget.generated.h"
class UCC_MenuArtData;
class UTextBlock;
class UButton;
class UWidget;
class UImage;

// Game home page, separate from the in-game Tab menu.
UCLASS()
class CRASHCOURSE_API UCC_MainMenuWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UCC_MainMenuWidget(const FObjectInitializer& ObjectInitializer);
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Home") TSoftObjectPtr<UCC_MenuArtData> ArtData;
 UFUNCTION(BlueprintCallable,Category="Home") void StartGame();
 UFUNCTION(BlueprintCallable,Category="Home") void OpenWebsite();
 UFUNCTION(BlueprintCallable,Category="Home") void ExitGame();
 UFUNCTION(BlueprintPure,Category="Home") bool IsStartupSplashActive() const {return bStartupSplashActive;}
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeConstruct() override;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
 UPROPERTY(Transient) TObjectPtr<UWidget> StartupLogoPanel;
 UPROPERTY(Transient) TObjectPtr<UImage> StartupLogoImage;
 bool bStartupSplashActive=false;
 double StartupLogoStartedAt=0.;
 UPROPERTY(Transient) TObjectPtr<UCC_MenuArtData> LoadedArt;
 UPROPERTY(Transient) TObjectPtr<UWidget> HomePanel;
 UPROPERTY(Transient) TObjectPtr<UWidget> LoadingPanel;
 UPROPERTY(Transient) TObjectPtr<UButton> StartButton;
 UPROPERTY(Transient) TObjectPtr<UButton> WebsiteButton;
 UPROPERTY(Transient) TObjectPtr<UButton> ExitButton;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
 FText WebsiteStatus;
 void BuildLayout();
};

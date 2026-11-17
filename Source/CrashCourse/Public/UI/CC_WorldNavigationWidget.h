#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_WorldNavigationWidget.generated.h"
class ACC_MinimapCapture;
class UTextBlock;
class UWidget;
class UButton;
class UImage;
class UCC_MenuArtData;

UCLASS()
class CRASHCOURSE_API UCC_WorldNavigationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UCC_WorldNavigationWidget(const FObjectInitializer& ObjectInitializer);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Travel|Appearance") TObjectPtr<UCC_MenuArtData> ArtData;
	void InitializeNavigation(ACC_MinimapCapture* Camera);
	UFUNCTION(BlueprintCallable,Category="Travel") void ToggleMinimap();
	UFUNCTION(BlueprintCallable,Category="Travel") void SetTravelOpen(bool bOpen);
	UFUNCTION(BlueprintPure,Category="Travel") bool IsTravelOpen() const {return bTravelOpen;}
	UFUNCTION(BlueprintPure,Category="Travel") bool IsMinimapVisible() const {return bMapVisible;}
	UFUNCTION(BlueprintCallable,Category="Travel") void SelectRegion(FName Id);
	UFUNCTION(BlueprintPure,Category="Travel") FName GetSelectedRegion() const {return SelectedRegion;}
	UFUNCTION(BlueprintImplementableEvent,Category="Travel") void OnNavigationVisibilityChanged(bool bMapOpen,bool bTravelMenuOpen);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
	virtual void NativeDestruct() override;
private:
	UPROPERTY(Transient) TObjectPtr<ACC_MinimapCapture> Minimap;
	UPROPERTY(Transient) TObjectPtr<UImage> MapImage;
 UPROPERTY(Transient) TObjectPtr<UWidget> MapDecoration;
	UPROPERTY(Transient) TObjectPtr<UWidget> MapPanel;
	UPROPERTY(Transient) TObjectPtr<UWidget> TravelPanel;
	UPROPERTY(Transient) TObjectPtr<UWidget> LoadingPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RegionTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Heading;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DestinationTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DestinationDescription;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmLabel;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TravelButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TravelLabels;
	bool bMapVisible=true;
	bool bTravelOpen=false;
	FName SelectedRegion;
	float RefreshElapsed=0.f;
	void BuildLayout();
	void RefreshPresentation();
	UFUNCTION() void ConfirmTravel();
	UFUNCTION() void CloseTravel();
	UFUNCTION() void TravelRest();
	UFUNCTION() void TravelTown();
	UFUNCTION() void TravelArena();
	UFUNCTION() void TravelHeaven();
	UFUNCTION() void TravelPalace();
};

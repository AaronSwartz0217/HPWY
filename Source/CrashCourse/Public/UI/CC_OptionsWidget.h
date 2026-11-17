#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Commands/InputChord.h"
#include "CC_OptionsWidget.generated.h"

class UCC_MenuArtData;
class UInputKeySelector;
class UTextBlock;
class UButton;
class ACC_PlayerController;

// Presentation for the existing local key, map and progression APIs.
UCLASS()
class CRASHCOURSE_API UCC_OptionsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	static void ApplySavedUpscaler();
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Options") TObjectPtr<UCC_MenuArtData> ArtData;
	UFUNCTION(BlueprintPure,Category="Options") bool IsSelectingKey() const;
	UFUNCTION(BlueprintPure,Category="Options") UInputKeySelector* GetKeySelector(int32 Action) const;
	UFUNCTION(BlueprintCallable,Category="Options") void RefreshOptions();
	UFUNCTION(BlueprintCallable,Category="Options") bool RebindAction(int32 Action,FInputChord Chord);
	UFUNCTION(BlueprintCallable,Category="Options") void ResetKeys();
	UFUNCTION(BlueprintCallable,Category="Options") void ToggleMap();
	UFUNCTION(BlueprintCallable,Category="Options") void SaveProgress();
	UFUNCTION(BlueprintCallable,Category="Options") void LoadProgress();
	UPROPERTY(BlueprintReadOnly,Category="Options") FText FeedbackMessage;
	UFUNCTION(BlueprintImplementableEvent,Category="Options") void OnOptionsMessage(const FText& Message,bool bError);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
	UPROPERTY(Transient) TObjectPtr<UTextBlock> UpscalerStatus;
	UPROPERTY(Transient) TObjectPtr<UButton> DLSSButton;
	void RefreshUpscaler();
	void SelectUpscaler(bool bDLSS);
	UFUNCTION() void SelectTSR();
	UFUNCTION() void SelectDLSS();
	UPROPERTY(Transient) TArray<TObjectPtr<UInputKeySelector>> KeySelectors;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MapStatus;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SaveStatus;
	UPROPERTY(Transient) TObjectPtr<UButton> MapButton;
	UPROPERTY(Transient) TObjectPtr<UButton> SaveButton;
	UPROPERTY(Transient) TObjectPtr<UButton> LoadButton;
	UPROPERTY(Transient) TWeakObjectPtr<ACC_PlayerController> BoundController;
	bool bUpdatingKeys=false;
	float RefreshElapsed=0.f;
	void BindController();
	void BuildLayout();
	void ApplyKey(int32 Action,FInputChord Chord);
	void Message(const FString& Text,bool bError=false);
	UFUNCTION() void Key0(FInputChord Chord);
	UFUNCTION() void Key1(FInputChord Chord);
	UFUNCTION() void Key2(FInputChord Chord);
	UFUNCTION() void Key3(FInputChord Chord);
	UFUNCTION() void Key4(FInputChord Chord);
	UFUNCTION() void Key5(FInputChord Chord);
};

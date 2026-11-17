#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CC_LoreReader.generated.h"

class SScrollBox;
class SWidgetSwitcher;
class UCC_LoreBook;

UCLASS()
class CRASHCOURSE_API UCC_LoreReader : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetBook(UCC_LoreBook* InBook) { Book = InBook; }
    const UCC_LoreBook* GetBook() const { return Book; }
    int32 GetPageIndex() const { return PageIndex; }
    bool IsOpeningThoughtActive() const { return bShowingThought; }
    int32 GetOpeningThoughtIndex() const { return ThoughtIndex; }
    void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    UPROPERTY(Transient) TObjectPtr<UCC_LoreBook> Book;
    TSharedPtr<SScrollBox> Contents;
    TSharedPtr<SScrollBox> PageScroll;
    TArray<TSharedPtr<SWidget>> PageButtons;
    TSharedPtr<SWidgetSwitcher> ReaderStage;
    int32 PageIndex = 0;
    int32 ThoughtIndex = 0;
    bool bShowingThought = false;

    FReply AdvanceOpeningThought();

    void SetPage(int32 Index);
    void CloseBook();
};

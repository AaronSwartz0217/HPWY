#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/CC_PlayerCharacter.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/CC_PlayerController.h"
#include "Progression/CC_ProgressionHUD.h"
#include "TimerManager.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "UI/CC_LoreReader.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"
#include "World/CC_LoreStand.h"

namespace
{
void ClickThought(UCC_LoreReader* Reader)
{
    auto& Slate = FSlateApplication::Get();
    const auto Window = Slate.FindWidgetWindow(Reader->TakeWidget());
    const FGeometry& Geometry = Reader->GetCachedGeometry();
    const FVector2D Point = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * .5);
    const TSet<FKey> Pressed{EKeys::LeftMouseButton};
    const TSet<FKey> Released;
    const FModifierKeysState Modifiers;
    Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex, Point, Point, Released, EKeys::Invalid, 0, Modifiers), true);
    Slate.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
        FPointerEvent(FSlateApplication::CursorPointerIndex, Point, Point, Pressed, EKeys::LeftMouseButton, 0, Modifiers));
    Slate.ProcessMouseButtonUpEvent(
        FPointerEvent(FSlateApplication::CursorPointerIndex, Point, Point, Released, EKeys::LeftMouseButton, 0, Modifiers));
}

void RunRestLoreSmoke(UWorld* World)
{
    if (!World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone)
        return;
    struct FProbe
    {
        bool bPassed = true;
        TArray<FString> Lines;
        void Check(const FString& Name, bool bOK)
        {
            bPassed &= bOK;
            Lines.Add(FString::Printf(TEXT("%s: %d"), *Name, bOK));
            UE_LOG(LogTemp, Display, TEXT("REST_LORE_CHECK %s: %d"), *Name, bOK);
        }
    };
    auto Probe = MakeShared<FProbe>();
    auto Later = [World](float Delay, TFunction<void()> Action)
    {
        FTimerHandle Timer;
        World->GetTimerManager().SetTimer(Timer, MoveTemp(Action), Delay, false);
    };
    Later(3.f,[World, Probe, Later]
    {
        auto* PC = Cast<ACC_PlayerController>(World->GetFirstPlayerController());
        auto* Pawn = PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
        TArray<ACC_LoreStand*> Stands;
        for (TActorIterator<ACC_LoreStand> It(World); It; ++It) Stands.Add(*It);
        Probe->Check(TEXT("three_placed_stands_and_player"), Stands.Num() == 3 && Pawn);
        if (Pawn)
        {
            for (int32 i = 0; i < Stands.Num(); ++i)
            {
                auto* Stand = Stands[i];
                const float Time = 0.15f + i*2.8f;
                const FString Name = Stand->Book ? Stand->Book->Title.ToString() : TEXT("missing");
                Later(Time, [PC, Pawn, Stand]
                {
                    Pawn->SetFirstPerson(false);
                    Pawn->TeleportTo(Stand->GetActorLocation(), FRotator::ZeroRotator, false, true);
                });
                Later(Time + .20f, [PC, Stand, Probe, Name]
                {
                    const auto* HUD = Cast<ACC_ProgressionHUD>(PC->GetHUD());
                    Probe->Check(Name + TEXT(" proximity prompt"), PC->FindInteractionTarget() == Stand &&
                        HUD && HUD->FirstPersonScanHUD && HUD->FirstPersonScanHUD->GetDisplayedTarget() == Stand);
                    const FKey Key = PC->GetCombatKey(5);
                    for (auto& Binding : PC->InputComponent->KeyBindings)
                    {
                        if (Binding.Chord.Key == Key && Binding.KeyEvent == IE_Pressed)
                        {
                            Binding.KeyDelegate.Execute(Key);
                            break;
                        }
                    }
                    const auto* Reader = PC->GetLoreReader();
                    Probe->Check(Name + TEXT(" E opens opening thought"), Reader &&
                        Reader->GetBook() == Stand->Book && Reader->IsOpeningThoughtActive() &&
                        Reader->GetOpeningThoughtIndex() == 0 && PC->IsGameplayInputBlocked());
                });
                Later(Time + .55f, [PC, Probe, Name, i]
                {
                    if (auto* Reader = PC->GetLoreReader())
                    {
                        if (i == 0) FScreenshotRequest::RequestScreenshot(
                            FPaths::ProjectSavedDir()/TEXT("Screenshots/RestLore/Thought.png"), true, false);
                        ClickThought(Reader);
                        const bool bHasMoreThoughts = Reader->GetBook()->OpeningThoughts.Num() > 1;
                        Probe->Check(Name + TEXT(" left click advances exactly one line"),
                            bHasMoreThoughts ? Reader->IsOpeningThoughtActive() && Reader->GetOpeningThoughtIndex() == 1
                                            : !Reader->IsOpeningThoughtActive());
                    }
                });
                Later(Time + 1.22f, [PC, Probe, Name]
                {
                    if (auto* Reader = PC->GetLoreReader())
                    {
                        const int32 ThoughtCount = Reader->GetBook()->OpeningThoughts.Num();
                        for (int32 Click = 0; Click < ThoughtCount && Reader->IsOpeningThoughtActive(); ++Click)
                            ClickThought(Reader);
                        Probe->Check(Name + TEXT(" final click opens the book"),
                            !Reader->IsOpeningThoughtActive() && Reader->GetPageIndex() == 0);
                    }
                });
                Later(Time + 1.58f, [PC, Probe, Name]
                {
                    if (auto* Reader = PC->GetLoreReader())
                    {
                        FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Right, {}, 0, false, 0, 0));
                        Probe->Check(Name + TEXT(" keyboard page turn"), Reader->GetPageIndex() == 1);
                        if (Name == TEXT("山河物魄志")) FScreenshotRequest::RequestScreenshot(
                            FPaths::ProjectSavedDir()/TEXT("Screenshots/RestLore/Book.png"), true, false);
                    }
                });
                Later(Time + 2.10f, [PC, Probe, Name]
                {
                    FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Tab, {}, 0, false, 0, 0));
                    Probe->Check(Name + TEXT(" close restores gameplay input"), !PC->GetLoreReader() &&
                        !PC->IsGameplayInputBlocked() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored());
                    PC->CloseLoreReader();
                });
            }
        }
        Later(9.f, [World, Probe]
        {
            Probe->Lines.Add(Probe->bPassed ? TEXT("PASS") : TEXT("FAIL"));
            FFileHelper::SaveStringArrayToFile(Probe->Lines, *(FPaths::ProjectSavedDir()/TEXT("RestLoreSmoke.txt")));
            GEngine->Exec(World,TEXT("QUIT"));
        });
    });
}

FAutoConsoleCommandWithWorld RestLoreSmokeCommand(TEXT("CC.RestLoreSmoke"),
    TEXT("Exercise the three home reading points through E and Slate input."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&RunRestLoreSmoke));
}

#endif


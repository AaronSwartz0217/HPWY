#include "World/CC_MinimapCapture.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ACC_MinimapCapture::ACC_MinimapCapture()
{
	bReplicates=false;PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f;
	Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));SetRootComponent(Capture);
	Capture->ProjectionType=ECameraProjectionMode::Orthographic;
	// Final LDR keeps an opaque alpha for UMG. Lighting/show flags make this unlit;
	// SCS_BaseColor writes alpha=0 and would disappear in a normal Image brush.
	Capture->OrthoWidth=ViewWidth;Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
	Capture->ShowFlags.SetLighting(false);Capture->ShowFlags.SetDynamicShadows(false);
	Capture->ShowFlags.SetPostProcessing(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetSkyLighting(false);Capture->ShowFlags.SetReflectionEnvironment(false);
	Capture->ShowFlags.SetAmbientOcclusion(false);Capture->ShowFlags.SetVolumetricFog(false);
}
void ACC_MinimapCapture::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickInterval(1.f/FMath::Clamp(RefreshRate,1.f,30.f));
	RenderTarget=NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->RenderTargetFormat=RTF_RGBA8;RenderTarget->ClearColor=FLinearColor(.015f,.02f,.025f,1);
	RenderTarget->InitAutoFormat(FMath::Clamp(Resolution,64,1024),FMath::Clamp(Resolution,64,1024));
	Capture->TextureTarget=RenderTarget;Capture->OrthoWidth=ViewWidth;
	// Optional art tags allow roofs/sky cards to be excluded without per-frame actor scans.
	for (TActorIterator<AActor> It(GetWorld());It;++It) if (It->ActorHasTag(TEXT("MinimapHide"))) Capture->HiddenActors.Add(*It);
}
void ACC_MinimapCapture::SetCaptureEnabled(bool bEnabled)
{
	SetActorTickEnabled(bEnabled);
	Capture->SetComponentTickEnabled(bEnabled);
}
void ACC_MinimapCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	auto* PC=FollowingController.Get();APawn* Pawn=PC ? PC->GetPawn() : nullptr;
	if (!Pawn || !RenderTarget) return;
	SetActorLocationAndRotation(Pawn->GetActorLocation()+FVector(0,0,CaptureHeight),FRotator(-90,0,0));
	Capture->CaptureScene();
}

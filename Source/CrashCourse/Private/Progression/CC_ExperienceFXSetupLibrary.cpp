#include "Progression/CC_ExperienceFXSetupLibrary.h"

#if WITH_EDITOR
#include "Materials/MaterialInterface.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#endif

bool UCC_ExperienceFXSetupLibrary::BuildExperienceOrbSystem(UNiagaraSystem* System, UMaterialInterface* SpriteMaterial)
{
#if WITH_EDITOR
	if (!System || !SpriteMaterial) return false;
	UNiagaraEmitter* Template = LoadObject<UNiagaraEmitter>(nullptr,
		TEXT("/Niagara/DefaultAssets/Templates/Emitters/SingleLoopingParticle.SingleLoopingParticle"));
	if (!Template) return false;
	System->Modify();
	TSet<FGuid> ExistingHandles;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles()) ExistingHandles.Add(Handle.GetId());
	System->RemoveEmitterHandlesById(ExistingHandles);
	// The standard empty system factory already created the system scripts. This also supports
	// systems created by NewObject, which makes the helper safe to reuse from editor tooling.
	if (!System->GetSystemSpawnScript()->GetLatestSource()) UNiagaraSystemFactoryNew::InitializeSystem(System, true);
	// The editor utility also inserts emitter-call nodes into System Spawn/Update.
	// Adding only a handle creates an asset that compiles, but never executes the emitter.
	const FGuid EmitterId = FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Template, Template->GetExposedVersion().VersionGuid);
	FNiagaraEmitterHandle* Handle = System->GetEmitterHandles().FindByPredicate([&](const FNiagaraEmitterHandle& Entry) { return Entry.GetId() == EmitterId; });
	FVersionedNiagaraEmitterData* Data = Handle ? Handle->GetEmitterData() : nullptr;
	if (!Data) return false;
	Data->RemoveParent();
	Data->bLocalSpace = true;
	Data->SimTarget = ENiagaraSimTarget::CPUSim;
	Data->FixedBounds = FBox(FVector(-30), FVector(30));
	Data->AllocationMode = EParticleAllocationMode::FixedCount;
	Data->PreAllocationCount = 1;
	for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
		if (UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer)) Sprite->Material = SpriteMaterial;

	// Use the template's editable rapid-iteration inputs instead of mutating engine content.
	// A single sprite lasts longer than its owning XP actor (maximum lifetime is bounded in C++).
	// The particle never receives velocity; all collision/scatter/homing belongs to that actor.
	TArray<UNiagaraScript*> Scripts;
	Data->GetScripts(Scripts, false);
	int32 ChangedInputs = 0;
	for (UNiagaraScript* Script : Scripts)
	{
		if (!Script) continue;
		TArray<FNiagaraVariable> Inputs;
		Script->RapidIterationParameters.GetParameters(Inputs);
		for (const FNiagaraVariable& Input : Inputs)
		{
			const FString Name = Input.GetName().ToString();
			bool Changed = false;
			if (Input.GetType() == FNiagaraTypeDefinition::GetFloatDef())
			{
				float Value = Script->RapidIterationParameters.GetParameterValue<float>(Input);
				if (Name.EndsWith(TEXT(".Lifetime Min")) || Name.EndsWith(TEXT(".Lifetime Max")) || Name.EndsWith(TEXT(".Loop Duration"))) { Value = 60.f; Changed = true; }
				else if (Name.EndsWith(TEXT(".Uniform Sprite Size"))) { Value = 12.f; Changed = true; }
				else if (Name.EndsWith(TEXT(".Spawn Time")) || Name.EndsWith(TEXT(".Loop Delay"))) { Value = 0.f; Changed = true; }
				if (Changed) Script->RapidIterationParameters.SetParameterValue(Value, Input);
			}
			else if (Input.GetType() == FNiagaraTypeDefinition::GetIntDef() && Name.EndsWith(TEXT(".Spawn Count")))
			{
				Script->RapidIterationParameters.SetParameterValue(1, Input); Changed = true;
			}
			else if (Input.GetType() == FNiagaraTypeDefinition::GetVec2Def() && (Name.EndsWith(TEXT(".Sprite Size Min")) || Name.EndsWith(TEXT(".Sprite Size Max"))))
			{
				Script->RapidIterationParameters.SetParameterValue(FVector2f(12, 12), Input); Changed = true;
			}
			else if (Input.GetType() == FNiagaraTypeDefinition::GetColorDef() && Name.EndsWith(TEXT(".Color")))
			{
				Script->RapidIterationParameters.SetParameterValue(FLinearColor(1.f, .5f, .08f, 1.f), Input); Changed = true;
			}
			if (Changed) { ++ChangedInputs; UE_LOG(LogTemp, Display, TEXT("Experience sprite input: %s"), *Name); }
		}
	}
	System->SetFixedBounds(FBox(FVector(-30), FVector(30)));
	System->RequestCompile(true);
	System->WaitForCompilationComplete();
	System->MarkPackageDirty();
	// IsReadyToRun intentionally returns false under -NullRHI, including for a valid asset.
	// Validate compilation here; the runtime smoke test checks the actual rendered component.
	UE_LOG(LogTemp, Display, TEXT("Experience sprite setup: %d inputs, valid=%d ready=%d"), ChangedInputs, System->IsValid(), System->IsReadyToRun());
	return ChangedInputs > 0 && System->IsValid() && !System->HasOutstandingCompilationRequests();
#else
	return false;
#endif
}

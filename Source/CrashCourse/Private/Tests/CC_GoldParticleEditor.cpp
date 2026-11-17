#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Progression/CC_GameplayData.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace {
void BuildGoldParticleFX(){
 auto* Core=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/HPWH/Effects/PlayerParticle/M_PlayerParticleCore.M_PlayerParticleCore"));
 auto* Skill=LoadObject<UCC_SkillDefinition>(nullptr,TEXT("/Game/HPWH/Skills/DA_Skill_01.DA_Skill_01"));
 if(!Core||!Skill){UE_LOG(LogTemp,Error,TEXT("GOLD_PARTICLE_SETUP missing core/skill"));return;}
 Skill->ProjectileCoreMaterial=Core;Skill->ProjectileNiagara=nullptr;Skill->CastNiagara=nullptr;Skill->ImpactNiagara=nullptr;Skill->CastParticles=nullptr;Skill->ImpactParticles=nullptr;Skill->EffectScale=.65f;Skill->MarkPackageDirty();
 FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
 if(UPackage::SavePackage(Skill->GetOutermost(),Skill,*FPackageName::LongPackageNameToFilename(Skill->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args))UE_LOG(LogTemp,Display,TEXT("GOLD_PARTICLE_SETUP_PASS clean energy core and impact flash; balance preserved"));
}
FAutoConsoleCommand GoldParticleEditorCommand(TEXT("CC.BuildGoldParticleFX"),TEXT("Explicit editor-only configuration of the player gold pulse."),FConsoleCommandDelegate::CreateStatic(&BuildGoldParticleFX));
}
#endif

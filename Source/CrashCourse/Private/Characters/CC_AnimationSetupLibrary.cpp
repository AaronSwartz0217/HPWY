#include "Characters/CC_AnimationSetupLibrary.h"
#include "Materials/MaterialInterface.h"
bool UCC_AnimationSetupLibrary::IsNaniteBlendSupported(UMaterialInterface* Material)
{
 return Material && (Material->GetBlendMode()==BLEND_Opaque || Material->GetBlendMode()==BLEND_Masked);
}
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/UObjectHash.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateMachineGraph.h"
#include "K2Node_VariableGet.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

template<class T> static T* AddAnimNode(UEdGraph* Graph, int32 X = 0, int32 Y = 0)
{
	FGraphNodeCreator<T> Creator(*Graph);
	T* Node = Creator.CreateNode(); Node->NodePosX = X; Node->NodePosY = Y; Creator.Finalize();
	return Node;
}
static UEdGraphPin* PoseOutput(UEdGraphNode* Node)
{
	for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output) return Pin;
	return nullptr;
}
#endif
int32 UCC_AnimationSetupLibrary::RepairLegacyWidgetPins(UBlueprint* Blueprint)
{
#if WITH_EDITOR
 if (!Blueprint) return -1;
 FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
 int32 Removed=0;TArray<UEdGraph*> Graphs;Blueprint->GetAllGraphs(Graphs);
 for (UEdGraph* Graph:Graphs) for (UEdGraphNode* Node:Graph->Nodes) {
  const auto Pins=Node->Pins;
  for (UEdGraphPin* Pin:Pins) if (Pin->bOrphanedPin && Pin->LinkedTo.IsEmpty() && Pin->PinName.ToString().Replace(TEXT(" "),TEXT("")).Equals(TEXT("DamageActor"),ESearchCase::IgnoreCase)) {
   Node->RemovePin(Pin);++Removed;
  }
 }
 FKismetEditorUtilities::CompileBlueprint(Blueprint);Blueprint->MarkPackageDirty();return Blueprint->Status==BS_Error ? -1 : Removed;
#else
 return -1;
#endif
}
FString UCC_AnimationSetupLibrary::DescribeBlueprintGraphs(UBlueprint* Blueprint)
{
	FString Text;
#if WITH_EDITOR
	if (!Blueprint) return Text;
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (const UEdGraph* Graph : Graphs) for (const UEdGraphNode* Node : Graph->Nodes)
	{
		Text += FString::Printf(TEXT("\n%s | %s | %s\n"), *Graph->GetName(), *Node->GetName(), *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			Text += FString::Printf(TEXT("  %s = %s %s"), *Pin->PinName.ToString(), *Pin->DefaultValue, Pin->DefaultObject ? *Pin->DefaultObject->GetPathName() : TEXT(""));
			for (const UEdGraphPin* Linked : Pin->LinkedTo) Text += FString::Printf(TEXT(" -> %s.%s"), *Linked->GetOwningNode()->GetName(), *Linked->PinName.ToString());
			Text += TEXT("\n");
		}
	}
#endif
	return Text;
}
bool UCC_AnimationSetupLibrary::BuildLocomotionGraph(UAnimBlueprint* Blueprint, UAnimSequence* Idle, UAnimSequence* Run, UAnimSequence* Jump, UAnimSequence* Fall, UAnimSequence* Land, UAnimSequence* Death)
{
#if WITH_EDITOR
	if (!Blueprint || !Idle || !Run || !Jump || !Fall || !Land || !Death) return false;
	UAnimSequence* ValidationClips[] = { Idle, Run, Jump, Fall, Land, Death };
	for (UAnimSequence* Clip : ValidationClips) if (Clip->GetSkeleton() != Blueprint->TargetSkeleton) return false;
	Blueprint->TargetSkeleton->SetSlotGroupName(TEXT("DefaultSlot"), TEXT("DefaultGroup"));
	Blueprint->TargetSkeleton->MarkPackageDirty();
	UEdGraph* Graph = nullptr;
	for (UEdGraph* Candidate : Blueprint->FunctionGraphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
	if (!Graph) return false;
	UAnimGraphNode_Root* Root = nullptr;
	const TArray<TObjectPtr<UEdGraphNode>> OldNodes = Graph->Nodes;
	for (UEdGraphNode* Node : OldNodes)
		if (UAnimGraphNode_Root* Result = Cast<UAnimGraphNode_Root>(Node)) Root = Result;
		else FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
	if (!Root) return false;
	UAnimGraphNode_StateMachine* Machine = AddAnimNode<UAnimGraphNode_StateMachine>(Graph, -500);
	UEdGraph* States = Machine->EditorStateMachineGraph;
	FBlueprintEditorUtils::RenameGraph(States, TEXT("CharacterLocomotion"));
	const TCHAR* Names[] = { TEXT("Idle"), TEXT("Run"), TEXT("Jump"), TEXT("Fall"), TEXT("Land"), TEXT("Death") };
	const TCHAR* Properties[] = { TEXT("bIdle"), TEXT("bRun"), TEXT("bJump"), TEXT("bFall"), TEXT("bLand"), TEXT("bDead") };
	UAnimSequence* Clips[] = { Idle, Run, Jump, Fall, Land, Death };
	TArray<UAnimStateNode*> Nodes;
	for (int32 I = 0; I < 6; ++I)
	{
		UAnimStateNode* State = AddAnimNode<UAnimStateNode>(States, (I % 3) * 300, (I / 3) * 300);
		FBlueprintEditorUtils::RenameGraph(State->BoundGraph, Names[I]); Nodes.Add(State);
		UAnimGraphNode_SequencePlayer* Player = AddAnimNode<UAnimGraphNode_SequencePlayer>(State->BoundGraph, -300);
		Player->Node.SetSequence(Clips[I]); Player->Node.SetLoopAnimation(I == 0 || I == 1 || I == 3);
		if (I == 1)
		{
			for (FOptionalPinFromProperty& Pin : Player->ShowPinForProperties) if (Pin.PropertyName == TEXT("PlayRate")) Pin.bShowPin = true;
			Player->ReconstructNode();
			FGraphNodeCreator<UK2Node_VariableGet> Creator(*State->BoundGraph);
			UK2Node_VariableGet* Rate = Creator.CreateNode(); Rate->VariableReference.SetSelfMember(TEXT("RunPlayRate")); Rate->NodePosX = -550; Creator.Finalize();
			State->BoundGraph->GetSchema()->TryCreateConnection(Rate->GetValuePin(), Player->FindPinChecked(TEXT("PlayRate")));
		}
		for (UEdGraphNode* Node : State->BoundGraph->Nodes)
			if (UAnimGraphNode_StateResult* Result = Cast<UAnimGraphNode_StateResult>(Node))
				State->BoundGraph->GetSchema()->TryCreateConnection(PoseOutput(Player), Result->FindPinChecked(TEXT("Result")));
	}
	for (UEdGraphNode* Node : States->Nodes)
		if (UAnimStateEntryNode* Entry = Cast<UAnimStateEntryNode>(Node)) States->GetSchema()->TryCreateConnection(PoseOutput(Entry), Nodes[0]->GetInputPin());
	for (int32 From = 0; From < 6; ++From) for (int32 To = 0; To < 6; ++To)
	{
		if (From == To) continue;
		UAnimStateTransitionNode* Transition = AddAnimNode<UAnimStateTransitionNode>(States);
		Transition->CreateConnections(Nodes[From], Nodes[To]);
        Transition->CrossfadeDuration = To==5 ? .12f : To==2 || To==4 ? .10f : To==3 || From==4 ? .18f : .24f;
        Transition->BlendMode=EAlphaBlendOption::Cubic;
		FGraphNodeCreator<UK2Node_VariableGet> Creator(*Transition->BoundGraph);
		UK2Node_VariableGet* Get = Creator.CreateNode(); Get->VariableReference.SetSelfMember(Properties[To]); Get->NodePosX = -250; Creator.Finalize();
		for (UEdGraphNode* Node : Transition->BoundGraph->Nodes)
			if (UAnimGraphNode_TransitionResult* Result = Cast<UAnimGraphNode_TransitionResult>(Node))
				Transition->BoundGraph->GetSchema()->TryCreateConnection(Get->GetValuePin(), Result->FindPinChecked(TEXT("bCanEnterTransition")));
	}
	UAnimGraphNode_Slot* Slot = AddAnimNode<UAnimGraphNode_Slot>(Graph, -250);
	Slot->Node.SlotName = TEXT("DefaultSlot");
    Slot->Node.bAlwaysUpdateSourcePose=true;
	Graph->GetSchema()->TryCreateConnection(PoseOutput(Machine), Slot->FindPinChecked(TEXT("Source")));
	Graph->GetSchema()->TryCreateConnection(PoseOutput(Slot), Root->FindPinChecked(TEXT("Result")));
	return !ConfigureUpperBodyAttack(Blueprint).IsNone();
#else
	return false;
#endif
}

int32 UCC_AnimationSetupLibrary::TuneLocomotionBlends(UAnimBlueprint* Blueprint)
{
#if WITH_EDITOR
    if (!Blueprint) return -1;
    int32 Updated=0;
    TArray<UEdGraph*> Graphs;Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph:Graphs) for (UEdGraphNode* Node:Graph->Nodes)
    {
        if (auto* Transition=Cast<UAnimStateTransitionNode>(Node))
        {
            const auto* From=Transition->GetPreviousState();const auto* To=Transition->GetNextState();
            if (!From || !To || Graph->GetFName()!=TEXT("CharacterLocomotion")) continue;
            const FString Target=To->GetStateName(),Previous=From->GetStateName();
            Transition->CrossfadeDuration=Target==TEXT("Death") ? .12f : Target==TEXT("Jump") || Target==TEXT("Land") ? .10f : Target==TEXT("Fall") || Previous==TEXT("Land") ? .18f : .24f;
            Transition->BlendMode=EAlphaBlendOption::Cubic;++Updated;
        }
        if (auto* Slot=Cast<UAnimGraphNode_Slot>(Node))
            if (Slot->Node.SlotName==TEXT("DefaultSlot")) {Slot->Node.bAlwaysUpdateSourcePose=true;++Updated;}
    }
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status==BS_Error ? -1 : Updated;
#else
    return -1;
#endif
}

FName UCC_AnimationSetupLibrary::ConfigureUpperBodyAttack(UAnimBlueprint* Blueprint)
{
#if WITH_EDITOR
    if (!Blueprint || !Blueprint->TargetSkeleton) return NAME_None;
    const FReferenceSkeleton& Skeleton = Blueprint->TargetSkeleton->GetReferenceSkeleton();
    FName Spine = NAME_None;
    const FName Candidates[] = { TEXT("spine_01"), TEXT("Spine"), TEXT("mixamorig:Spine"), TEXT("mixamorig_Spine") };
    for (const FName Candidate : Candidates)
        if (Skeleton.FindBoneIndex(Candidate) != INDEX_NONE) { Spine = Candidate; break; }
    if (Spine.IsNone()) return NAME_None;

    UEdGraph* Graph = nullptr;
    for (UEdGraph* Candidate : Blueprint->FunctionGraphs)
        if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
    if (!Graph) return NAME_None;
    UAnimGraphNode_Root* Root = nullptr;
    UAnimGraphNode_StateMachine* Machine = nullptr;
    UAnimGraphNode_Slot* Slot = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Result = Cast<UAnimGraphNode_Root>(Node)) Root = Result;
        if (auto* State = Cast<UAnimGraphNode_StateMachine>(Node))
            if (State->EditorStateMachineGraph && State->EditorStateMachineGraph->GetFName() == TEXT("CharacterLocomotion")) Machine = State;
        if (auto* Montage = Cast<UAnimGraphNode_Slot>(Node))
            if (Montage->Node.SlotName == TEXT("DefaultSlot")) Slot = Montage;
    }
    if (!Root || !Machine || !Slot) return NAME_None;

    // Replace only our generated layer nodes. Keep all locomotion states and transition rules.
    const FString Marker = TEXT("CC Upper Body Attack");
    const TArray<TObjectPtr<UEdGraphNode>> OldNodes = Graph->Nodes;
    for (UEdGraphNode* Node : OldNodes)
        if (Node->NodeComment == Marker) FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
    const UEdGraphSchema* Schema = Graph->GetSchema();
    Schema->BreakPinLinks(*PoseOutput(Machine), true);
    Schema->BreakPinLinks(*Slot->FindPinChecked(TEXT("Source")), true);
    Schema->BreakPinLinks(*Root->FindPinChecked(TEXT("Result")), true);

    auto* Save = AddAnimNode<UAnimGraphNode_SaveCachedPose>(Graph, -850, 0);
    Save->CacheName = TEXT("CC_LocomotionPose"); Save->NodeComment = Marker;
    auto* Base = AddAnimNode<UAnimGraphNode_UseCachedPose>(Graph, -520, 200);
    auto* AttackSource = AddAnimNode<UAnimGraphNode_UseCachedPose>(Graph, -850, -220);
    Base->SaveCachedPoseNode = Save; Base->NodeComment = Marker;
    AttackSource->SaveCachedPoseNode = Save; AttackSource->NodeComment = Marker;
    auto* Layer = AddAnimNode<UAnimGraphNode_LayeredBoneBlend>(Graph, 0, 0);
    Layer->NodeComment = Marker;
    Layer->Node.bMeshSpaceRotationBlend = true;
    Layer->Node.bBlendRootMotionBasedOnRootBone = true;
    Layer->Node.BlendWeights[0] = 1.f;
    FBranchFilter Filter; Filter.BoneName = Spine; Filter.BlendDepth = 3;
    Layer->Node.LayerSetup[0].BranchFilters = { Filter };
    Slot->Node.bAlwaysUpdateSourcePose = true;
    Machine->NodePosX = -1200; Machine->NodePosY = 0;
    Slot->NodePosX = -520; Slot->NodePosY = -220;
    Root->NodePosX = 400;

    bool bConnected = Schema->TryCreateConnection(PoseOutput(Machine), Save->FindPinChecked(TEXT("Pose")));
    bConnected &= Schema->TryCreateConnection(PoseOutput(AttackSource), Slot->FindPinChecked(TEXT("Source")));
    bConnected &= Schema->TryCreateConnection(PoseOutput(Base), Layer->FindPinChecked(TEXT("BasePose")));
    bConnected &= Schema->TryCreateConnection(PoseOutput(Slot), Layer->FindPinChecked(TEXT("BlendPoses_0")));
    bConnected &= Schema->TryCreateConnection(PoseOutput(Layer), Root->FindPinChecked(TEXT("Result")));
    if (!bConnected) return NAME_None;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status == BS_Error ? NAME_None : Spine;
#else
    return NAME_None;
#endif
}

int32 UCC_AnimationSetupLibrary::ReplacePackageAssetReferences(UObject* Asset, const TArray<UObject*>& From, const TArray<UObject*>& To)
{
#if WITH_EDITOR
    if (!Asset || From.Num() != To.Num()) return -1;
    TMap<UObject*,UObject*> Replacements;
    for (int32 I=0; I<From.Num(); ++I)
        if (From[I] && To[I] && To[I]->IsA(From[I]->GetClass()) && From[I] != To[I])
            Replacements.Add(From[I],To[I]);
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Asset->GetOutermost(), Objects, true);
    int32 Count = 0;
    for (UObject* Obj : Objects)
    {
        FArchiveReplaceObjectRef<UObject> Archive(Obj, Replacements,
            EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);
        Count += Archive.GetCount();
    }
    if (Count) Asset->MarkPackageDirty();
    return Count;
#else
    return -1;
#endif
}

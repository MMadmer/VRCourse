// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/VRPhysGrabComponent.h"

#include "Kismet/KismetSystemLibrary.h"

UVRPhysGrabComponent::UVRPhysGrabComponent()
{
	bIsParent = true;
	bFreeGrab = false;
	GrabObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));

	SetIsReplicated(true);

	SetDisableCollision(true);
	SetAngularSwing1Limit(ACM_Locked, 45.0f);
	SetAngularSwing2Limit(ACM_Locked, 45.0f);
	SetAngularTwistLimit(ACM_Locked, 45.0f);
}

bool UVRPhysGrabComponent::TryGrab(const bool bDebug)
{
	if (IsHeld() || !Cast<UPrimitiveComponent>(GetAttachParent())) return false;

	const FHitResult GrabHit = FindGrabObject(bDebug);
	if (!GrabHit.bBlockingHit) return false;

	if (!IsValid(GrabHit.GetActor()) || !GrabHit.GetActor()->ActorHasTag(GrabTag)) return false;

	if (IsValid(GrabHit.GetComponent())) ConnectComponents(GrabHit.GetComponent());
	else
	{
		UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(GrabHit.GetActor()->GetRootComponent());
		if (!IsValid(PhysComp)) return false;

		ConnectComponents(PhysComp);
	}

	if (!IsHeld()) return false;

	OnGrabbed.Broadcast(GetHeldComponent(), GetParentComponent());

	return true;
}

bool UVRPhysGrabComponent::TryRelease()
{
	if (!IsHeld()) return true;

	UPrimitiveComponent* HeldComp = GetHeldComponent();
	UPrimitiveComponent* ParentComp = GetParentComponent();

	BreakConstraint();

	if (IsHeld()) return false;

	OnReleased.Broadcast(HeldComp, ParentComp);

	return true;
}

UPrimitiveComponent* UVRPhysGrabComponent::GetHeldComponent()
{
	UPrimitiveComponent* FirstComp;
	FName FirstBone;

	UPrimitiveComponent* SecondComp;
	FName SecondBone;

	GetConstrainedComponents(FirstComp, FirstBone, SecondComp, SecondBone);

	return bIsParent ? FirstComp : SecondComp;
}

UPrimitiveComponent* UVRPhysGrabComponent::GetParentComponent()
{
	UPrimitiveComponent* FirstComp;
	FName FirstBone;

	UPrimitiveComponent* SecondComp;
	FName SecondBone;

	GetConstrainedComponents(FirstComp, FirstBone, SecondComp, SecondBone);

	return bIsParent ? SecondComp : FirstComp;
}

FHitResult UVRPhysGrabComponent::FindGrabObject(const bool bDebug) const
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	const EDrawDebugTrace::Type DebugType = bDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;


	FHitResult HitResult{};
	UKismetSystemLibrary::SphereTraceSingleForObjects(this, GetComponentLocation(), GetComponentLocation(), GrabRadius,
	                                                  GrabObjectTypes, true, ActorsToIgnore, DebugType, HitResult,
	                                                  true);

	return HitResult;
}

void UVRPhysGrabComponent::ConnectComponents(UPrimitiveComponent* OtherComp, const FName& BoneName)
{
	if (bIsParent)
		SetConstrainedComponents(OtherComp, BoneName, Cast<UPrimitiveComponent>(GetAttachParent()),
		                         SelfBoneName);
	else SetConstrainedComponents(Cast<UPrimitiveComponent>(GetAttachParent()), SelfBoneName, OtherComp, BoneName);
}

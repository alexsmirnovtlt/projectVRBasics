// Alex Smirnov 2020-2021


#include "VRMotionControllerHand.h"

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MotionControllerComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "HandActor.h"
#include "VirtualRealityPawn.h"
#include "HandPhysConstraint.h"


AVRMotionControllerHand::AVRMotionControllerHand()
{
	PrimaryActorTick.bCanEverTick = true;

	MotionControllerCheckInterval = 0.8f;
	bHandFollowsController = false;
}

void AVRMotionControllerHand::BeginPlay()
{
	Super::BeginPlay();

	if (!PhysicalHandClass || !PhysicalHandConstraintClass || !GetPhantomHandSkeletalMesh()) return;

	// Waiting to give headset some time to update motion controller location so it wont move to local FVector::ZeroVector on spawn 
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_BeginPlayWait,
		this,
		&AVRMotionControllerHand::OnBeginPlayWaitEnd,
		MotionControllerCheckInterval,
		true
	);
}

void AVRMotionControllerHand::Destroyed()
{
	Super::Destroyed();

	if (HandActor) HandActor->Destroy();
	if (PhysConstraint) PhysConstraint->Destroy();

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_BeginPlayWait))
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BeginPlayWait);
	}
}

void AVRMotionControllerHand::OnBeginPlayWaitEnd()
{
	if (!MotionController->IsTracked() || MotionController->CurrentTrackingStatus != ETrackingStatus::Tracked)
	{
		// Timer will loop for now
		UE_LOG(LogTemp, Error, TEXT("AVRMotionControllerHand Init was skipped for now. Will retry")); // TODO Check if that is actually may happen
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BeginPlayWait);

	HandActor = GetWorld()->SpawnActor<AHandActor>(PhysicalHandClass);
	HandActor->SetOwner(this);
	HandActor->SetInstigator(OwningVRPawn);

	HandActor->ChangeHandPhysProperties(false, true);
	StartFollowingPhantomHand(false);

	HandActor->ChangeHandPhysProperties(true, true); // Enabling physics and collision and sweeping from camera to Motion Controller location
	SweepHandToMotionControllerLocation(true);

	HandActor->RefreshWeldedBoneDriver(); // TODO Setup was already done in BeginPlay() of HandActor and now we must reinit or it wont update properly for some reason 

	OnPhysicalHandAppearedEvent(); // Hand is placed in the world. BP may add some logic here
}

void AVRMotionControllerHand::SweepHandToMotionControllerLocation(bool bSweepFromCamera)
{
	if (!HandActor) return;

	if (bSweepFromCamera)
	{
		TeleportHandToLocation(OwningVRPawn->GetCameraWorldTransform().GetLocation(), GetPhantomHandSkeletalMesh()->GetComponentRotation());
	}

	HandActor->SetActorTransform(GetPhantomHandSkeletalMesh()->GetComponentTransform(), true, nullptr, ETeleportType::TeleportPhysics);
}

void AVRMotionControllerHand::TeleportHandToLocation(FVector WorldLocation, FRotator WorldRotation)
{
	HandActor->SetActorLocation(WorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
	HandActor->SetActorRotation(WorldRotation, ETeleportType::TeleportPhysics);
}

void AVRMotionControllerHand::StartFollowingPhantomHand(bool bReturnHandBackAfterSetup)
{
	if (!bPhysConstraintAttachedToPhantomHand) AttachPhysConstraintToPhantomHand();

	FTransform CurrentHandTransform = HandActor->GetTransform();

	HandActor->SetActorTransform(GetPhantomHandSkeletalMesh()->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	PhysConstraint->CreateConstraint(HandActor->GetSkeletalHandMeshComponent(), HandActor->GetRootBoneName());

	if (bReturnHandBackAfterSetup) HandActor->SetActorTransform(CurrentHandTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AVRMotionControllerHand::StartFollowingPhysConstraint(bool TeleportHandToPhantomToSetupConstraint)
{
	if (!HandActor) return;

	// TODO not used yet

	FTransform HandCurrentTransform;

	if (TeleportHandToPhantomToSetupConstraint)
	{
		//HandCurrentTransform = HandActor->GetSkeletalHandMeshComponent()->GetComponentTransform();
		//TeleportHandToMotionControllerLocation(false, false);
	}

	PhysConstraint->CreateConstraint(HandActor->GetSkeletalHandMeshComponent(), HandActor->GetRootBoneName());

	bPhysConstraintAttachedToPhantomHand = true;

	if (TeleportHandToPhantomToSetupConstraint)
	{
		// Returning hand back if needed
		HandActor->GetSkeletalHandMeshComponent()->SetWorldTransform(HandCurrentTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AVRMotionControllerHand::StopFollowingPhysConstraint()
{
	if (!bPhysConstraintAttachedToPhantomHand) return;
	
	PhysConstraint->BreakConstraint();
	bPhysConstraintAttachedToPhantomHand = false;
}

void AVRMotionControllerHand::OnPawnTeleport(bool bStarted, bool bCameraViewOnly)
{
	if (!bStarted)
	{
		// TODO Attach constraint back and follow it if not already
		//StartFollowingPhantomHand(true);
		SweepHandToMotionControllerLocation(true); // After pawn teleported, teleport hand by sweeping from camera
	}
	Super::OnPawnTeleport(bStarted, bCameraViewOnly);
}

void AVRMotionControllerHand::OnTeleportWaitForPhysicsResetEnd()
{
	//HandActor->GetSkeletalHandMeshComponent()->SetSimulatePhysics(true);
	//MakeHandFollowMovementController(true);
}

void AVRMotionControllerHand::AttachPhysConstraintToPhantomHand()
{
	if (!PhysConstraint)
	{
		// Other actors may have destroyed it or it is not exists yet
		PhysConstraint = GetWorld()->SpawnActor<AHandPhysConstraint>(PhysicalHandConstraintClass);
		PhysConstraint->SetOwner(this);
		PhysConstraint->SetInstigator(OwningVRPawn);
	}

	bPhysConstraintAttachedToPhantomHand = true;
	PhysConstraint->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

/*
void AVRMotionControllerHand::ChangeHandAnimationEnum_Implementation(int32 index)
{
	// Left and Right hand animators are different so they will override this function to setup their Animation Blueprint accordingly
}*/

USkeletalMeshComponent* AVRMotionControllerHand::GetPhantomHandSkeletalMesh_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetPhantomHandSkeletalMesh()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

AHandPhysConstraint* AVRMotionControllerHand::GetPhysConstraint()
{
	return PhysConstraint;
}
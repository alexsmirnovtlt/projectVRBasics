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

	// Creating actor for physical hand
	FTransform PhantomHandTransform = GetPhantomHandSkeletalMesh()->GetComponentTransform();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	HandActor = GetWorld()->SpawnActor<AHandActor>(PhysicalHandClass, PhantomHandTransform, SpawnParams);
	HandActor->SetActorScale3D(PhantomHandTransform.GetScale3D()); // SpawnActor() ignores scale apparently
	HandActor->SetOwner(this);
	HandActor->SetInstigator(OwningVRPawn);
	HandActor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	//

	AttachPhysConstraintToMovementController(); // Creating Physical constraint that we can manage from this class and other actors

	MakeHandFollowMovementController(true); // Attaching hand to phys constraint so it will follow

	OnPhysicalHandAppearedEvent(); // Hand is placed in the world. BP may add some logic here
}

void AVRMotionControllerHand::TeleportHandToMotionControllerLocation(bool bSweepFromCamera, bool SweepToTarget)
{
	if (!HandActor) return;

	if (bSweepFromCamera)
	{
		FVector CameraWorldLocation = OwningVRPawn->GetCameraWorldLocation();
		HandActor->GetSkeletalHandMeshComponent()->SetWorldLocation(CameraWorldLocation, true, nullptr, ETeleportType::ResetPhysics);
	}

	FTransform PhantomHandTransform = GetPhantomHandSkeletalMesh()->GetComponentTransform();
	HandActor->GetSkeletalHandMeshComponent()->SetWorldTransform(PhantomHandTransform, SweepToTarget, nullptr, ETeleportType::ResetPhysics);
}

void AVRMotionControllerHand::TeleportHandToLocation(FVector WorldLocation, FRotator WorldRotation)
{
	HandActor->GetSkeletalHandMeshComponent()->SetWorldLocation(WorldLocation, false, nullptr, ETeleportType::ResetPhysics);
	HandActor->GetSkeletalHandMeshComponent()->SetWorldRotation(WorldRotation, false, nullptr, ETeleportType::ResetPhysics);
}

void AVRMotionControllerHand::MakeHandFollowMovementController(bool TeleportHandToPhantomToSetupConstraint)
{
	if (!HandActor) return;

	FTransform HandCurrentTransform;

	if (TeleportHandToPhantomToSetupConstraint)
	{
		HandCurrentTransform = HandActor->GetSkeletalHandMeshComponent()->GetComponentTransform();
		TeleportHandToMotionControllerLocation(false, false);
	}

	HandActor->GetSkeletalHandMeshComponent()->SetSimulatePhysics(true);
	PhysConstraint->CreateConstraint(HandActor->GetSkeletalHandMeshComponent(), HandActor->GetRootBoneName());

	bHaveActivePhysConstraint = true;

	if (TeleportHandToPhantomToSetupConstraint)
	{
		// Returning hand back if needed
		HandActor->GetSkeletalHandMeshComponent()->SetWorldTransform(HandCurrentTransform, false, nullptr, ETeleportType::ResetPhysics);
	}
}

void AVRMotionControllerHand::OnPawnTeleport(bool bStarted, bool bCameraViewOnly)
{
	if (bStarted)
	{
		//BreakCurrentHandConstraint();
		HandActor->GetSkeletalHandMeshComponent()->SetSimulatePhysics(false);
	}
	else
	{
		TeleportHandToMotionControllerLocation(true, true);
		
		// The easiest way to teleport hands so they dont sway after that is disable physics for some time (player`s camera is faded to black at this moment) and SetSimulatePhysics(true) again
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_TeleportPhysicsResetWait,
			this,
			&AVRMotionControllerHand::OnTeleportWaitForPhysicsResetEnd,
			TeleportWaitTimeForPhysicsReset,
			true
		);
	}

	Super::OnPawnTeleport(bStarted, bCameraViewOnly);
}

void AVRMotionControllerHand::OnTeleportWaitForPhysicsResetEnd()
{
	HandActor->GetSkeletalHandMeshComponent()->SetSimulatePhysics(true);
	//MakeHandFollowMovementController(true);
}

void AVRMotionControllerHand::BreakCurrentHandConstraint()
{
	if (bHaveActivePhysConstraint)
	{
		PhysConstraint->BreakConstraint();
		bHaveActivePhysConstraint = false;
	}
}

void AVRMotionControllerHand::AttachPhysConstraintToMovementController()
{
	if (!PhysConstraint)
	{
		// Other actors may have destroyed it or it is not exists yet
		PhysConstraint = GetWorld()->SpawnActor<AHandPhysConstraint>(PhysicalHandConstraintClass);
		PhysConstraint->SetOwner(this);
		PhysConstraint->SetInstigator(OwningVRPawn);
	}

	PhysConstraint->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

void AVRMotionControllerHand::EnableHandCollision(bool bEnable)
{
	FName NewCollisionProfileName = bEnable ? HandActor->GetActiveCollisionPresetName() : HandActor->GeNoCollisionPresetName();
	HandActor->GetSkeletalHandMeshComponent()->SetCollisionProfileName(NewCollisionProfileName);

	if (!bEnable)
	{
		// Returning Physics Collision even if we are disabling it
		HandActor->GetSkeletalHandMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	//if(bEnable) HandActor->GetSkeletalHandMeshComponent()->SetMassOverrideInKg() BONE MASS CHANGE
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
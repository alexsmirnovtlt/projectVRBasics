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

	if (HandActor)
	{
		HandActor->RemoveHandSphereCollisionCallbacks(this);
		HandActor->Destroy();
	}

	if (PhysConstraint) PhysConstraint->Destroy();

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_BeginPlayWait))
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BeginPlayWait);
	}
}

void AVRMotionControllerHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

void AVRMotionControllerHand::OnBeginPlayWaitEnd()
{
	if (!MotionController->IsTracked() || MotionController->CurrentTrackingStatus != ETrackingStatus::Tracked)
	{
		// Timer will loop for now
		UE_LOG(LogTemp, Warning, TEXT("AVRMotionControllerHand Init was skipped for now. Will retry"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BeginPlayWait);

	HandActor = GetWorld()->SpawnActor<AHandActor>(PhysicalHandClass);
	HandActor->SetOwner(this);
	HandActor->SetInstigator(OwningVRPawn);

	HandActor->SetupHandSphereCollisionCallbacks(this);

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

void AVRMotionControllerHand::ChangeHandAnimationStateEnum_Implementation(uint8 byte) const
{
	// Left and Right hand animators are different so they will override this function to setup their Animation Blueprint accordingly
}

bool AVRMotionControllerHand::IsHandInIdleState_Implementation() const
{
	return !bIsGrabbing && Axis_Trigger_Value < 0.5f && Axis_Grip_Value < 0.5f;
}

USkeletalMeshComponent* AVRMotionControllerHand::GetPhantomHandSkeletalMesh_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetPhantomHandSkeletalMesh()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

FTransform AVRMotionControllerHand::GetPointingWorldTransform_Implementation() const
{
	return GetControllerWorldOriginTransform();
}

AHandPhysConstraint* AVRMotionControllerHand::GetPhysConstraint()
{
	return PhysConstraint;
}

void AVRMotionControllerHand::HandCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UHandInteractable>()) return; // If we just cast OtherActor to IHandInteractable, Implements() will return true and Cast<IHandInteractable>(OtherActor) will return nullptr because we added interface in BP and not in cpp class

	OverlappingActorsArray.Add(OtherActor);
	IHandInteractable::Execute_OnCanBeGrabbedByHand_Start(OtherActor, this, OtherComp);
	//IHandInteractable::OnCanBeGrabbedByHand_Start(OtherActor, this, OtherComp);

	// TODO this and EndOverlap gets triggered a lot more than needed. Custom Collision presets with custom object types are better suited for that
	//UE_LOG(LogTemp, Warning, TEXT("BeginOverlap %s --- %s"), *OtherActor->GetName(), *OtherComp->GetName());
}

void AVRMotionControllerHand::HandCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->Implements<UHandInteractable>()) return;
	
	OverlappingActorsArray.Remove(OtherActor);
	IHandInteractable::Execute_OnCanBeGrabbedByHand_End(OtherActor, this, OtherComp);
	//IHandInteractable::OnCanBeGrabbedByHand_End(OtherActor, this, OtherComp);

	//UE_LOG(LogTemp, Warning, TEXT("EndOverlap %s --- %s"), *OtherActor->GetName(), *OtherComp->GetName());
}



int32 AVRMotionControllerHand::GetClosestGrabbableActorIndex() const
{
	if (OverlappingActorsArray.Num() == 0) return -1;
	else if (OverlappingActorsArray.Num() == 1) return 0;

	int32 IndexToReturn = 0;
	float CurrentDistance = IHandInteractable::Execute_GetWorldSquaredDistanceToMotionController(OverlappingActorsArray[0], this);

	for (int32 i = 1; i < OverlappingActorsArray.Num(); ++i)
	{
		float Distance = IHandInteractable::Execute_GetWorldSquaredDistanceToMotionController(OverlappingActorsArray[i], this);
		if (Distance < CurrentDistance)
		{
			IndexToReturn = i;
			CurrentDistance = Distance;
		}
	}

	return IndexToReturn;
}
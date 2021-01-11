// Alex Smirnov 2020-2021


#include "VRMotionControllerHand.h"

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "MotionControllerComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "HandActor.h"
#include "VirtualRealityPawn.h"
#include "HandPhysConstraint.h"

#include "Interfaces/VRPlayerInput.h"
#include "Interfaces/HandInteractable.h"


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

	// Waiting to give headset some time to update motion controller location so it wont move to pawn`s local FVector::ZeroVector on spawn 
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

	TryToReleaseGrabbedActor();

	if (HandActor)
	{
		HandActor->RemoveHandSphereCollisionCallbacks(this);
		HandActor->Destroy();
	}

	if (PhysConstraint) PhysConstraint->Destroy();

	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_BeginPlayWait)) GetWorld()->GetTimerManager().ClearTimer(TimerHandle_BeginPlayWait);
	if (GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_NoCollisionOnDropWait)) GetWorld()->GetTimerManager().ClearTimer(TimerHandle_NoCollisionOnDropWait);
}

void AVRMotionControllerHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAttachmentIsInTransitionToHand) UpdateAttachedActorLocation(DeltaTime); // If we grabbed something, updating its location here until it reaches its destination
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
	StartFollowingPhysConstraint(bReturnHandBackAfterSetup);
}

void AVRMotionControllerHand::StartFollowingPhysConstraint(bool bReturnHandBackAfterSetup)
{
	if (!HandActor || !PhysConstraint) return;
	bHandFollowsController = bPhysConstraintAttachedToPhantomHand;

	FTransform CurrentHandTransform = HandActor->GetTransform();

	HandActor->SetActorTransform(GetPhantomHandSkeletalMesh()->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	PhysConstraint->CreateConstraint(HandActor->GetSkeletalHandMeshComponent(), HandActor->GetRootBoneName());

	if (bReturnHandBackAfterSetup) HandActor->SetActorTransform(CurrentHandTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AVRMotionControllerHand::StopFollowingPhysConstraint()
{
	if (!bPhysConstraintAttachedToPhantomHand || !PhysConstraint) return;
	
	PhysConstraint->BreakConstraint();
	bPhysConstraintAttachedToPhantomHand = false;
}

void AVRMotionControllerHand::OnPawnTeleport(bool bStarted, bool bCameraViewOnly)
{
	if (bStarted)
	{
		if(ConnectedActorWithHandInteractableInterface) IHandInteractable::Execute_OnHandTeleported(ConnectedActorWithHandInteractableInterface, this);
	}
	else
	{
		if (!bPhysConstraintAttachedToPhantomHand) AttachPhysConstraintToPhantomHand();
		SweepHandToMotionControllerLocation(true); // After pawn teleported, teleport hand by sweeping from camera
		if (!bHandFollowsController) StartFollowingPhantomHand(true);
	}

	Super::OnPawnTeleport(bStarted, bCameraViewOnly);
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

	auto ParentActor = PhysConstraint->GetAttachParentActor();
	if (ParentActor == nullptr || ParentActor != this)
	{
		PhysConstraint->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}

	bPhysConstraintAttachedToPhantomHand = true;
}

void AVRMotionControllerHand::ChangeHandAnimationStateEnum_Implementation(uint8 byte) const
{
	// Left and Right hand animators are different so they will override this function to setup their Animation Blueprint accordingly
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function ChangeHandAnimationStateEnum()"), *this->GetClass()->GetFName().ToString());
}

bool AVRMotionControllerHand::IsHandInIdleState_Implementation() const
{
	return !bIsGrabbing;
}

USkeletalMeshComponent* AVRMotionControllerHand::GetPhantomHandSkeletalMesh_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetPhantomHandSkeletalMesh()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

AHandPhysConstraint* AVRMotionControllerHand::GetPhysConstraint()
{
	bPhysConstraintAttachedToPhantomHand = false; // This function is public so every time some actor want to detach and/or move constraint we consider it detached for logic purposes such as teleport
	return PhysConstraint;
}

void AVRMotionControllerHand::HandCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UHandInteractable>()) return; // If we just cast OtherActor to IHandInteractable, Implements() will return true and Cast<IHandInteractable>(OtherActor) will return nullptr because we added interface in BP and not in cpp class
	// This and EndOverlap gets triggered a lot more than needed. Consider using custom collision presets with custom object types to reduce unnecessary calls
	OverlappingActorsArray.Add(OtherActor);
	IHandInteractable::Execute_OnCanBeGrabbedByHand_Start(OtherActor, this, OtherComp);

	//UE_LOG(LogTemp, Warning, TEXT("BeginOverlap --- OtherActor:%s --- OtherComp:%s"), *OtherActor->GetName(), *OtherComp->GetName());
}

void AVRMotionControllerHand::HandCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->Implements<UHandInteractable>()) return;
	
	OverlappingActorsArray.Remove(OtherActor);
	IHandInteractable::Execute_OnCanBeGrabbedByHand_End(OtherActor, this, OtherComp);

	//UE_LOG(LogTemp, Warning, TEXT("EndOverlap --- OtherActor:%s --- OtherComp:%s"), *OtherActor->GetName(), *OtherComp->GetName());
}

AActor* AVRMotionControllerHand::GetActorToForwardInputTo()
{
	AActor* ActorToForwardInputTo = Super::GetActorToForwardInputTo();
	if (ActorToForwardInputTo == nullptr && bGrabbedObjectImplementsPlayerInputInterface)
	{
		return ConnectedActorWithHandInteractableInterface;
	}

	return ActorToForwardInputTo;
}

bool AVRMotionControllerHand::CanDoPointingChecks() const
{
	// When this Actor is created, Tick functions immediately needs PointingTransform that this Actors is not having yet.
	// And there is some special cases when we dont need ponting checks (f.e when hand is detached)
	if (!HandActor || bIsAttachmentIsInTransitionToHand || !bPhysConstraintAttachedToPhantomHand) return false;

	return true;
}

// BEGIN Logic Related to interaction with IHandInteractable Objects

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

bool AVRMotionControllerHand::TryToGrabActor()
{
	if (bIsAttachmentIsInTransitionToHand) return false;

	// Check if we have grabbed object already and it drops on second Grab Event, so drop it instead of grab
	if (bIsGrabbing && ConnectedActorWithHandInteractableInterface && IHandInteractable::Execute_IsRequiresSecondButtonPressToDrop(ConnectedActorWithHandInteractableInterface))
	{
		TryToReleaseGrabbedActor(true);
		return false;
	}

	if (bIsGrabbing) return false;

	int32 ActorIndex = GetClosestGrabbableActorIndex();
	if (ActorIndex == -1) return false;

	if (!IHandInteractable::Execute_CanBeGrabbed(OverlappingActorsArray[ActorIndex])) return false;

	bIsGrabbing = true;
	bIsAttachmentIsInTransitionToHand = true;

	ConnectedActorWithHandInteractableInterface = OverlappingActorsArray[ActorIndex];
	IHandInteractable::Execute_OnGrab(ConnectedActorWithHandInteractableInterface, this);

	return true;
}

bool AVRMotionControllerHand::TryToReleaseGrabbedActor(bool bForceRelease)
{
	if (bIsAttachmentIsInTransitionToHand) { bIsGrabbing = false; return false; } // Special case when we just grabbed actor that on its way to hand to be attached

	int32 ActorIndex = GetClosestGrabbableActorIndex();

	if (!ConnectedActorWithHandInteractableInterface)
	{
		// Case when we pressed grab when nothing was around to grab then moved hand close to grabbable item and reseased grab
		if (ActorIndex != -1) IHandInteractable::Execute_OnCanBeGrabbedByHand_Start(OverlappingActorsArray[ActorIndex], this, nullptr);

		return false;
	}

	// Same as TryToGrabActor(), if grabbed actor cannot be dropped on input release, skipping drop
	if (!bForceRelease && IHandInteractable::Execute_IsRequiresSecondButtonPressToDrop(ConnectedActorWithHandInteractableInterface)) return false;

	if (!IHandInteractable::Execute_CanBeDropped(ConnectedActorWithHandInteractableInterface)) return false;

	bIsGrabbing = false;

	IHandInteractable::Execute_OnDrop(ConnectedActorWithHandInteractableInterface, this);
	ConnectedActorWithHandInteractableInterface = nullptr;
	bGrabbedObjectImplementsPlayerInputInterface = false;

	// Case when we dropped actor but there is more actors in the vicinity to grab. Notify closest one
	if (ActorIndex != -1) IHandInteractable::Execute_OnCanBeGrabbedByHand_Start(OverlappingActorsArray[ActorIndex], this, nullptr);

	// Disabling collision while dropping actor so it can drop or be thrown correctly
	HandActor->ChangeHandPhysProperties(false, true);
	// Returning collision after some time
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_NoCollisionOnDropWait,
		this,
		&AVRMotionControllerHand::OnNoCollisionOnDropTimerEnd,
		NoCollisionOnDropSec,
		true
	);

	return true;
}

void AVRMotionControllerHand::StartMovingActorToHandForAttachment(AActor* ActorToAttach, FVector RelativeToMotionControllerLocation, FRotator RelativeToMotionControllerRotation)
{
	if (!HandActor) return;
	if (!ActorToAttach->Implements<UHandInteractable>())
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to attach actor '%s' to hand, but no IHandInteractable interface was found!"), *ActorToAttach->GetName());
		return;
	}

	ConnectedActorWithHandInteractableInterface = ActorToAttach;

	auto HandAttachmentComponent = HandActor->GetActorAttachmentComponent();
	HandAttachmentComponent->SetRelativeLocationAndRotation(RelativeToMotionControllerLocation, RelativeToMotionControllerRotation);
	
	HandActor->ChangeHandPhysProperties(false, true);

	InitialAttachmentTransform = ActorToAttach->GetActorTransform();
}

void AVRMotionControllerHand::UpdateAttachedActorLocation(float DeltaTime)
{
	if (!ConnectedActorWithHandInteractableInterface || !HandActor) return;

	if (CurrentAttachmentLerpValue >= 1.0f) // Finalize attachment
	{
		FAttachmentTransformRules AttachmentTransformRules = FAttachmentTransformRules::KeepWorldTransform;
		AttachmentTransformRules.bWeldSimulatedBodies = true;

		ConnectedActorWithHandInteractableInterface->AttachToActor(HandActor, AttachmentTransformRules);

		IHandInteractable::Execute_OnFinishedAttachingToHand(ConnectedActorWithHandInteractableInterface);

		CurrentAttachmentLerpValue = 0.f;
		bIsAttachmentIsInTransitionToHand = false;
		HandActor->ChangeHandPhysProperties(true, true);

		if (!bIsGrabbing) TryToReleaseGrabbedActor(true); // If input to drop was executed, drop it now
		else bGrabbedObjectImplementsPlayerInputInterface = ConnectedActorWithHandInteractableInterface->Implements<UVRPlayerInput>(); // Making so grabbed object may use and consume Player Input
	}
	else // Keep updating attached actor location so it will be attached later
	{
		CurrentAttachmentLerpValue = FMath::Min(1.0f, CurrentAttachmentLerpValue + DeltaTime / AttachmentTimeSec);

		FTransform TargetTransform = HandActor->GetActorAttachmentComponent()->GetComponentTransform();
		TargetTransform.SetScale3D(InitialAttachmentTransform.GetScale3D());

		TargetTransform.SetLocation(FMath::Lerp(InitialAttachmentTransform.GetLocation(), TargetTransform.GetLocation(), CurrentAttachmentLerpValue));
		TargetTransform.SetRotation(FMath::Lerp(InitialAttachmentTransform.GetRotation(), TargetTransform.GetRotation(), CurrentAttachmentLerpValue));

		ConnectedActorWithHandInteractableInterface->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::ResetPhysics);
	}
}

void AVRMotionControllerHand::OnNoCollisionOnDropTimerEnd()
{
	HandActor->ChangeHandPhysProperties(true, true);
}

// END Logic Related to interaction with IHandInteractable Objects
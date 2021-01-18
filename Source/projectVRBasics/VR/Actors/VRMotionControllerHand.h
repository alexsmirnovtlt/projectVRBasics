// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "VirtualRealityMotionController.h"

#include "VRMotionControllerHand.generated.h"

class AHandActor;
class AHandPhysConstraint;
class USkeletalMeshComponent;

/**
 * 
 */
UCLASS()
class PROJECTVRBASICS_API AVRMotionControllerHand : public AVirtualRealityMotionController
{
	GENERATED_BODY()
	
public:
	AVRMotionControllerHand();

protected:

	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;

public:

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void TeleportHandToLocation(FVector WorldLocation, FRotator WorldRotation);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StartFollowingPhantomHand(bool bReturnHandBackAfterSetup);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StartFollowingPhysConstraint(bool bReturnHandBackAfterSetup);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StopFollowingPhysConstraint();
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hand Motion Controller")
	AHandPhysConstraint* GetPhysConstraint();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	void ChangeHandAnimationStateEnum(uint8 byte) const;

	 // Is hand able to grab or interact with collided actors that implements IHandInteractable
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Override")
	bool IsHandInIdleState() const;
	// World Location of Phantom Hand or Motion Controller itself (if was not overridden)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Override")
	FTransform GetPhantomHandWorldTransform() const; // TODO actually GetControllerWorldOriginTransform() is kind of the same but this was made especially to the location that not realated to the hand itself, but the controller.

	UFUNCTION()
	void HandCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

	void AttachPhysConstraintToPhantomHand();
	void SweepHandToMotionControllerLocation(bool bSweepFromCamera);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	USkeletalMeshComponent* GetPhantomHandSkeletalMesh() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand Motion Controller")
	float MotionControllerCheckInterval;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand Motion Controller")
	TSubclassOf<AHandActor> PhysicalHandClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand Motion Controller")
	TSubclassOf<AHandPhysConstraint> PhysicalHandConstraintClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "Hand Motion Controller Events")
	void OnPhysicalHandAppearedEvent();

	virtual void OnPawnTeleport(bool bStarted, bool bCameraViewOnly) override;

	UPROPERTY(BlueprintReadOnly)
	AHandActor* HandActor;

	UPROPERTY(BlueprintReadOnly)
	AHandPhysConstraint* PhysConstraint;

	UPROPERTY(BlueprintReadWrite, Category = "Hand Motion Controller")
	bool bHandFollowsController = false;

	virtual AActor* GetActorToForwardInputTo() override;
	virtual bool CanDoPointingChecks() const;

	// BEGIN Logic Related to interaction with IHandInteractable Objects
public:

	UPROPERTY(BlueprintReadOnly, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool bIsAttachmentIsInTransitionToHand = false;

	// Trying to find closest overlapping actor that implements IHandInteractable and Grab it
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool TryToGrabActor();
	// Drop actor if hand is currently holding one. bForceRelease::True will drop regardless of IHandInteractable values IsRequiresSecondButtonPressToDrop() or IsDropDisabled(). So True should be used only if those were overridden
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool TryToReleaseGrabbedActor(bool bForceRelease = false);

protected:

	bool bIsGrabbing = false;

	UPROPERTY()
	FTransform InitialAttachmentTransform;

	UPROPERTY()
	float CurrentAttachmentLerpValue = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	float AttachmentTimeSec = 0.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	float NoCollisionOnDropSec = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	AActor* ConnectedActorWithHandInteractableInterface;
	// To pick up objects more accurately we keep array of them and ask them to provide distance to it so closest one will be picked up
	UPROPERTY(BlueprintReadOnly, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	TArray<AActor*> OverlappingActorsArray; // TODO May need additional testing if all actors are correctly removed from this array

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	int32 GetClosestGrabbableActorIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	void StartMovingActorToHandForAttachment(AActor* ActorToAttach, FVector RelativeToMotionControllerLocation, FRotator RelativeToMotionControllerRotation);

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	void UpdateAttachedActorLocation(float DeltaTime);

	bool bGrabbedObjectImplementsPlayerInputInterface;

	// END Logic Related to interaction with IHandInteractable Objects

private:

	FTimerHandle TimerHandle_BeginPlayWait; 
	FTimerHandle TimerHandle_NoCollisionOnDropWait;

	UFUNCTION()
	void OnBeginPlayWaitEnd();
	UFUNCTION()
	void OnNoCollisionOnDropTimerEnd();

	bool bPhysConstraintAttachedToPhantomHand = false;
};

// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "VirtualRealityMotionController.h"

#include "Interfaces/HandInteractable.h"

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
	void AttachPhysConstraintToPhantomHand();
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void SweepHandToMotionControllerLocation(bool bSweepFromCamera);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void TeleportHandToLocation(FVector WorldLocation, FRotator WorldRotation);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StartFollowingPhantomHand(bool bReturnHandBackAfterSetup);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StartFollowingPhysConstraint(bool TeleportHandToPhantomToSetupConstraint);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void StopFollowingPhysConstraint();
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	class AHandPhysConstraint* GetPhysConstraint();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	void ChangeHandAnimationStateEnum(uint8 byte) const;

	 // What location and rotation we need to cast a ray from to determine which IHandInteractable actor are we pointing to. If hand is in Idle state, could be Arrow`s Transform. If we are holding something, might be a different transform
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FTransform GetPointingWorldTransform() const;
	FTransform GetPointingWorldTransform_Implementation() const;

	 // Is hand able to grab or interact with collided actors that implements IHandInteractable
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	bool IsHandInIdleState() const;

	UFUNCTION()
	void HandCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandCollisionSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

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
	bool bHandFollowsController = false; // TODO Check if its even in use anywhere

	// BEGIN Logic Related to interaction with IHandInteractable Objects
public:
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	bool IsHandInGrabState() { return bIsGrabbing; }

protected:

	UPROPERTY(BlueprintReadWrite, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool bIsGrabbing = false;
	UPROPERTY(BlueprintReadOnly, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool bIsAttachmentIsInTransitionToHand = false;

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

	UPROPERTY(BlueprintReadOnly, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	TArray<AActor*> OverlappingActorsArray;

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	int32 GetClosestGrabbableActorIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool TryToGrabActor();

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	void StartMovingActorToHandForAttachment(AActor* ActorToAttach, FVector RelativeToMotionControllerLocation, FRotator RelativeToMotionControllerRotation);

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	void UpdateAttachedActorLocation(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller - Interaction with IHandInteractable")
	bool TryToReleaseGrabbedActor(bool bForceRelease = false);

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

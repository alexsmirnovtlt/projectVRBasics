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
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	//void ChangeHandAnimationEnum(int32 index);

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

	bool bHandFollowsController;

private:

	FTimerHandle TimerHandle_BeginPlayWait;
	FTimerHandle TimerHandle_TeleportPhysicsResetWait;

	UFUNCTION()
	void OnBeginPlayWaitEnd();
	UFUNCTION()
	void OnTeleportWaitForPhysicsResetEnd();

	float TeleportWaitTimeForPhysicsReset = 0.1f;
	bool bPhysConstraintAttachedToPhantomHand = false;
};

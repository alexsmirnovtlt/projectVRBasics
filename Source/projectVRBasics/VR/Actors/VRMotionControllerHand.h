// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "VirtualRealityMotionController.h"

#include "VRMotionControllerHand.generated.h"

class AHandActor;
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
	void TeleportHandToMotionControllerLocation(bool bSweepFromCamera, bool SweepToTarget = true);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void TeleportHandToLocation(FVector WorldLocation, FRotator WorldRotation);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void MakeHandFollowMovementController(bool TeleportHandToPhantomToSetupConstraint);
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void BreakCurrentHandConstraint();
	UFUNCTION(BlueprintCallable, Category = "Hand Motion Controller")
	void EnableHandCollision(bool bEnable);
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	//void ChangeHandAnimationEnum(int32 index);

protected:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	USkeletalMeshComponent* GetPhantomHandSkeletalMesh() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	UPhysicsConstraintComponent* GetPhysicsConstraint() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	UStaticMeshComponent* GetFirstPhysicsConstraintComponent() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand Motion Controller")
	float MotionControllerCheckInterval;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand Motion Controller")
	TSubclassOf<AHandActor> PhysicalHandClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "Hand Motion Controller Events")
	void OnPhysicalHandAppearedEvent();

	virtual void OnPawnTeleport(bool bStarted, bool bCameraViewOnly) override;

	UPROPERTY(BlueprintReadOnly)
	AHandActor* HandActor;

	bool bHandFollowsController;

private:

	FTimerHandle TimerHandle_BeginPlayWait;
	FTimerHandle TimerHandle_TeleportPhysicsResetWait;
	UFUNCTION()
	void OnBeginPlayWaitEnd();
	void OnTeleportWaitForPhysicsResetEnd();

	void CreatePhysicalHand();

	float TeleportWaitTimeForPhysicsReset = 0.1f;
	float MinSquaredDistanceToSpawnPhysicalHand = 400.f;
	bool bHaveActivePhysConstraint = false;
};

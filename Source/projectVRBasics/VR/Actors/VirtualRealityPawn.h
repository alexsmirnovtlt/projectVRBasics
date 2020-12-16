// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "VirtualRealityPawn.generated.h"

class UCameraComponent;
class IXRTrackingSystem;
class UCapsuleComponent;
class AVirtualRealityMotionController;

struct FStreamableHandle;

USTRUCT(Blueprintable)
struct FControllerType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName HeadsetName;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AVirtualRealityMotionController> LeftController;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AVirtualRealityMotionController> RightController;
};

UCLASS()
class PROJECTVRBASICS_API AVirtualRealityPawn : public APawn
{
	GENERATED_BODY()

public:
	AVirtualRealityPawn();

protected:

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

public:

	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	FName GetCurrentControllersTypeName() const;

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void AddCameraYawRotation(float YawToAdd); // used in teleport state when player may move view left or right. Pawn itself should not be affected by this view change
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void TeleportToLocation(FVector NewLocation, FRotator NewRotation); //  used in teleport state when player teleports
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FVector GetCameraRelativeLocation() const;
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FRotator GetCameraRelativeRotation() const;
	
	// bool that can be accessible in Motion Controller BPs
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "VR Setup")
	bool RightControllerIsPrimary;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup") // If not empty, overrides controller type that will be used and gets value from ControllerTypes. 
	FName StartingControllerName;  // If is empty, headset info will be used to determine Headset Type. (f.e if we use same hands on any Headset, we define it here and in ControllerTypes)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	TArray<FControllerType> ControllerTypes;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	float StartFadeTimeSec;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCapsuleComponent* PawnRootComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* VRRootComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCameraComponent* MainCamera;
	UPROPERTY()
	AVirtualRealityMotionController* LeftHand;
	UPROPERTY()
	AVirtualRealityMotionController* RightHand;

	FName CurrentControllersTypeName;

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false);;

	bool InitHeadset(IXRTrackingSystem& TrackingSystem);
	void InitMotionControllers(IXRTrackingSystem& TrackingSystem);
	void CreateMotionController(bool bLeft, UClass* ClassToCreate);

	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	bool SwitchMotionControllersByName(FName NewProfileName);
	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	void SwitchMotionControllersByClass(TSoftClassPtr<AVirtualRealityMotionController> LeftHandClassSoftObjPtr, TSoftClassPtr<AVirtualRealityMotionController> RightHandClassSoftObjPtr);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Headset Error Handling")
	FName StartupLevelName = TEXT("StartUpVRMap"); // If headset is not found, load this level

	void OnHandAssetLoadDone(bool bLeft, bool bUseForBothHands);
	UFUNCTION()
	void OnStartTimerEnd();

	FTimerHandle TimerHandle_StartCameraFade;

	TSharedPtr<FStreamableHandle> LeftHandStreamableHandle;
	TSharedPtr<FStreamableHandle> RightHandStreamableHandle;
};

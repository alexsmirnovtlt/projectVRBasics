// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Interfaces/VRPlayerInput.h"

#include "VirtualRealityPawn.generated.h"

class UCameraComponent;
class IXRTrackingSystem;
class UCapsuleComponent;
class AVirtualRealityMotionController;

struct FStreamableHandle;

DECLARE_DELEGATE_OneParam(InputActionType, EButtonActionType);

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
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:

	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	FName GetCurrentControllersTypeName() const;

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void AddCameraYawRotation(float YawToAdd); // used in teleport state when player may move view left or right. Pawn itself should not be affected by this view change
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void TeleportToLocation(FVector NewLocation, FRotator NewRotation); //  used in teleport state when player teleports
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FTransform GetCameraRelativeTransform() const;
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FTransform GetCameraWorldTransform() const;

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

	bool InitHeadset(IXRTrackingSystem* TrackingSystem);
	void InitMotionControllers(IXRTrackingSystem* TrackingSystem);
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

	// Input bindings
	// Binding Input one time so controller states and objects that were grabbed by hand should not receive any input themselves and just implement IVRPlayerInputInterface BP events.
	// Input gets received by a Pawn, then Pawn calls Interface functions on Virtual Reality Motion Controller class. Then VRMotionController may forward them to states and grabbed objects.

	// Downside of this implementation is there is a lot of boilerplate code
	// But upsides are:
	// - Only 10 nodes in BPs: 3 axies and 7 button actions (because left hand state has no need to know about right hand input and so on). Full controll on what button was pressed/touched/released
	// - Changing ue4 Input Settings will not affect any BPs
	// - Custom thresholds for Touch and Press Events (currently for Trigger and Grip buttons)
	// As the result ue4 input is completely detached from any logic in BP regarding input

	protected:
		static const FName InputBindingName_Axis_Right_Thumbstick_X;
		static const FName InputBindingName_Axis_Right_Thumbstick_Y;
		static const FName InputBindingName_Axis_Left_Thumbstick_X;
		static const FName InputBindingName_Axis_Left_Thumbstick_Y;
		static const FName InputBindingName_Axis_Right_Trigger;
		static const FName InputBindingName_Axis_Left_Trigger;
		static const FName InputBindingName_Axis_Right_Grip;
		static const FName InputBindingName_Axis_Left_Grip;

		static const FName InputBindingName_Button_Action_Button_Left_Primary_Press;
		static const FName InputBindingName_Button_Action_Button_Left_Secondary_Press;
		static const FName InputBindingName_Button_Action_Button_Right_Primary_Press;
		static const FName InputBindingName_Button_Action_Button_Right_Secondary_Press;
		static const FName InputBindingName_Button_Action_Button_Left_Primary_Touch;
		static const FName InputBindingName_Button_Action_Button_Left_Secondary_Touch;
		static const FName InputBindingName_Button_Action_Button_Right_Primary_Touch;
		static const FName InputBindingName_Button_Action_Button_Right_Secondary_Touch;
		static const FName InputBindingName_Button_Action_Button_Right_Trigger_Touch;
		static const FName InputBindingName_Button_Action_Button_Left_Trigger_Touch;
		//static const FName InputBindingName_Button_Action_Button_Right_Trigger_Press;
		//static const FName InputBindingName_Button_Action_Button_Left_Trigger_Press;
		static const FName InputBindingName_Button_Action_Button_Right_Grip_Touch;
		static const FName InputBindingName_Button_Action_Button_Left_Grip_Touch;
		//static const FName InputBindingName_Button_Action_Button_Right_Grip_Press;
		//static const FName InputBindingName_Button_Action_Button_Left_Grip_Press;
		static const FName InputBindingName_Button_Action_Button_Right_Thumbstick_Touch;
		static const FName InputBindingName_Button_Action_Button_Left_Thumbstick_Touch;
		static const FName InputBindingName_Button_Action_Button_Right_Thumbstick_Press;
		static const FName InputBindingName_Button_Action_Button_Left_Thumbstick_Press;
		static const FName InputBindingName_Button_Action_Button_Menu;
		static const FName InputBindingName_Button_Action_Button_System;

		UFUNCTION() void Input_Axis_Right_Thumbstick_X(float Value);
		UFUNCTION() void Input_Axis_Right_Thumbstick_Y(float Value);
		UFUNCTION() void Input_Axis_Left_Thumbstick_X(float Value);
		UFUNCTION() void Input_Axis_Left_Thumbstick_Y(float Value);
		UFUNCTION() void Input_Axis_Right_Trigger(float Value);
		UFUNCTION() void Input_Axis_Left_Trigger(float Value);
		UFUNCTION() void Input_Axis_Right_Grip(float Value);
		UFUNCTION() void Input_Axis_Left_Grip(float Value);

		UFUNCTION() void Input_Button_Left_Primary(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Left_Secondary(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Right_Primary(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Right_Secondary(EButtonActionType EventType);

		UFUNCTION() void Input_Button_Right_Trigger_Touch(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Left_Trigger_Touch(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Right_Grip_Touch(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Left_Grip_Touch(EButtonActionType EventType);

		UFUNCTION() void Input_Button_Left_Thumbstick(EButtonActionType EventType);
		UFUNCTION() void Input_Button_Right_Thumbstick(EButtonActionType EventType);

		UFUNCTION() void Input_Button_Menu(EButtonActionType EventType); // these and System_Press are not working for Oculus Rift S. Looks like steam VR and Oculus Home consumes those inputs. Maybe?
		UFUNCTION() void Input_Button_System(EButtonActionType EventType);

		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup")
		bool bIsTriggerCapacitive = true; // TODO make false and create custom settings for every headset. Maybe move this to controller class
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup")
		bool bIsGripCapacitive = false;
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup", meta = (EditCondition = "!bIsTriggerCapacitive"))
		float AxisTriggerTouchThreshold = 0.0001f;
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup", meta = (EditCondition = "!bIsGripCapacitive"))
		float AxisGripTouchThreshold = 0.01f;
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup")
		float AxisTriggerPressThreshold = 0.95f;
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Input Setup")
		float AxisGripPressThreshold = 0.95f;
	private:
		bool bRightTriggerPressed = false;
		bool bRightTriggerTouched = false;
		bool bRightGripPressed = false;
		bool bRightGripTouched = false;

		bool bLeftTriggerPressed = false;
		bool bLeftTriggerTouched = false;
		bool bLeftGripPressed = false;
		bool bLeftGripTouched = false;
};

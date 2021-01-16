// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interfaces/VRPlayerInput.h"

#include "VirtualRealityMotionController.generated.h"

class AVirtualRealityPawn;
class UControllerState;
class USplineComponent;


UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AVirtualRealityMotionController : public AActor, public IVRPlayerInput
{
	GENERATED_BODY()
	
public:	
	AVirtualRealityMotionController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

public:	

	void InitialSetup(AVirtualRealityPawn* Owner, bool IsLeft, bool IsPrimary);
	void PairControllers(AVirtualRealityMotionController* AnotherMotionController);

	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeState(TSubclassOf<UControllerState> NewStateClass, bool NotifyPairedControllerIfAble = true); 
	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeToDefaultState(bool NotifyPairedControllerIfAble = true);

	// Location and Rotation of point in space from where we cast rays or draw splines to perform various checks (where teleport to or at what actor contoller are we pointing)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FTransform GetControllerWorldOriginTransform() const;
	virtual FTransform GetControllerWorldOriginTransform_Implementation() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	USplineComponent* GetSplineComponent() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Motion Controller")
	bool IsRightHandController();

	virtual void OnPawnTeleport(bool bStarted, bool bCameraViewOnly);
	UFUNCTION(BlueprintImplementableEvent, Category = "Motion Controller Events")
	void OnPawnTeleportEvent(bool bStarted, bool bCameraViewOnly);

	UFUNCTION()
	UControllerState* GetControllerState() const;

	UFUNCTION(BlueprintCallable, Category = "Motion Controller")
	AVirtualRealityPawn* GetVRPawn() const;

	// What location and rotation we need to cast a ray from to determine which IHandInteractable actor are we pointing to. If hand is in Idle state, could be Arrow`s Transform. If we are holding something, might be a different transform
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FTransform GetPointingWorldTransform() const;
	FTransform GetPointingWorldTransform_Implementation() const;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup")
	TSubclassOf<UControllerState> StartStateClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup - Pointing")
	FName PointingRaycastProfileName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup - Pointing")
	float PointingMaxDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Motion Controller")
	class UMotionControllerComponent* MotionController;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	AVirtualRealityPawn* OwningVRPawn;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	UControllerState* ControllerState;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	bool IsControllerPrimary;

	UFUNCTION(BlueprintImplementableEvent, Category = "Motion Controller Events")
	void OnDoneInitByPawn();

	UFUNCTION()
	virtual AActor* GetActorToForwardInputTo();
	UFUNCTION()
	virtual bool CanDoPointingChecks() const { return true; };

	bool IsRightController;

	UPROPERTY()
	USplineComponent* SplineComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Controller")
	TWeakObjectPtr<AActor> PointedAtActorWithPointableInterface;
	UPROPERTY(BlueprintReadWrite, Category = "Motion Controller")
	bool bPointedAtActorImplementsInputInterface;// Valid only if PointedAtActorWithPointableInterface implements both interfaces: UVRPlayerInput and UControllerPointable

	UFUNCTION()
	void UpdateActorThatItPointsTo();

	// BEGIN Input from Pawn implementation 
public:
	void PawnInput_Axis_Thumbstick_X(float Value);
	void PawnInput_Axis_Thumbstick_Y(float Value);
	void PawnInput_Axis_Trigger(float Value);
	void PawnInput_Axis_Grip(float Value);

	void PawnInput_Button_Primary(EButtonActionType ActionType);
	void PawnInput_Button_Secondary(EButtonActionType ActionType);
	void PawnInput_Button_Thumbstick(EButtonActionType ActionType);
	void PawnInput_Button_Trigger(EButtonActionType ActionType);
	void PawnInput_Button_Grip(EButtonActionType ActionType);
	void PawnInput_Button_Menu(EButtonActionType ActionType);
	void PawnInput_Button_System(EButtonActionType ActionType);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Thumbstick_X_Value = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Thumbstick_Y_Value = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Trigger_Value = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Grip_Value = 0.f;
	// END Input from Pawn implementation */
};

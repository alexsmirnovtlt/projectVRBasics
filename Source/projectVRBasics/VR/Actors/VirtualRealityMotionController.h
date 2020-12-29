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

public:	

	void InitialSetup(AVirtualRealityPawn* Owner, bool IsLeft, bool IsPrimary);
	void PairControllers(AVirtualRealityMotionController* AnotherMotionController);

	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeState(TSubclassOf<UControllerState> NewStateClass, bool NotifyPairedControllerIfAble = true); 
	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeToDefaultState(bool NotifyPairedControllerIfAble = true);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FVector GetControllerWorldOriginLocation() const;
	virtual FVector GetControllerWorldOriginLocation_Implementation() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FRotator GetControllerWorldOriginRotation() const;
	virtual FRotator GetControllerWorldOriginRotation_Implementation() const;
	//For that 2 functions above we are making sure that Origin will return this Actor location and rotation but it may be overridden in BP. So joystick controller will return its MotionController location and hand controler will return its arrow location
	
	//UFUNCTION(BlueprintCallable, Category = "Motion Controller")
	//FTransform GetMotionControllerRelativeTransform() const;

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

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup")
	TSubclassOf<UControllerState> StartStateClass;

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

	bool IsRightController;

	UPROPERTY()
	USplineComponent* SplineComponent;

	UPROPERTY()
	TScriptInterface<IVRPlayerInput> ConnectedActorWithInputInterface;

	// BEGIN Input from Pawn implementation 
public:
	UFUNCTION() void PawnInput_Axis_Thumbstick_X(float Value);
	UFUNCTION() void PawnInput_Axis_Thumbstick_Y(float Value);
	UFUNCTION() void PawnInput_Axis_Trigger(float Value);
	UFUNCTION() void PawnInput_Axis_Grip(float Value);

	UFUNCTION() void PawnInput_Button_Primary(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_Secondary(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_Thumbstick(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_Trigger(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_Grip(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_Menu(EButtonActionType ActionType);
	UFUNCTION() void PawnInput_Button_System(EButtonActionType ActionType);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Thumbstick_X = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Controller Input")
	float Axis_Thumbstick_Y = 0.f;
	// END Input from Pawn implementation */
};

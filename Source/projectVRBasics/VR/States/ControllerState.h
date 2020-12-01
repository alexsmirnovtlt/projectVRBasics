// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ControllerState.generated.h"

UENUM(BlueprintType)
enum class EButtonActionType : uint8 {
	Touched = 0 UMETA(DisplayName = "Touched"),
	Pressed = 1 UMETA(DisplayName = "Pressed"),
	ReleasedPress = 2 UMETA(DisplayName = "Released Press"),
	ReleasedTouch = 3 UMETA(DisplayName = "Released Touch")
};

class AVirtualRealityMotionController;

/**
 * 
 */
UCLASS(Blueprintable)
class PROJECTVRBASICS_API UControllerState : public UObject
{
	GENERATED_BODY()

public:

	void SetOwningController(AVirtualRealityMotionController* MotionController);
	void SetOtherControllerReference(UControllerState* OtherControllerReference);

	//UFUNCTION(BlueprintCallable)
	uint16 GetControllerStateAsByte();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void ChangeState(UControllerState* PreviousState);

	// Exposing input to BP as events
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Axis_Thumbstick(float Horizontal, float Vertical);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Button_Thumbstick(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Button_Primary(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Button_Secondary(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Button_Trigger(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void Input_Button_Grip(EButtonActionType ActionType);
	//

protected:

	UPROPERTY()
	UControllerState* OtherController;

	UPROPERTY(BlueprintReadonly)
	AVirtualRealityMotionController* OwningMotionController;

	UPROPERTY(EditAnywhere)
	uint16 CurrentState;
};

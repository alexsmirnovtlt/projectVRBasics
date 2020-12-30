// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VRPlayerInput.generated.h"

UENUM(BlueprintType)
enum class EButtonActionType : uint8 {
	Touched = 0 UMETA(DisplayName = "Touched"),
	Pressed = 1 UMETA(DisplayName = "Pressed"),
	ReleasedPress = 2 UMETA(DisplayName = "Released Press"),
	ReleasedTouch = 3 UMETA(DisplayName = "Released Touch")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UVRPlayerInput : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implements ability to receive Player`s input without being subscribed to Input Events
 */
class PROJECTVRBASICS_API IVRPlayerInput
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Axis_Thumbstick(float Horizontal, float Vertical);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Axis_Trigger(float Value);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Axis_Grip(float Value);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Thumbstick(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Primary(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Secondary(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Trigger(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Grip(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_Menu(EButtonActionType ActionType);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IVRPlayerInput")
	void Input_Button_System(EButtonActionType ActionType);
};

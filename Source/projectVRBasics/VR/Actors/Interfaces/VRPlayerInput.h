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

USTRUCT(BlueprintType)
struct FConsumeInputParams_Axes
{
	GENERATED_BODY()

	FConsumeInputParams_Axes()
	{
		Thumbstick = false;
		Trigger = false;
		Grip = false;
	}

	UPROPERTY(EditAnywhere)
	bool Thumbstick;
	UPROPERTY(EditAnywhere)
	bool Trigger;
	UPROPERTY(EditAnywhere)
	bool Grip;
};

USTRUCT(BlueprintType)
struct FConsumeInputParams_Buttons
{
	GENERATED_BODY()

	FConsumeInputParams_Buttons()
	{
		Primary = false;
		Secondary = false;
		Thumbstick = false;
		Trigger = false;
		Grip = false;
	}

	UPROPERTY(EditAnywhere)
	bool Primary;
	UPROPERTY(EditAnywhere)
	bool Secondary;
	UPROPERTY(EditAnywhere)
	bool Thumbstick;
	UPROPERTY(EditAnywhere)
	bool Trigger;
	UPROPERTY(EditAnywhere)
	bool Grip;
};

USTRUCT(BlueprintType)
struct FConsumeInputParams
{
	GENERATED_BODY()

	FConsumeInputParams()
	{
		Axes = FConsumeInputParams_Axes();
		Buttons = FConsumeInputParams_Buttons();
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FConsumeInputParams_Axes Axes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FConsumeInputParams_Buttons Buttons;
};

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

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVRPlayerInput")
	FConsumeInputParams GetConsumeInputParams() const;
	FConsumeInputParams GetConsumeInputParams_Implementation() const { return FConsumeInputParams(); };
};

// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HandInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHandInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implements functionality related to interaction with VR controller
 */
class PROJECTVRBASICS_API IHandInteractable
{
	GENERATED_BODY()

public:

	// When physical hand overlaps with something, it tries to cast to to this interface and executes OnHandEnter
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandEnter(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);
	// When physical hand leaves overlapped area
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandExit(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);
	
	// Same as OnHandEnter, but collider is hand`s grab sphere, not the hand itself 
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnCanBeGrabbedByHand_Start(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnCanBeGrabbedByHand_End(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);

	// When hand`s arrow points at IHandInteractable actor
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandPointing_Start(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent, FVector& HitLocation);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandPointing_End(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent, FVector& HitLocation);

	// When phantom hand (actual motion controller world location that ignores any physics collision) overlaps something
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnPhantomHandEnter(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);
	// When phantom hand leaves overlapped area
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnPhantomHandExit(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);

	// When player input indicates that he wants to grab something
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnGrab(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);
	// When player input indicates that he wants to drop something he already grabbed
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnDrop(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "IHandInteractable")
	void OnHandTeleported(AVirtualRealityMotionController* MotionController, USceneComponent* CollidedComponent);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IHandInteractable")
	FVector GetWorldLocationOfGrabCheck() const;// When player tries to grab but overlaps multiple IHandInteractable actors, the one with the lowest distance will be chosen
};

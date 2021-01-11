// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "HandPhysConstraint.generated.h"

UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AHandPhysConstraint : public AActor
{
	GENERATED_BODY()
	
public:	
	AHandPhysConstraint();

public:	
	UFUNCTION(BlueprintCallable, Category = "Hand Physics Constraint")
	void BreakConstraint();
	UFUNCTION(BlueprintCallable, Category = "Hand Physics Constraint")
	void CreateConstraint(class UPrimitiveComponent* Component, FName BoneName);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UPhysicsConstraintComponent* PhysConstraint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UBoxComponent* FirstConstraintComponent;

private:

	bool bHaveAttachment = false;
};

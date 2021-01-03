// Alex Smirnov 2020-2021


#include "HandPhysConstraint.h"

#include "Components/BoxComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"


AHandPhysConstraint::AHandPhysConstraint()
{
	PrimaryActorTick.bCanEverTick = true;

	PhysConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("PhysicsConstraintComponent"));
	RootComponent = PhysConstraint;

	// Phys constraint settings
	PhysConstraint->SetDisableCollision(true);
	PhysConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	PhysConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, 0.f);
	PhysConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, 0.f);

	PhysConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	PhysConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.f);
	PhysConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.f);

	PhysConstraint->SetLinearPositionDrive(true, true, true);
	PhysConstraint->SetLinearVelocityDrive(true, true, true);
	PhysConstraint->SetLinearPositionTarget(FVector::ZeroVector);
	PhysConstraint->SetLinearVelocityTarget(FVector::ZeroVector);
	PhysConstraint->SetLinearDriveParams(10000.f, 3000.f, 10000.f);
	
	PhysConstraint->SetAngularDriveMode(EAngularDriveMode::SLERP);
	PhysConstraint->SetAngularOrientationTarget(FRotator::ZeroRotator);
	PhysConstraint->SetOrientationDriveSLERP(true);
	PhysConstraint->SetOrientationDriveTwistAndSwing(true, true);
	PhysConstraint->SetAngularVelocityDriveTwistAndSwing(true, true);
	PhysConstraint->SetAngularVelocityDriveSLERP(true);
	
	PhysConstraint->SetAngularDriveParams(1200000.f, 90000.f, 1200000.f);
	//

	FirstConstraintComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("FirstConstraintPrimitiveComponent"));
	FirstConstraintComponent->SetBoxExtent(FVector::OneVector, false);
	FirstConstraintComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	FirstConstraintComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	FirstConstraintComponent->SetGenerateOverlapEvents(false);
	FirstConstraintComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	FirstConstraintComponent->SetupAttachment(RootComponent);
}

void AHandPhysConstraint::CreateConstraint(class UPrimitiveComponent* Component, FName BoneName)
{
	if (bHaveAttachment) BreakConstraint();

	bHaveAttachment = true;

	PhysConstraint->SetConstrainedComponents(
		FirstConstraintComponent,
		NAME_None,
		Component,
		BoneName
	);
}

void AHandPhysConstraint::BreakConstraint()
{
	if (!bHaveAttachment) return;

	bHaveAttachment = false;
	PhysConstraint->BreakConstraint();
}
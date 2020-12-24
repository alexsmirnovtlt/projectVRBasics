// Alex Smirnov 2020-2021


#include "HandActor.h"

#include "Components/SkeletalMeshComponent.h"

#include "ActorComponents/VRHandPhysicalAnimationComponent.h"

AHandActor::AHandActor()
{
	PrimaryActorTick.bCanEverTick = true;

	HandMass = 10.f;

	HandPhysicalAnimationComponent = CreateDefaultSubobject<UVRHandPhysicalAnimationComponent>(TEXT("HandPhysicalAnimationComponent"));
}

void AHandActor::BeginPlay()
{
	Super::BeginPlay();

	auto HandMesh = GetSkeletalHandMeshComponent();
	if (!HandMesh) return;

	HandMesh->SetUseCCD(true);
	HandMesh->SetMassOverrideInKg(NAME_None, HandMass);
	HandPhysicalAnimationComponent->SetSkeletalMeshComponent(HandMesh);
	HandPhysicalAnimationComponent->SetupWeldedBoneDriver();

	SetTickGroup(ETickingGroup::TG_PrePhysics);
}

void AHandActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

USkeletalMeshComponent* AHandActor::GetSkeletalHandMeshComponent_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetSkeletalHandMeshComponent()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

USceneComponent* AHandActor::GetArrowComponent_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetArrowComponent()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}
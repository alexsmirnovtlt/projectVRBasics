// Alex Smirnov 2020-2021


#include "VRCheckGameMode.h"

#include "IXRTrackingSystem.h"


void AVRCheckGameMode::StartPlay()
{
	Super::StartPlay();

	auto TrackingSystem = GEngine->XRSystem.Get();
	if (!ensure(TrackingSystem)) { return; }

	UE_LOG(LogTemp, Warning, TEXT("VR Headset Info: %s"), *TrackingSystem->GetVersionString()); // Log contains important headset info such as manufacturer and driver version 
}
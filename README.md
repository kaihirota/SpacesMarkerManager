# MojexaSpaces

Mojexa is a SaaS asset and property management product. Our clients use Mojexa to manage major public infrastructure, property portfolios, schools, coffee shops, business assets and much more.

Mojexa Spaces is a 3D digital twin of real world geographic spaces, built in Unreal Engine 4. It provides a way to visualise data on a local, regional, city, country or global scale within a virtual model built on accurate height maps and high resolution satellite imagery.

Mojexa Spaces is our new platform for our users to visualise their asset information. Users will be able to track the location of assets across the virtual geography in Spaces, providing a powerful new way of using their Mojexa software to manage their assets and properties.

This repository contains the LocationMarker classes and the LocationMarkerManager class written in C++, made to work well with Blueprint.

## Get Started

- Settings.h

## Usage

```c++
void AMyProjectCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Singleton object is already instantiated automatically at start of the game
	MarkerManager = GetWorld()->GetGameInstance<UMarkerManager>();
}
```
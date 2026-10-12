#pragma once

#include "CoreMinimal.h"

// One shared category: DEFINE_LOG_CATEGORY_STATIC in several .cpp files collides when the build merges them (unity build).
// Defined in TelemetrySubsystem.cpp.
DECLARE_LOG_CATEGORY_EXTERN(LogVehicleTelemetry, Log, All);

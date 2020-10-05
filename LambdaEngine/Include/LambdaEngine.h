#pragma once
#include "Types.h"
#include "Defines.h"

#include "Assert/Assert.h"

// Should always be included in this order since the New.h depends on Malloc.h in SharedLib for now
#include "Memory/API/Malloc.h"
#include "Memory/API/New.h"
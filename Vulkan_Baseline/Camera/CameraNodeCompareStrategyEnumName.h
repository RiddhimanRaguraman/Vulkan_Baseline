//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef Camera_NODE_Compare_Strategy_Enum_Name_H
#define Camera_NODE_Compare_Strategy_Enum_Name_H
// PORT: EngineDLLInterface.h / AZUL_ENGINE_LIBRARY_API dropped -- app source, not a DLL export.

#include "Manager_DLLInterface.h"
#include "CompareStrategyBase.h"

namespace Azul
{
	class CameraNodeCompareStrategyEnumName : public CompareStrategyBase
	{
	public:

		MANAGER_LIBRARY_API CameraNodeCompareStrategyEnumName() = default;
		MANAGER_LIBRARY_API CameraNodeCompareStrategyEnumName(const CameraNodeCompareStrategyEnumName &) = default;
		MANAGER_LIBRARY_API CameraNodeCompareStrategyEnumName &operator = (const CameraNodeCompareStrategyEnumName &) = default;
		MANAGER_LIBRARY_API virtual ~CameraNodeCompareStrategyEnumName() = default;

		MANAGER_LIBRARY_API virtual bool Compare(DLink *pRefNode, DLink *pTargetNode) override;

	};
}

#endif

// --- End of File ---

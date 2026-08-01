//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

// PORT: guard was CAM_NODE_MAN_H -- a copy/paste of CameraNodeMan.h's. It did not
// actually collide (that one is CAMERA_NODE_MAN_H) but it named the wrong class.
#ifndef CAMERA_MAN_H
#define CAMERA_MAN_H
// PORT: EngineDLLInterface.h / AZUL_ENGINE_LIBRARY_API dropped -- app source, not a DLL export.

#include "ManBase.h"
#include "CameraNode.h"
#include "Camera.h"
// PORT: dropped #include "CameraUtility.h" -- that file was not part of the folder
// you handed over, and nothing in CameraMan.{h,cpp} references it.

namespace Azul
{
	class CameraMan : public ManBase
	{
		//----------------------------------------------------------------------
		// Static Methods
		//----------------------------------------------------------------------
	public:
		static void Create(int reserveNum = 0, int reserveGrow = 1);
		static void Destroy();

		static CameraNode *Add(Camera::Name name, Camera *pCam);
		static Camera *Find(Camera::Name name);

		// w1(7)
		static void SetCurrent(Camera::Name name, Camera::Type type);
		static Camera *GetCurrent(Camera::Type type);

		static void Remove(CameraNode *pNode);
		static void Dump();

		void ProcessInput();
		static void InputUpdate();
		//----------------------------------------------------------------------
		// Private methods
		//----------------------------------------------------------------------
	private:
		static CameraMan *privGetInstance();

		CameraMan() = delete;
		CameraMan(const CameraMan &) = delete;
		CameraMan &operator = (const CameraMan &) = delete;
		~CameraMan();

		CameraMan(int reserveNum, int reserveGrow);



		//----------------------------------------------------------------------
		// Override Abstract methods
		//----------------------------------------------------------------------
	protected:
		DLink *derivedCreateNode() override;



		//----------------------------------------------------------------------
		// Data: unique data for this manager 
		//----------------------------------------------------------------------
	private:
		CameraNode *poNodeCompare;
		static CameraMan *posInstance;
		static CompareStrategyBase *posEnumNameCompare;

		// w1(7)
		//Camera *currCamera;		
		Camera *pCamOrthographic;
		Camera *pCamPerspective;
	};

}

#endif

// --- End of File ---

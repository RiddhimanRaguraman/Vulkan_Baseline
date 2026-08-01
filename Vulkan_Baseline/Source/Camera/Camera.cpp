//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#include "Camera.h"

namespace Neelam
{

	Camera::~Camera()
	{

	}

	void Camera::GetHelper(Azul::Vec3 &up, Azul::Vec3 &tar, Azul::Vec3 &pos,
						   Azul::Vec3 &upNorm, Azul::Vec3 &forwardNorm, Azul::Vec3 &pRightNorm)
	{
		this->getPos(pos);
		this->getLookAt(tar);
		this->getUp(upNorm);
		up = pos + upNorm;

		forwardNorm = tar - pos;
		forwardNorm.norm();

		this->getRight(pRightNorm);
	}

	void Camera::SetHelper(Azul::Vec3 &up_pt, Azul::Vec3 &tar_pt, Azul::Vec3 &pos_pt)
	{
		Azul::Vec3 upVect = up_pt - pos_pt;
		this->setOrientAndPosition(upVect, tar_pt, pos_pt);
	}

	Camera::Camera(Camera::Type _camType)
		: aspectRatio(0), farDist(0), fovy(0), nearDist(0)
	{
		//out("Camera(): ---------\n");
		this->name = Camera::Name::NOT_INITIALIZED;
		this->camType = _camType;
	}


	Camera::Type Camera::getType() const
	{
		return this->camType;
	}

	char *Camera::GetName() const
	{
		static char pTmp[128];

		const char *pName = "Camera::<unknown>";

		switch (this->name)
		{
		case Camera::Name::CAMERA_0:			pName = "Camera::CAMERA_0";			break;
		case Camera::Name::CAMERA_1:			pName = "Camera::CAMERA_1";			break;
		case Camera::Name::CAMERA_2:			pName = "Camera::CAMERA_2";			break;
		case Camera::Name::NOT_INITIALIZED:		pName = "Camera::NOT_INITIALIZED";	break;
		case Camera::Name::NullCamera:			pName = "Camera::NullCamera";		break;
		default:								assert(false);						break;
		}

		strcpy_s(pTmp, 128, pName);
		return pTmp;
	}
	void Camera::SetName(Camera::Name inName)
	{
		this->name = inName;
	}

	// critical informational knobs for the perspective matrix
	// Field of View Y is in degrees (copying lame OpenGL)
	void Camera::setPerspective(const float Fovy, const float Aspect, const float NearDist, const float FarDist)
	{
		this->aspectRatio = Aspect;
		this->fovy = Azul::MATH_PI_180 * Fovy;
		this->nearDist = NearDist;
		this->farDist = FarDist;
	}

	void Camera::setOrthographic(const float _xMin, const float _xMax, const float _yMin, const float _yMax, const float _zMin, const float _zMax)
	{
		assert(this->camType == Camera::Type::ORTHOGRAPHIC_2D);

		this->xMin = _xMin;
		this->yMin = _yMin;
		this->zMin = _zMin;

		this->xMax = _xMax;
		this->yMax = _yMax;
		this->zMax = _zMax;

		this->far_height = this->yMax - this->yMin;
		this->far_width = this->xMax - this->xMin;
		this->near_width = this->yMax - this->yMin;
		this->nearDist = this->zMin;
		this->farDist = this->zMax;
	}

	int Camera::getScreenWidth() const
	{
		return this->viewport_width;
	}

	int Camera::getScreenHeight() const
	{
		return this->viewport_height;
	}

	// PORT: stores only. The DX11 version ended with privSetViewState(), which
	// pushed the viewport to the immediate context the moment you called this.
	// Vulkan has no immediate context -- there is nothing to push to outside of
	// command-buffer recording -- so the apply moved to SetActive().
	void Camera::setViewport(const int inX, const int inY, const int width, const int height)
	{
		this->viewport_x = inX;
		this->viewport_y = inY;
		this->viewport_width = width;
		this->viewport_height = height;
	};

	// PORT: replaces privSetViewState() / RSSetViewports(1, &tmp).
	//
	// VkViewport maps 1:1 onto D3D11_VIEWPORT -- including MinDepth/MaxDepth 0..1,
	// because Vulkan's clip-space Z is [0,1] exactly like D3D's. Vulkan additionally
	// needs a SCISSOR: D3D11 defaults to "no clipping" when none is set, Vulkan
	// requires one per viewport, so it is set here to match the full viewport.
	//
	// Both are declared VK_DYNAMIC_STATE in the pipeline (see ShaderObject), which
	// is what lets a camera set them per frame without rebuilding the pipeline.
	void Camera::SetActive(VkCommandBuffer cmd) const
	{
		VkViewport viewport = {};
		viewport.x        = (float)this->viewport_x;
		viewport.y        = (float)this->viewport_y;
		viewport.width    = (float)this->viewport_width;
		viewport.height   = (float)this->viewport_height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x      = this->viewport_x;
		scissor.offset.y      = this->viewport_y;
		scissor.extent.width  = (uint32_t)this->viewport_width;
		scissor.extent.height = (uint32_t)this->viewport_height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	};

	// Goal, calculate the near height / width, same for far plane
	//
	// PORT/BUGFIX (not a DirectX issue -- it was wrong in the original):
	//   setPerspective() already stores fovy in RADIANS (fovy = MATH_PI_180 * Fovy),
	//   and privUpdateProjectionMatrix() correctly uses tanf(fovy/2). This function
	//   was converting a second time -- tanf((fovy * MATH_PI / 180) * 0.5f) -- which
	//   made every plane height ~57x too small. The frustum verts and therefore the
	//   collision normals were all wrong; the projection matrix was unaffected,
	//   which is why it never showed up on screen. Now matches the projection.
	void Camera::privCalcPlaneHeightWidth(void)
	{
		this->near_height = 2.0f * tanf(this->fovy * 0.5f) * this->nearDist;
		this->near_width = this->near_height * this->aspectRatio;

		this->far_height = 2.0f * tanf(this->fovy * 0.5f) * this->farDist;
		this->far_width = this->far_height * this->aspectRatio;
	};

	void Camera::setOrientAndPosition(const Azul::Vec3 &inUp, const Azul::Vec3 &inLookAt, const Azul::Vec3 &inPos)
	{
		// Remember the up, lookAt and right are unit length, and are perpendicular.
		// Treat lookAt as king, find Right vect, then correct Up to insure perpendiculare.
		// Make sure that all vectors are unit vectors.

		this->vLookAt = inLookAt;
		this->vDir = -(inLookAt - inPos); // Right-Hand camera: vDir is flipped
		this->vDir.norm();

		// Clean up the vectors (Right hand rule)
		this->vRight = inUp.cross(this->vDir);
		this->vRight.norm();

		this->vUp = this->vDir.cross(this->vRight);
		this->vUp.norm();

		this->vPos = inPos;
	};

	void Camera::privCalcFrustumVerts(void)
	{
		// Top Left corner and so forth.  In this form to see the pattern
		// Might be confusing (remember the picture) vDir goes from screen into your EYE
		// so distance from the eye is "negative" vDir
		this->nearTopLeft = this->vPos + this->vDir * this->nearDist + this->vUp * this->near_height * 0.5f - this->vRight * this->near_width * 0.5f;
		this->nearTopRight = this->vPos + this->vDir * this->nearDist + this->vUp * this->near_height * 0.5f + this->vRight * this->near_width * 0.5f;
		this->nearBottomLeft = this->vPos + this->vDir * this->nearDist - this->vUp * this->near_height * 0.5f - this->vRight * this->near_width * 0.5f;
		this->nearBottomRight = this->vPos + this->vDir * this->nearDist - this->vUp * this->near_height * 0.5f + this->vRight * this->near_width * 0.5f;
		this->farTopLeft = this->vPos + this->vDir * this->farDist + this->vUp * this->far_height * 0.5f - this->vRight * this->far_width * 0.5f;
		this->farTopRight = this->vPos + this->vDir * this->farDist + this->vUp * this->far_height * 0.5f + this->vRight * this->far_width * 0.5f;
		this->farBottomLeft = this->vPos + this->vDir * this->farDist - this->vUp * this->far_height * 0.5f - this->vRight * this->far_width * 0.5f;
		this->farBottomRight = this->vPos + this->vDir * this->farDist - this->vUp * this->far_height * 0.5f + this->vRight * this->far_width * 0.5f;
	};

	void Camera::privCalcFrustumCollisionNormals(void)
	{
		// Normals of the frustum around nearTopLeft
		Azul::Vec3 A = this->nearBottomLeft - this->nearTopLeft;
		Azul::Vec3 B = this->nearTopRight - this->nearTopLeft;
		Azul::Vec3 C = this->farTopLeft - this->nearTopLeft;

		this->frontNorm = A.cross(B);
		this->frontNorm.norm();

		this->leftNorm = C.cross(A);
		this->leftNorm.norm();

		this->topNorm = B.cross(C);
		this->topNorm.norm();

		// Normals of the frustum around farBottomRight
		A = this->farBottomLeft - this->farBottomRight;
		B = this->farTopRight - this->farBottomRight;
		C = this->nearBottomRight - this->farBottomRight;

		this->backNorm = A.cross(B);
		this->backNorm.norm();

		this->rightNorm = B.cross(C);
		this->rightNorm.norm();

		this->bottomNorm = C.cross(A);
		this->bottomNorm.norm();
	};

	// The projection matrix (note it's invertable)
	void Camera::privUpdateProjectionMatrix(void)
	{
		using namespace Azul;
		if (this->camType == Camera::Type::PERSPECTIVE_3D)
		{
			float d = 1.0f / tanf(fovy / 2.0f);

			this->projMatrix[Azul::m0] = d / aspectRatio;
			this->projMatrix[Azul::m1] = 0.0f;
			this->projMatrix[Azul::m2] = 0.0f;
			this->projMatrix[Azul::m3] = 0.0f;

			this->projMatrix[m4] = 0.0f;
			// PORT: NEGATED for Vulkan. Vulkan's NDC has +Y pointing DOWN; D3D
			// (and GL) have +Y UP. Without this every scene renders vertically
			// mirrored. Flipping the projection is chosen over the other common
			// fix -- a negative-height VkViewport -- because that would also flip
			// the existing SV_VertexID triangle, which is authored in Vulkan NDC
			// already and does not go through a camera.
			this->projMatrix[m5] = -d;
			this->projMatrix[m6] = 0.0f;
			this->projMatrix[m7] = 0.0f;

			this->projMatrix[m8] = 0.0f;
			this->projMatrix[m9] = 0.0f;
			this->projMatrix[m10] = -(this->farDist + this->nearDist) / (this->farDist - this->nearDist);
			this->projMatrix[m11] = -1.0f;

			this->projMatrix[m12] = 0.0f;
			this->projMatrix[m13] = 0.0f;
			this->projMatrix[m14] = (-2.0f * this->farDist * this->nearDist) / (this->farDist - this->nearDist);
			this->projMatrix[m15] = 0.0f;

			// Converting from OpenGL-style NDC of [-1,1] to DX's [0,1].  See note: https://anteru.net/blog/2011/12/27/1830/
			// (Note: NDCConvert should be precomputed once and stored for reuse)
			//
			// PORT: KEPT AS-IS, deliberately. This is the piece that looks like it
			// must change and does not -- Vulkan's clip-space Z is [0,1], the same
			// as D3D (it is OpenGL that is the odd one out at [-1,1]). Deleting it
			// would put half the depth range behind the near plane.
			Azul::Trans B(0.0f, 0.0f, 1.0f);
			Azul::Scale S(1.0f, 1.0f, 0.5f);

			projMatrix = projMatrix * B * S;
		}
		else
		{
			this->projMatrix[m0] = 2.0f / (xMax - xMin);
			this->projMatrix[m1] = 0.0f;
			this->projMatrix[m2] = 0.0f;
			this->projMatrix[m3] = 0.0f;

			this->projMatrix[m4] = 0.0f;
			// PORT: NEGATED for Vulkan's Y-down NDC, same reason as the
			// perspective branch above. The 2D/ortho camera needs it too, or
			// sprites and UI come out upside down.
			this->projMatrix[m5] = -2.0f / (yMax - yMin);
			this->projMatrix[m6] = 0.0f;
			this->projMatrix[m7] = 0.0f;

			this->projMatrix[m8] = 0.0f;
			this->projMatrix[m9] = 0.0f;
			this->projMatrix[m10] = -1.0f / (zMax - zMin); // -2.0f / (zMax - zMin);
			this->projMatrix[m11] = 0.0f;

			this->projMatrix[m12] = 0;
			this->projMatrix[m13] = 0;
			this->projMatrix[m14] = -(zMin) / (zMax - zMin); //-(zMax + zMin) / (zMax - zMin);
			this->projMatrix[m15] = 1.0f;
		}
	};

	void Camera::privUpdateViewMatrix(void)
	{
		// This functions assumes the your vUp, vRight, vDir are still unit
		// And perpendicular to each other
		using namespace Azul;
		// Set for DX Right-handed space
		this->viewMatrix[m0] = this->vRight[x];
		this->viewMatrix[m1] = this->vUp[x];
		this->viewMatrix[m2] = this->vDir[x];
		this->viewMatrix[m3] = 0.0f;

		this->viewMatrix[m4] = this->vRight[y];
		this->viewMatrix[m5] = this->vUp[y];
		this->viewMatrix[m6] = this->vDir[y];
		this->viewMatrix[m7] = 0.0f;

		this->viewMatrix[m8] = this->vRight[z];
		this->viewMatrix[m9] = this->vUp[z];
		this->viewMatrix[m10] = this->vDir[z];
		this->viewMatrix[m11] = 0.0f;

		// Apply R^t( -Pos ). Use dot-product with the basis vectors
		this->viewMatrix[m12] = -vPos.dot(vRight);
		this->viewMatrix[m13] = -vPos.dot(vUp);
		this->viewMatrix[m14] = -vPos.dot(vDir);
		this->viewMatrix[m15] = 1.0f;
	};

	// Update everything (make sure it's consistent)
	void Camera::updateCamera(void)
	{
		// First find the near height/width, far height/width
		this->privCalcPlaneHeightWidth();

		// Find the frustum physical verts
		this->privCalcFrustumVerts();

		// find the frustum collision normals
		this->privCalcFrustumCollisionNormals();

		// update the projection matrix
		this->privUpdateProjectionMatrix();

		// update the view matrix
		this->privUpdateViewMatrix();
	}

	// Accessor mess:
	Azul::Mat4 &Camera::getViewMatrix(void)
	{
		return this->viewMatrix;
	}

	Azul::Mat4 &Camera::getProjMatrix(void)
	{
		return this->projMatrix;
	}

	void Camera::getPos(Azul::Vec3 &outPos) const
	{
		outPos = this->vPos;
	}

	void  Camera::getDir(Azul::Vec3 &outDir) const
	{
		outDir = this->vDir;
	}

	void  Camera::getUp(Azul::Vec3 &outUp) const
	{
		outUp = this->vUp;
	}

	void Camera::getLookAt(Azul::Vec3 &outLookAt) const
	{
		outLookAt = this->vLookAt;
	}

	void Camera::getRight(Azul::Vec3 &outRight) const
	{
		outRight = this->vRight;
	}

	void Camera::getFieldOfView(float &Value) const
	{
		Value = this->aspectRatio;
	}

	void Camera::setFieldOfView(const float Value)
	{
		this->aspectRatio = Value;
	}

	// ADDED (not in the DX11 original) -- see the note in Camera.h.
	void Camera::setAspectRatio(const float Value)
	{
		this->aspectRatio = Value;
	}

	void Camera::getNearDist(float &Value) const
	{
		Value = this->nearDist;
	}

	void Camera::setNearDist(const float Value)
	{
		this->nearDist = Value;
	}

	void Camera::getNearTopLeft(Azul::Vec3 &vOut) const
	{
		vOut = this->nearTopLeft;
	}

	void Camera::getNearTopRight(Azul::Vec3 &vOut) const
	{
		vOut = this->nearTopRight;
	}

	void Camera::getNearBottomLeft(Azul::Vec3 &vOut)const
	{
		vOut = this->nearBottomLeft;
	}

	void Camera::getNearBottomRight(Azul::Vec3 &vOut) const
	{
		vOut = this->nearBottomRight;
	}

	void Camera::getFarTopLeft(Azul::Vec3 &vOut) const
	{
		vOut = this->farTopLeft;
	}

	void Camera::getFarTopRight(Azul::Vec3 &vOut) const
	{
		vOut = this->farTopRight;
	}

	void Camera::getFarBottomLeft(Azul::Vec3 &vOut) const
	{
		vOut = this->farBottomLeft;
	}

	void Camera::getFarBottomRight(Azul::Vec3 &vOut)const
	{
		vOut = this->farBottomRight;
	}
}

// --- End of File ---

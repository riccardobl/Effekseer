#pragma once

#include <array>
#include <random>

#include "../Effekseer.Vector3D.h"
#include "../Noise/CurlNoise.h"
#include "../Noise/PerlinNoise.h"
#include "../SIMD/Effekseer.Mat44f.h"
#include "../SIMD/Effekseer.Vec3f.h"

namespace Effekseer
{

/**
	TODO
	Implement rotation
	Check falloff specification
*/

struct ForceFieldCommonParameter
{
	Vec3f Position;
	Vec3f PreviousVelocity;
	Vec3f PreviousSumVelocity;
	Vec3f FieldCenter;
	Mat44f FieldRotation;
	bool IsFieldRotated = false;
};

struct ForceFieldFalloffCommonParameter
{
	float Power = 0.0f;
	float MinDistance = 0.0f;
	float MaxDistance = 0.0f;
};

struct ForceFieldFalloffSphereParameter
{
};

struct ForceFieldFalloffTubeParameter
{
	float RadiusPower = 0.0f;
	float MinRadius = 0.0f;
	float MaxRadius = 0.0f;
};

struct ForceFieldFalloffConeParameter
{
	float Power = 0.0f;
	float MinAngle = 0.0f;
	float MaxAngle = 0.0f;
};

struct ForceFieldForceParameter
{
	// Shape
	float Power;
	bool Gravitation;
};

struct ForceFieldWindParameter
{
	// Shape
	float Power;
};

struct ForceFieldVortexParameter
{
	// Shape
	float Power;
};

struct ForceFieldMagineticParameter
{
	// Shape
	float Power;
};

struct ForceFieldChargeParameter
{
	// Shape
	float Power;
};

struct ForceFieldTurbulenceParameter
{
	float Power;
	CurlNoise Noise;

	ForceFieldTurbulenceParameter(int32_t seed, float scale, float strength, int octave);
};

struct ForceFieldDragParameter
{
	float Power;
};

class ForceFieldFalloff
{
public:
	//! Sphare
	float GetPower(float power,
				   const ForceFieldCommonParameter& ffc,
				   const ForceFieldFalloffCommonParameter& fffc,
				   const ForceFieldFalloffSphereParameter& fffs)
	{
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength();
		if (distance > fffc.MaxDistance)
		{
			return power;
		}

		if (distance < fffc.MinDistance)
		{
			return power;
		}

		return power * fffc.Power;
	}

	//! Tube
	float GetPower(float power,
				   const ForceFieldCommonParameter& ffc,
				   const ForceFieldFalloffCommonParameter& fffc,
				   const ForceFieldFalloffTubeParameter& ffft)
	{
		// Sphere
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength();
		if (distance > fffc.MaxDistance)
		{
			return power;
		}

		if (distance < fffc.MinDistance)
		{
			return power;
		}

		// Tube
		auto tubePos = localPos;
		if (ffc.IsFieldRotated)
		{
			// tubePos = tubePos - Vec3f::Dot(tubePos, ffc.FieldRotation.)
		}
		else
		{
			tubePos.SetY(0);
		}

		auto tubeRadius = tubePos.GetLength();

		if (tubeRadius > ffft.MaxRadius)
		{
			return power;
		}

		if (tubeRadius < ffft.MinRadius)
		{
			return power;
		}

		return power * fffc.Power;
	}

	float GetPower(float power,
				   const ForceFieldCommonParameter& ffc,
				   const ForceFieldFalloffCommonParameter& fffc,
				   const ForceFieldFalloffConeParameter& ffft)
	{
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength();
		if (distance > fffc.MaxDistance)
		{
			return power;
		}

		if (distance < fffc.MinDistance)
		{
			return power;
		}

		auto angle = atan(localPos.GetX() / localPos.GetY());

		if (angle > ffft.MaxAngle)
		{
			return power;
		}

		if (angle < ffft.MinAngle)
		{
			return power;
		}

		return power * fffc.Power;
	}
};

class ForceField
{
public:
	/**
		@brief	Force
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldForceParameter& ffp)
	{
		float eps = 0.0000001f;
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength() + eps;
		auto dir = localPos / distance;

		if (ffp.Gravitation)
		{
			return dir * ffp.Power / distance;
		}

		return dir * ffp.Power;
	}

	/**
		@brief	Wind
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldWindParameter& ffp)
	{
		auto dir = Vec3f(0, 1, 0);
		return dir * ffp.Power;
	}

	/**
		@brief	Vortex
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldVortexParameter& ffp)
	{
		float eps = 0.0000001f;
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength() + eps;
		auto axis = Vec3f(0, 1, 0);
		Vec3f front = Vec3f::Cross(axis, localPos);
		front.Normalize();

		return front * ffp.Power;
	}

	/**
		@brief	Maginetic
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldMagineticParameter& ffp)
	{
		float eps = 0.0000001f;
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength() + eps;
		auto dir = localPos / distance;

		auto forceDir = Vec3f::Cross(ffc.PreviousSumVelocity, dir);

		if (forceDir.GetSquaredLength() < 0.01f)
			return Vec3f(0.0f, 0.0f, 0.0f);

		return forceDir * ffp.Power;
	}

	/**
		@brief	Charge
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldChargeParameter& ffp)
	{
		float eps = 0.0000001f;
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto distance = localPos.GetLength() + eps;
		auto dir = localPos / distance;

		if (dir.GetLength() < ffp.Power)
		{
			return -ffc.PreviousSumVelocity;
		}

		return -dir * ffp.Power;
	}

	/**
		@brief	Turbulence
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldTurbulenceParameter& ffp)
	{
		auto localPos = ffc.Position - ffc.FieldCenter;
		auto vel = ffp.Noise.Get(localPos) * ffp.Power;
		auto acc = vel - ffc.PreviousVelocity;
		return acc;
	}

	/**
		@brief	Drag
	*/
	Vec3f GetAcceleration(const ForceFieldCommonParameter& ffc, const ForceFieldDragParameter& ffp)
	{
		return -ffc.PreviousSumVelocity * ffp.Power;
	}
};

enum class LocalForceFieldFalloffType : int32_t
{
	None = 0,
	Sphere = 1,
	Tube = 2,
	Cone = 3,
};

enum class LocalForceFieldType : int32_t
{
	None = 0,
	Force = 2,
	Wind = 3,
	Vortex = 4,
	Maginetic = 5,
	Charge = 6,
	Turbulence = 1,
	Drag = 7,
};

struct LocalForceFieldTurbulenceParameter
{
	float Strength = 0.1f;
	CurlNoise Noise;

	LocalForceFieldTurbulenceParameter(int32_t seed, float scale, float strength, int octave);
};

//! TODO Replace
struct LocalForceFieldParameter
{
	std::unique_ptr<LocalForceFieldTurbulenceParameter> Turbulence;

	bool Load(uint8_t*& pos, int32_t version);
};

//! TODO Replace
struct LocalForceFieldElementParameter2
{
	std::unique_ptr<ForceFieldForceParameter> Force;
	std::unique_ptr<ForceFieldWindParameter> Wind;
	std::unique_ptr<ForceFieldVortexParameter> Vortex;
	std::unique_ptr<ForceFieldMagineticParameter> Maginetic;
	std::unique_ptr<ForceFieldChargeParameter> Charge;
	std::unique_ptr<ForceFieldTurbulenceParameter> Turbulence;
	std::unique_ptr<ForceFieldDragParameter> Drag;

	std::unique_ptr<ForceFieldFalloffCommonParameter> FalloffCommon;
	std::unique_ptr<ForceFieldFalloffSphereParameter> FalloffSphere;
	std::unique_ptr<ForceFieldFalloffTubeParameter> FalloffTube;
	std::unique_ptr<ForceFieldFalloffConeParameter> FalloffCone;

	bool Load(uint8_t*& pos, int32_t version);
};

//! TODO rename
struct LocalForceFieldParameter2
{
	std::array<LocalForceFieldElementParameter2, LocalFieldSlotMax> LocalForceFields;

	bool Load(uint8_t*& pos, int32_t version);
};

struct LocalForceFieldInstance
{
	std::array<Vec3f, LocalFieldSlotMax> Velocities;

	Vec3f VelocitySum;
	Vec3f ModifyLocation;

	void Update(const LocalForceFieldParameter2& parameter, const Vec3f& location, float magnification);
};

} // namespace Effekseer

#include "ForceFields.h"
#include "../Utils/Effekseer.BinaryReader.h"

namespace Effekseer
{

ForceFieldTurbulenceParameter::ForceFieldTurbulenceParameter(int32_t seed, float scale, float strength, int octave) : Noise(seed)
{
	Noise.Octave = octave;
	Noise.Scale = scale;
	Power = strength;
}

LocalForceFieldTurbulenceParameter::LocalForceFieldTurbulenceParameter(int32_t seed, float scale, float strength, int octave) : Noise(seed)
{
	Noise.Octave = octave;
	Noise.Scale = scale;
	Strength = strength;
}

bool LocalForceFieldParameter::Load(uint8_t*& pos, int32_t version)
{
	auto br = BinaryReader<false>(pos, std::numeric_limits<int>::max());

	LocalForceFieldType type{};
	br.Read(type);

	if (type == LocalForceFieldType::Turbulence)
	{
		int32_t seed{};
		float scale{};
		float strength{};
		int octave{};

		br.Read(seed);
		br.Read(scale);
		br.Read(strength);
		br.Read(octave);

		scale = 1.0f / scale;

		Turbulence =
			std::unique_ptr<LocalForceFieldTurbulenceParameter>(new LocalForceFieldTurbulenceParameter(seed, scale, strength, octave));
	}

	pos += br.GetOffset();

	return true;
}

bool LocalForceFieldElementParameter2::Load(uint8_t*& pos, int32_t version)
{
	auto br = BinaryReader<false>(pos, std::numeric_limits<int>::max());

	LocalForceFieldType type{};
	br.Read(type);

	if (type == LocalForceFieldType::Turbulence)
	{
		int32_t seed{};
		float scale{};
		float strength{};
		int octave{};

		br.Read(seed);
		br.Read(scale);
		br.Read(strength);
		br.Read(octave);

		scale = 1.0f / scale;

		Turbulence = std::unique_ptr<ForceFieldTurbulenceParameter>(new ForceFieldTurbulenceParameter(seed, scale, strength, octave));
	}

	pos += br.GetOffset();

	return true;
}

bool LocalForceFieldParameter2::Load(uint8_t*& pos, int32_t version)
{
	int32_t count = 0;
	memcpy(&count, pos, sizeof(int));
	pos += sizeof(int);

	for (int32_t i = 0; i < count; i++)
	{
		if (!LocalForceFields[i].Load(pos, version))
		{
			return false;
		}
	}

	return true;
}

void LocalForceFieldInstance::Update(const LocalForceFieldParameter2& parameter, const Vec3f& location, float magnification)
{
	for (size_t i = 0; i < parameter.LocalForceFields.size(); i++)
	{
		auto& field = parameter.LocalForceFields[i];

		ForceFieldCommonParameter ffcp;
		ffcp.FieldCenter = Vec3f(0, 0, 0);
		ffcp.Position = location / magnification;
		ffcp.PreviousSumVelocity = VelocitySum;
		ffcp.PreviousVelocity = Velocities[i];

		ForceField ff;

		Vec3f acc;
		if (field.Force != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Force) * magnification;
		}

		if (field.Wind != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Wind) * magnification;
		}

		if (field.Vortex != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Vortex) * magnification;
		}

		if (field.Maginetic != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Maginetic) * magnification;
		}

		if (field.Charge != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Charge) * magnification;
		}

		if (field.Turbulence != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Turbulence) * magnification;
		}

		if (field.Drag != nullptr)
		{
			acc = ff.GetAcceleration(ffcp, *field.Drag) * magnification;
		}

		float power = 1.0f;
		if (field.FalloffCommon != nullptr && field.FalloffCone != nullptr)
		{
			ForceFieldFalloff fff;
			power = fff.GetPower(power, ffcp, *field.FalloffCommon, *field.FalloffCone);
		}

		if (field.FalloffCommon != nullptr && field.FalloffSphere != nullptr)
		{
			ForceFieldFalloff fff;
			power = fff.GetPower(power, ffcp, *field.FalloffCommon, *field.FalloffSphere);
		}

		if (field.FalloffCommon != nullptr && field.FalloffTube != nullptr)
		{
			ForceFieldFalloff fff;
			power = fff.GetPower(power, ffcp, *field.FalloffCommon, *field.FalloffTube);
		}

		acc *= power;

		Velocities[i] += acc;
	}

	VelocitySum = Vec3f(0, 0, 0);

	for (size_t i = 0; i < parameter.LocalForceFields.size(); i++)
	{
		VelocitySum += Velocities[i];
	}

	ModifyLocation += VelocitySum;
}

} // namespace Effekseer
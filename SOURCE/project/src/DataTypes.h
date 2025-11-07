#pragma once
#include <stdexcept>
#include <vector>
#include "Math.h"

namespace dae
{
#pragma region GEOMETRY
	struct Sphere final
	{
		Vector3 origin{};
		float radius{};

		unsigned char materialIndex{ 0 };
	};

	struct Plane final
	{
		Vector3 origin{};
		Vector3 normal{};

		unsigned char materialIndex{ 0 };
	};

	enum class TriangleCullMode
	{
		FrontFaceCulling,
		BackFaceCulling,
		NoCulling
	};

	struct Triangle final
	{
		Triangle() = default;
		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2, const Vector3& _normal) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }, normal{ _normal.Normalized() } {}

		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }
		{
			const Vector3 edgeV0V1 = v1 - v0;
			const Vector3 edgeV0V2 = v2 - v0;
			normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();
		}

		Vector3 v0{};
		Vector3 v1{};
		Vector3 v2{};

		Vector3 normal{};

		TriangleCullMode cullMode{ TriangleCullMode::BackFaceCulling };
		unsigned char materialIndex{};
	};

	struct TriangleMesh final
	{
		TriangleMesh() = default;
		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), cullMode(_cullMode)
		{
			CalculateNormals();

			UpdateAABB();

			UpdateTransforms();
		}

		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, const std::vector<Vector3>& _normals, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), normals(_normals), cullMode(_cullMode)
		{
			UpdateAABB();

			UpdateTransforms();
		}

		std::vector<Vector3> positions{};
		std::vector<Vector3> normals{};
		std::vector<int> indices{};
		unsigned char materialIndex{};

		TriangleCullMode cullMode{ TriangleCullMode::BackFaceCulling };

		Matrix rotationTransform{};
		Matrix translationTransform{};
		Matrix scaleTransform{};

		Vector3 minAABB{};
		Vector3 maxAABB{};

		Vector3 transformedMinAABB{};
		Vector3 transformedMaxAABB{};

		std::vector<Vector3> transformedPositions{};
		std::vector<Vector3> transformedNormals{};

		void Translate(const Vector3& translation)
		{
			translationTransform = Matrix::CreateTranslation(translation);
		}

		void RotateY(float yaw)
		{
			rotationTransform = Matrix::CreateRotationY(yaw);
		}

		void Scale(const Vector3& scale)
		{
			scaleTransform = Matrix::CreateScale(scale);
		}

		void AppendTriangle(const Triangle& triangle, bool ignoreTransformUpdate = false)
		{
			int startIndex = static_cast<int>(positions.size());

			positions.emplace_back(triangle.v0);
			positions.emplace_back(triangle.v1);
			positions.emplace_back(triangle.v2);

			indices.emplace_back(startIndex);
			indices.emplace_back(++startIndex);
			indices.emplace_back(++startIndex);

			normals.emplace_back(triangle.normal);

			//Not ideal, but making sure all vertices are updated
			if (!ignoreTransformUpdate)
				UpdateTransforms();
		}

		void CalculateNormals()
		{
			normals.clear();
			normals.resize(positions.size(), Vector3{});
			//Accumulate normals
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const int i0 = indices[i];
				const int i1 = indices[i + 1];
				const int i2 = indices[i + 2];
				const Vector3 edgeV0V1 = positions[i1] - positions[i0];
				const Vector3 edgeV0V2 = positions[i2] - positions[i0];
				const Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();
				normals[i0] += normal;
				normals[i1] += normal;
				normals[i2] += normal;
			}
			//Normalize normals
			for (auto& n : normals)
				n.Normalize();
			
		}

		void UpdateTransforms()
		{
			const auto finalTransform = rotationTransform * translationTransform * scaleTransform;

			transformedPositions.clear();
			transformedPositions.reserve(positions.size());

			for (const auto& pos : positions)
			{
				const Vector3 transformedPos = finalTransform.TransformPoint(pos);
				transformedPositions.emplace_back(transformedPos);
			}

			UpdateTransformedAABB(finalTransform);
			
			transformedNormals.clear();
			transformedNormals.reserve(normals.size());

			for (const auto& norm : normals)
			{
				const Vector3 transformedNormal = finalTransform.TransformVector(norm).Normalized();
				transformedNormals.emplace_back(transformedNormal);
			}
		}

		void UpdateAABB()
		{
			if (positions.empty())
				return;
			minAABB = positions[0];
			maxAABB = positions[0];
			for (const auto& pos : positions)
			{
				minAABB = Vector3::Min(minAABB, pos);
				maxAABB = Vector3::Max(maxAABB, pos);
			}
		}

		void UpdateTransformedAABB(const Matrix& finalTransform) {
			Vector3 tMinAABB = finalTransform.TransformPoint(minAABB);
			Vector3 tMaxAABB = tMinAABB;

			Vector3 tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(maxAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(minAABB.x, minAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
	
			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(maxAABB.x, maxAABB.y, minAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);
	
			tAABB = finalTransform.TransformPoint(maxAABB);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			tAABB = finalTransform.TransformPoint(minAABB.x, maxAABB.y, maxAABB.z);
			tMinAABB = Vector3::Min(tAABB, tMinAABB);
			tMaxAABB = Vector3::Max(tAABB, tMaxAABB);

			transformedMinAABB = tMinAABB;
			transformedMaxAABB = tMaxAABB;
		}
	};
#pragma endregion
#pragma region LIGHT
	enum class LightType
	{
		Point,
		Directional
	};

	struct Light final
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};

		LightType type{};
	};
#pragma endregion
#pragma region MISC
	struct Ray final
	{
		Vector3 origin{};
		Vector3 direction{};

		float min{ 0.0001f };
		float max{ FLT_MAX };
	};

	struct HitRecord final
	{
		Vector3 origin{};
		Vector3 normal{};
		float t = FLT_MAX;

		bool didHit{ false };
		unsigned char materialIndex{ 0 };
	};
#pragma endregion
}
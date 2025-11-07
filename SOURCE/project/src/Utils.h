#pragma once
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			
			auto L = sphere.origin - ray.origin;
			float tca = Vector3::Dot(L, ray.direction);
			float d2 = Vector3::Dot(L, L) - tca * tca;
			float radius2 = sphere.radius * sphere.radius;
			
            if (d2 > radius2)
            {
				// No intersection
				return false;
            }
            
			float thc = sqrtf(radius2 - d2);
			float t0 = tca - thc;

			if (t0 < 0 || t0 > ray.max) {
				//check if intersection is behind us
				//cause then no need to render
				return false;
			}

			if (!ignoreHitRecord) {
				auto P = ray.origin + t0 * ray.direction;
				hitRecord.origin = P;
				hitRecord.normal = (P - sphere.origin).Normalized();
				hitRecord.t = t0;
				hitRecord.didHit = true;
				hitRecord.materialIndex = sphere.materialIndex;
			}
            
			return true;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			
			float denom = Vector3::Dot(plane.normal, ray.direction);

			// Use a small epsilon to check if the ray is not parallel to the plane
			if (abs(denom) > 0.0001f)
			{
				// Calculate the distance from the ray origin to the plane
				float t = Vector3::Dot(plane.origin - ray.origin, plane.normal) / denom;

				// Ensure the hit is in front of the ray's origin and within the ray's bounds
				if (t >= ray.min && t <= ray.max)
				{
					if (!ignoreHitRecord) {
						hitRecord.origin = ray.origin + t * ray.direction;
						hitRecord.t = t;
						hitRecord.didHit = true;
						hitRecord.materialIndex = plane.materialIndex;

						hitRecord.normal = (denom < 0) ? plane.normal : -plane.normal;
					}
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			auto a = triangle.v1 - triangle.v0;
			auto b = triangle.v2 - triangle.v0;
			auto n = Vector3::Cross(a, b);
			if(Vector3::Dot(n, ray.direction)==0)
				return false;
			auto l = triangle.v0 - ray.origin;
			auto t = Vector3::Dot(n, l) / Vector3::Dot(n, ray.direction);
			if(t<ray.min || t > ray.max)
				return false;

			auto P = ray.origin + t * ray.direction;
			auto c = P - triangle.v0;

			for (int i = 0; i < 3; ++i) {
				switch(i){
					case 0:
						a = triangle.v1 - triangle.v0;
						
						break;
					case 1:
						a = triangle.v2 - triangle.v1;
						c = P - triangle.v1;
						break;
					case 2:
						a = triangle.v0 - triangle.v2;
						c = P - triangle.v2;
						break;
				}
				if (Vector3::Dot(Vector3::Cross(a, c), n) < 0)
					return false;
			}
			if(!ignoreHitRecord){
				hitRecord.origin = P;
				hitRecord.normal = triangle.normal;
				hitRecord.t = t;
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;
			}
			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region triangle mesh slab test
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			if (!SlabTest_TriangleMesh(mesh, ray))
				return false;

			bool didHitSomething = false;
			float closestT = FLT_MAX;
			HitRecord closestHit{};
			for (size_t i = 0; i < mesh.indices.size(); i += 3)
			{
				const int i0 = mesh.indices[i];
				const int i1 = mesh.indices[i + 1];
				const int i2 = mesh.indices[i + 2];
				Triangle triangle{
					mesh.transformedPositions[i0],
					mesh.transformedPositions[i1],
					mesh.transformedPositions[i2],

				};
				triangle.materialIndex = mesh.materialIndex;
				triangle.cullMode = mesh.cullMode;
				HitRecord hit{};
				if (HitTest_Triangle(triangle, ray, hit, false))
				{
					didHitSomething = true;
					if (hit.t < closestT)
					{
						closestT = hit.t;
						closestHit = hit;
					}
				}
			}
			if (didHitSomething && !ignoreHitRecord)
			{
				hitRecord = closestHit;
			}
			return didHitSomething;

		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			return (light.origin - origin).Normalized();
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			switch (light.type)
			{
			case LightType::Directional:
				return light.color * light.intensity;
				break;
			case LightType::Point:
				return light.color * (light.intensity / (light.origin - target).SqrMagnitude());
				break;
			default:
				return ColorRGB{};
				break;
			}
		}
	}

	namespace Utils
	{
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;

			while (!file.eof())
			{
				file >> sCommand;
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}
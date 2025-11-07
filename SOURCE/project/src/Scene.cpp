#include "Scene.h"
#include "Utils.h"
#include "Material.h"

namespace dae {

#pragma region Base Scene
	//Initialize Scene with Default Solid Color Material (RED)
	Scene::Scene() :
		m_Materials({ new Material_SolidColor({1,0,0}) })
	{
		m_SphereGeometries.reserve(32);
		m_PlaneGeometries.reserve(32);
		m_TriangleMeshGeometries.reserve(32);
		m_Lights.reserve(32);
	}

	Scene::~Scene()
	{
		for (auto& pMaterial : m_Materials)
		{
			delete pMaterial;
			pMaterial = nullptr;
		}

		m_Materials.clear();
	}

    void dae::Scene::GetClosestHit(const Ray& ray, HitRecord& closestHit) const
    {
        closestHit.didHit = false;
        float closestT = FLT_MAX;

        for (const auto& sphere : m_SphereGeometries)
        {
            HitRecord hit{};
            if (GeometryUtils::HitTest_Sphere(sphere,ray,hit))
            {
                if (hit.didHit && hit.t < closestT)
                {
                    closestT = hit.t;
                    closestHit = hit;
                }
            }
        }
		
		for (const auto& plane : m_PlaneGeometries)
		{
			HitRecord hit{};
			if (GeometryUtils::HitTest_Plane(plane, ray, hit))
			{
				if (hit.didHit && hit.t < closestT)
				{
					closestT = hit.t;
					closestHit = hit;
				}
			}
		}

		for (const auto& triangle : m_Triangles) {
			HitRecord hit{};
			if (GeometryUtils::HitTest_Triangle(triangle, ray, hit))
			{
				switch (triangle.cullMode) {
				case TriangleCullMode::BackFaceCulling:
					if (Vector3::Dot(triangle.normal, ray.direction) > 0)
						continue;
					break;
				case TriangleCullMode::FrontFaceCulling:
					if (Vector3::Dot(triangle.normal, ray.direction) < 0)
						continue;
					break;
				case TriangleCullMode::NoCulling:
					break;
				}
				if (hit.didHit && hit.t < closestT)
				{
					closestT = hit.t;
					closestHit = hit;
				}
			}
		}

		for (const auto& mesh : m_TriangleMeshGeometries)
		{
			HitRecord hit{};
			if(GeometryUtils::HitTest_TriangleMesh(mesh, ray, hit))
			{
				switch (mesh.cullMode) {
				case TriangleCullMode::BackFaceCulling:
					if (Vector3::Dot(hit.normal, ray.direction) > 0)
						continue;
					break;
				case TriangleCullMode::FrontFaceCulling:
					if (Vector3::Dot(hit.normal, ray.direction) < 0)
						continue;
					break;
				case TriangleCullMode::NoCulling:
					break;
				}
				if (hit.didHit && hit.t < closestT)
				{
					closestT = hit.t;
					closestHit = hit;
				}
			};
		}
    }

	bool Scene::DoesHit(const Ray& ray) const
	{
		for (const auto& sphere : m_SphereGeometries)
		{
			if (GeometryUtils::HitTest_Sphere(sphere, ray))
			{ 
				return true; 
			}	
		}

		for (const auto& plane : m_PlaneGeometries)
		{
			if (GeometryUtils::HitTest_Plane(plane, ray))
			{
				return true;
			}
		}

		for (const auto& triangle : m_Triangles) {
			if (GeometryUtils::HitTest_Triangle(triangle, ray))
			{
				return true;
			}
		}

		for (const auto& mesh : m_TriangleMeshGeometries)
		{
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
				if (GeometryUtils::HitTest_Triangle(triangle, ray))
				{
					return true;
				}
			};
				
		}
		return false;
	}

#pragma region Scene Helpers
	Sphere* Scene::AddSphere(const Vector3& origin, float radius, unsigned char materialIndex)
	{
		Sphere s;
		s.origin = origin;
		s.radius = radius;
		s.materialIndex = materialIndex;

		m_SphereGeometries.emplace_back(s);
		return &m_SphereGeometries.back();
	}

	Plane* Scene::AddPlane(const Vector3& origin, const Vector3& normal, unsigned char materialIndex)
	{
		Plane p;
		p.origin = origin;
		p.normal = normal;
		p.materialIndex = materialIndex;

		m_PlaneGeometries.emplace_back(p);
		return &m_PlaneGeometries.back();
	}

	TriangleMesh* Scene::AddTriangleMesh(TriangleCullMode cullMode, unsigned char materialIndex)
	{
		TriangleMesh m{};
		m.cullMode = cullMode;
		m.materialIndex = materialIndex;

		m_TriangleMeshGeometries.emplace_back(m);
		return &m_TriangleMeshGeometries.back();
	}

	Light* Scene::AddPointLight(const Vector3& origin, float intensity, const ColorRGB& color)
	{
		Light l;
		l.origin = origin;
		l.intensity = intensity;
		l.color = color;
		l.type = LightType::Point;

		m_Lights.emplace_back(l);
		return &m_Lights.back();
	}

	Light* Scene::AddDirectionalLight(const Vector3& direction, float intensity, const ColorRGB& color)
	{
		Light l;
		l.direction = direction;
		l.intensity = intensity;
		l.color = color;
		l.type = LightType::Directional;

		m_Lights.emplace_back(l);
		return &m_Lights.back();
	}

	unsigned char Scene::AddMaterial(Material* pMaterial)
	{
		m_Materials.push_back(pMaterial);
		return static_cast<unsigned char>(m_Materials.size() - 1);
	}
#pragma endregion
#pragma endregion
#pragma region W4-TestScene
	void Scene_W4_TestScene::Initialize()
	{
		sceneName = "W4 Test Scene";	
		m_Camera.origin = { 0.f, 1.f, -5.f };
		m_Camera.fovAngle = 45.f;

		const auto matCT_GrayRoughMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, 1.f));
		const auto matCT_GrayMediumMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, .6f));
		const auto matCT_GraySmoothMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, .1f));
		const auto matCT_GrayRoughPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, 1.f));
		const auto matCT_GrayMediumPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, .6f));
		const auto matCT_GraySmoothPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, .1f));

		const auto matLambert_GrayBlue = AddMaterial(new Material_Lambert({ .49f,.57f,.57f }, 1.f));
		const auto matLambert_White = AddMaterial(new Material_Lambert(colors::White, 1.f));

		//Plane
		AddPlane({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f }, matLambert_GrayBlue); //BACK
		AddPlane({ -5.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, matLambert_GrayBlue); //LEFT
		AddPlane({ 5.f, 0.f, 0.f }, { -1.f, 0.f, 0.f }, matLambert_GrayBlue); //RIGHT
		AddPlane({ 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, matLambert_GrayBlue); //BOTTOM
		AddPlane({ 0.f, 10.f, 0.f }, { 0.f, -1.f, 0.f }, matLambert_GrayBlue); //TOP

		//Spheres
		AddSphere({ -1.75f, 1.f, 0.f }, .75f, matCT_GrayRoughMetal);
		AddSphere({ 0.f, 1.f, 0.f }, .75f, matCT_GrayMediumMetal);
		AddSphere({ 1.75f, 1.f, 0.f }, .75f, matCT_GraySmoothMetal);
		AddSphere({ -1.75f, 3.f, 0.f }, .75f, matCT_GrayRoughPlastic);
		AddSphere({ 0.f, 3.f, 0.f }, .75f, matCT_GrayMediumPlastic);
		AddSphere({ 1.75f, 3.f, 0.f }, .75f, matCT_GraySmoothPlastic);

		const Triangle baseTriangle = Triangle{ Vector3(-0.75f,1.5f,0.f),Vector3(0.75f,0.f,0.f),Vector3(-0.75f,0.f,0.f) };
		m_Meshes[0] = AddTriangleMesh(TriangleCullMode::BackFaceCulling, matLambert_White);
		m_Meshes[0]->AppendTriangle(baseTriangle,true);
		m_Meshes[0]->Translate({ -1.75f, 4.5f, 0.f });
		m_Meshes[0]->UpdateAABB();
		m_Meshes[0]->UpdateTransforms();

		m_Meshes[1] = AddTriangleMesh(TriangleCullMode::FrontFaceCulling, matLambert_White);
		m_Meshes[1]->AppendTriangle(baseTriangle, true);
		m_Meshes[1]->Translate({ 0.f, 4.5f, 0.f });
		m_Meshes[1]->UpdateAABB();
		m_Meshes[1]->UpdateTransforms();

		m_Meshes[2] = AddTriangleMesh(TriangleCullMode::NoCulling, matLambert_White);
		m_Meshes[2]->AppendTriangle(baseTriangle, true);
		m_Meshes[2]->Translate({ 1.75f, 4.5f, 0.f });
		m_Meshes[2]->UpdateAABB();
		m_Meshes[2]->UpdateTransforms();

		//Light
		AddPointLight({ 0.f, 5.f, 5.f }, 50.f, ColorRGB{ 1.f,.61f,.45f });//back light
		AddPointLight({ -2.5f, 5.f, -5.f }, 70.f, ColorRGB{ 1.f,.8f,.45f });//front left light
		AddPointLight({ 2.5f, 2.5f, -5.f }, 50.f, ColorRGB{ .34f,.47f,.68f });
	}
	void Scene_W4_TestScene::Update(dae::Timer* pTimer)
	{
		Scene::Update(pTimer);

		const auto yawAngle = (cos(pTimer->GetTotal()) + 1.f) / 2.f * PI_2;
		for(auto& mesh : m_Meshes)
		{
			mesh->RotateY(yawAngle);
			mesh->UpdateTransforms();
		}
	}
#pragma endregion
#pragma region bunnyscene
	void Scene_W4_BunnyScene::Initialize()
	{
		sceneName = "W4 Bunny Scene";
		m_Camera.origin = { 0.f, 1.f, -5.f };
		m_Camera.fovAngle = 45.f;

		const auto matCT_GrayRoughMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, 1.f));
		const auto matCT_GrayMediumMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, .6f));
		const auto matCT_GraySmoothMetal = AddMaterial(new Material_CookTorrence({ .927f,.960f,.915f }, 1.f, .1f));
		const auto matCT_GrayRoughPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, 1.f));
		const auto matCT_GrayMediumPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, .6f));
		const auto matCT_GraySmoothPlastic = AddMaterial(new Material_CookTorrence({ .75F,.75F,.75F }, 0.f, .1f));

		const auto matLambert_GrayBlue = AddMaterial(new Material_Lambert({ .49f,.57f,.57f }, 1.f));
		const auto matLambert_White = AddMaterial(new Material_Lambert(colors::White, 1.f));

		//Plane
		AddPlane({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f }, matLambert_GrayBlue); //BACK
		AddPlane({ -5.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, matLambert_GrayBlue); //LEFT
		AddPlane({ 5.f, 0.f, 0.f }, { -1.f, 0.f, 0.f }, matLambert_GrayBlue); //RIGHT
		AddPlane({ 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, matLambert_GrayBlue); //BOTTOM
		AddPlane({ 0.f, 10.f, 0.f }, { 0.f, -1.f, 0.f }, matLambert_GrayBlue); //TOP

		m_pMesh = AddTriangleMesh(TriangleCullMode::BackFaceCulling, matLambert_White);
		Utils::ParseOBJ("resources/lowpoly_bunny.obj",
			m_pMesh->positions,
			m_pMesh->normals,
			m_pMesh->indices);

		m_pMesh->Translate({ 0.f, 1.5f, 0.f });

		m_pMesh->UpdateAABB();

		m_pMesh->UpdateTransforms();

		AddPointLight({ 0.f, 5.f, 5.f }, 50.f, ColorRGB{ 1.f,.61f,.45f });//back light
		AddPointLight({ -2.5f, 5.f, -5.f }, 70.f, ColorRGB{ 1.f,.8f,.45f });//front left light
		AddPointLight({ 2.5f, 2.5f, -5.f }, 50.f, ColorRGB{ .34f,.47f,.68f });
	}
	void Scene_W4_BunnyScene::Update(dae::Timer* pTimer)
	{
		Scene::Update(pTimer);

		m_pMesh->RotateY(PI_DIV_2 * pTimer->GetTotal());
		m_pMesh->UpdateTransforms();
	}
#pragma endregion
}

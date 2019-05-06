#include "System\Lightmaps.hpp"

#include "component\Transform.hpp"
#include "component\Model.hpp"

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>
#include <glm\geometric.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec2 texcoord;
};

struct Texel {
	glm::vec3 position;
	glm::vec3 tangent;
	glm::uvec2 texcoord;
};

inline float crossProduct(const glm::vec2& a, const glm::vec2& b) {
	return glm::cross(glm::vec3(a, 0), glm::vec3(b, 0)).z;
}

// s = v1 -> v2, t = v1 -> v3
inline glm::vec3 baryInterpolate(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, float s, float t) {
	return v1 + s * (v2 - v1) + t * (v3 - v1);
}

template <typename T>
inline void interpolateTexels(Vertex triangle[3], const glm::vec2& resolution, const T& lambda) {
	// order triangle verts by y
	//std::sort(triangle, triangle + 3, [](const Vertex& a, const Vertex& b) -> bool {
	//	return a.texcoord.y < b.texcoord.y;
	//});

	// scale texcoords to resolution
	glm::vec2 vt1 = glm::clamp(triangle[0].texcoord, { 0, 0 }, { 1, 1 }) * resolution;
	glm::vec2 vt2 = glm::clamp(triangle[1].texcoord, { 0, 0 }, { 1, 1 }) * resolution;
	glm::vec2 vt3 = glm::clamp(triangle[2].texcoord, { 0, 0 }, { 1, 1 }) * resolution;

	// find triangle bounding box
	int minX = glm::floor(glm::min(vt1.x, glm::min(vt2.x, vt3.x)));
	int maxX = glm::ceil(glm::max(vt1.x, glm::max(vt2.x, vt3.x)));
	int minY = glm::floor(glm::min(vt1.y, glm::min(vt2.y, vt3.y)));
	int maxY = glm::ceil(glm::max(vt1.y, glm::max(vt2.y, vt3.y)));

	glm::vec2 vs1 = vt2 - vt1; // vt1 vertex to vt2
	glm::vec2 vs2 = vt3 - vt1; // vt1 vertex to vt3

	for (uint32_t x = minX; x < maxX; x++) {
		for (uint32_t y = minY; y < maxY; y++) {
			glm::vec2 q{ x - vt1.x, y - vt1.y };

			q += glm::vec2{ 0.5f, 0.5f };

			float area2 = crossProduct(vs1, vs2); // area * 2 of triangle

			// find barycentric coordinates
			float s = crossProduct(q, vs2) / area2;
			float t = crossProduct(vs1, q) / area2;

			glm::vec3 position = baryInterpolate(triangle[0].position, triangle[1].position, triangle[2].position, s, t);
			//glm::vec3 normal = baryInterpolate(triangle[0].normal, triangle[1].normal, triangle[2].normal, s, t);

			// if in triangle
			if ((s >= 0) && (t >= 0) && (s + t <= 1))
				lambda({ position,{ x, y } });
		}
	}
}

void Lightmaps::configure(entityx::EventManager & events){

}

void Lightmaps::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){

}

void Lightmaps::buildLightmaps(entityx::EntityManager & entities){
	struct VertexData {
		uint32_t* indices;
		uint8_t* buffer;
	};

	std::unordered_map<uint32_t, VertexData> meshData;

	// testing container for texels
	std::vector<std::pair<glm::vec3, glm::quat>> texels;

	for (entityx::Entity entity : entities.entities_with_components<Transform, Model>()) {


	}




	// testing container
	//std::vector<std::pair<glm::vec3, glm::quat>> texels;
	//
	//for (uint32_t i = 0; i < _meshContexts.size(); i++) {
	//	// buffer vertex data from opengl
	//	const MeshContext& meshContext = _meshContexts[i];
	//
	//	glBindVertexArray(meshContext.arrayObject);
	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshContext.indexBuffer);
	//	glBindBuffer(GL_ARRAY_BUFFER, meshContext.vertexBuffer);
	//
	//	GLuint texcoordsEnabled = 0;
	//	glGetVertexAttribIuiv(_constructionInfo.texcoordAttrLoc, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texcoordsEnabled);
	//
	//	if (!texcoordsEnabled)
	//		continue;
	//
	//	size_t texcoordsOffset = 0;
	//	glGetVertexAttribPointerv(_constructionInfo.texcoordAttrLoc, GL_VERTEX_ATTRIB_ARRAY_POINTER, (void**)&texcoordsOffset);
	//
	//	uint32_t* indices = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
	//	uint8_t* buffer = (uint8_t*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//
	//	glm::vec3* positions = (glm::vec3*)(buffer);
	//	glm::vec2* texcoords = (glm::vec2*)(buffer + texcoordsOffset);
	//
	//	// find all entities with same mesh
	//	std::vector<uint64_t> entityIds;
	//
	//	_engine.iterateEntities([&](Engine::Entity& entity) {
	//		if (!entity.has<Transform, Model>())
	//			return;
	//
	//		const Model& model = *entity.get<Model>();
	//
	//		if (model.meshContextId == i + 1)
	//			entityIds.push_back(entity.id());
	//	});
	//
	//	// interpolate texel positions for each entity (due to different lightmap resolutions)
	//	for (uint64_t id : entityIds) {
	//		const Transform& transform = *_engine.getComponent<Transform>(id);
	//		const Model& model = *_engine.getComponent<Model>(id);
	//
	//		glm::mat4 modelMatrix = transform.globalMatrix();
	//
	//		// iterate over triangles in mesh
	//		for (uint32_t y = 0; y < meshContext.indexCount / 3; y++) {
	//			Vertex triangle[3];
	//
	//			for (uint8_t x = 0; x < 3; x++) {
	//				uint32_t index = indices[y * 3 + x];
	//				triangle[x] = { positions[index], texcoords[index] };
	//			}
	//
	//			glm::vec3 tangent = glm::cross(triangle[1].position - triangle[0].position, triangle[2].position - triangle[0].position);
	//
	//			interpolateTexels(triangle, model.lightmapResolution, [&](const Vertex& vertex) {
	//				glm::vec3 worldPosition = modelMatrix * glm::vec4(vertex.position, 1);
	//				glm::quat worldRotation;// = glm::inverse(glm::lookAt(worldPosition, worldPosition + tangent, { 1.f, 1.f, 1.f }));
	//
	//				texels.push_back({ worldPosition, worldRotation });
	//			});
	//		}
	//	}
	//
	//	// free data from opengl
	//	glUnmapBuffer(GL_ARRAY_BUFFER);
	//	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	//}

	// testing, change this!
	//for (auto i : texels) {
	//	Test::createAxis(_engine, "C:/Git Projects/SimpleEngine/bin/data/", i.first, i.second);
	//}
}

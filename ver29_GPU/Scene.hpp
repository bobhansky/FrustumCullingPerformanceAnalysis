#pragma once

#include <unordered_map>
#include <vector>
#include <memory>	// smart pointer
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "BVH.hpp"
#include "version.h"

struct CullingSSBOdata {
	GLuint modelMatrixSSBO;
	GLuint visibleIndecesSSBO;
	GLuint aabbSSBO; 
};

struct aabo_for_ssbo {
	glm::vec3 min;	// 12 bytes
	float padding1;	// 4 bytes padding to make it 16 bytes aligned
	glm::vec3 max;	// 12 bytes
	float padding2;	// 4 bytes padding to make it 16 bytes aligned
};

struct DrawElementsIndirectCommand {
    GLuint count;          // number of indices per instance
    GLuint instanceCount;  // number of instances to draw (updated by GPU)
    GLuint firstIndex;     // first index in the index buffer
    GLuint baseVertex;     // added to each index
    GLuint baseInstance;   // added to gl_InstanceID
};

class Scene {
public:
	// all objects added to this scene
	std::vector<Entity*> objList;
	// modelAdded[i] = j;   model i added to scene j times
	std::unordered_map<Model*, unsigned int> modelAdded;

	// update visible object in camFrustum,
	// camFrustum comes from draw() or updateVisibleObject
	std::vector<Entity*> visList;
	// only contains visible model matrix
	std::unordered_map<Model*, std::vector<glm::mat4>> instanceModelMatrices;
	// for each model, the VBO for instance model matrix, or visible index
	std::unordered_map<Model*, unsigned int> instanceModelVBO;

	std::unordered_map<Model*, std::vector<unsigned int>> visibleIndices;


	// contains all the instance model matrices
	// for non-frustum culling instance drawing
	// or for sending to SSBO in shader
	std::unordered_map<Model*, std::vector<glm::mat4>> allInstanceModelMat;
	std::unordered_map<Model*, std::vector<unsigned int>> instanceVisibleIndex;

	std::unordered_map<Model*, CullingSSBOdata> cullingSSBOdatas;
	// the aabo container
	std::unordered_map<Model*, std::vector<aabo_for_ssbo>> aabbs;

	BVHAccel* BVHaccelerator;

	// add object into object list
	void add(Entity* obj) {
		modelAdded[obj->pModel]++;
		// add all it's children
		for (auto& i : obj->children) {
			add(i.get());
		}
		objList.emplace_back(obj);
	}

	// only update visList
	void updateVisibleObject(const Frustum& camFrustum) {
		// call deconstructor if for object, here it is pointer so it is fine.
		visList.clear();
		BVHaccelerator->updateVisibleObject(BVHaccelerator->getNode(), camFrustum, visList);
	}

	void updateInstanceMat() {
		// instead of "instance_ModelMatrices.clear()" which Destroys the entire unordered_map and 
		// all the std::vector<glm::mat4> inside. Every frame I pay for rehashing and allocation cost

		// Clear only the vectors inside, Preserves the map structure.
		// Keeps the std::vector<glm::mat4> allocations alive.
		// Next frame's emplace_back() won’t reallocate unless needed — much faster.
		for (auto& [model, matrices] : instanceModelMatrices) {
			if (!matrices.empty())
				matrices.clear();
		}

		for (Entity* entity : visList) {
			if (entity->isInstanced) {
				instanceModelMatrices[entity->pModel].emplace_back(entity->transform.m_modelMatrix);
			}
		}
	}

	// draw without instance drawing
	void draw_non_instanced(Shader shader, const Frustum& camFrustum) {

#if FRUSTUM_CULLING == 1
		updateVisibleObject(camFrustum);
		for (auto i : visList) {
			i->draw(shader);
		}
		cout << "all object size: " << objList.size() << ", draw size: " << visList.size() << endl;

#else
		for (auto i : objList) {
			i->draw(shader);
		}
		cout << "all object size: " << objList.size() << ", draw size: " << objList.size() << endl;
#endif
	}

	// instance drawing
	void draw_instanced(Shader shader, const Frustum& camFrustum) {

#if FRUSTUM_CULLING == 1

		updateVisibleObject(camFrustum); // should make it indepenedent, cuz later draw_non_inctanced will be used with it combinedly
		updateInstanceMat();
		// draw each instanced model
		for (auto& [pModel, mat] : instanceModelMatrices) {
			if (mat.empty()) continue;
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, mat.size() * sizeof(glm::mat4), mat.data());

			// for each mesh
			for (unsigned int i = 0; i < pModel->meshes.size(); i++)
			{
				glBindVertexArray(pModel->meshes[i].VAO);
				glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(pModel->meshes[i].indices.size()), GL_UNSIGNED_INT, 0, mat.size());
				glBindVertexArray(0);
			}
		}
		cout << "all object size: " << objList.size() << ", draw size: " << visList.size() << endl;

#else
		for (auto& [pModel, matrices] : allInstanceModelMat) {
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

			for (unsigned int i = 0; i < pModel->meshes.size(); i++) {
				glBindVertexArray(pModel->meshes[i].VAO);
				glDrawElementsInstanced(GL_TRIANGLES,
					static_cast<unsigned int>(pModel->meshes[i].indices.size()),
					GL_UNSIGNED_INT,
					0,
					static_cast<unsigned int>(matrices.size()));
				glBindVertexArray(0);
			}
		}

		cout << "all object size: " << objList.size() << ", draw size: " << objList.size() << endl;
#endif
	}


	// with FRUSTUM_CULLING, sending visible indeces to shader
	void draw_instanced_with_ssbo(Shader shader, const Frustum& camFrustum) {
		updateVisibleObject(camFrustum);

		// update visible indeces
		visibleIndices.clear();
		for (Entity* entity : visList) {
			if (entity->isInstanced) {
				visibleIndices[entity->pModel].emplace_back(entity->id);
			}
		}

		// draw each instanced model
		for (auto& [pModel, visIndex] : visibleIndices) {
			if (visList.empty()) continue;
			// visibily index VBO, each frame it is updated for n * 4 bytes, instead of n * 64 bytes
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, visIndex.size() * sizeof(unsigned int), visIndex.data());
			// binding point is 0, which is accessed in shader
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cullingSSBOdatas[pModel].modelMatrixSSBO);

			// for each mesh
			for (unsigned int i = 0; i < pModel->meshes.size(); i++)
			{
				glBindVertexArray(pModel->meshes[i].VAO);
				glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(pModel->meshes[i].indices.size()), GL_UNSIGNED_INT, 0, visIndex.size());
				glBindVertexArray(0);
			}
		}
		cout << "all object size: " << objList.size() << ", draw size: " << visList.size() << endl;
	}

	void draw_instance_compute(Shader shader, ComputeShader cShader, const Frustum& camFrustum) {

		glm::vec4 frustumPlanes[6];
		frustumPlanes[0] = glm::vec4(camFrustum.topFace.normal.x, camFrustum.topFace.normal.y, camFrustum.topFace.normal.z, camFrustum.topFace.distance);
		frustumPlanes[1] = glm::vec4(camFrustum.bottomFace.normal.x, camFrustum.bottomFace.normal.y, camFrustum.bottomFace.normal.z, camFrustum.bottomFace.distance);
		frustumPlanes[2] = glm::vec4(camFrustum.leftFace.normal.x, camFrustum.leftFace.normal.y, camFrustum.leftFace.normal.z, camFrustum.leftFace.distance);
		frustumPlanes[3] = glm::vec4(camFrustum.rightFace.normal.x, camFrustum.rightFace.normal.y, camFrustum.rightFace.normal.z, camFrustum.rightFace.distance);
		frustumPlanes[4] = glm::vec4(camFrustum.nearFace.normal.x, camFrustum.nearFace.normal.y, camFrustum.nearFace.normal.z, camFrustum.nearFace.distance);
		frustumPlanes[5] = glm::vec4(camFrustum.farFace.normal.x, camFrustum.farFace.normal.y, camFrustum.farFace.normal.z, camFrustum.farFace.distance);

		cShader.use();
		GLint loc = glGetUniformLocation(cShader.ID, "frustumPlanes");
		glUniform4fv(loc, 6, glm::value_ptr(frustumPlanes[0]));

		for (auto& [pModel, addedNum] : modelAdded) {
			cShader.use();

			// clear the draw count everyframe.
			GLuint zero = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cullingSSBOdatas[pModel].visibleIndecesSSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

			unsigned int totalInstances = addedNum;
			unsigned int workGroupSize = 256; // same as local_size_x
			// doing ceiling division to get all instance covered
			unsigned int numGroups = (totalInstances + workGroupSize - 1) / workGroupSize;

			// binding point 0,  all model matrix 
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cullingSSBOdatas[pModel].modelMatrixSSBO);
			// binding point 1, AABB of all instances
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cullingSSBOdatas[pModel].aabbSSBO);
			// binding point 2, visible indeces
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cullingSSBOdatas[pModel].visibleIndecesSSBO);

			glDispatchCompute(numGroups, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // ensure the data is ready before next draw call


			// have this read back to cpu is slow
			GLuint visibleCount = 0;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cullingSSBOdatas[pModel].visibleIndecesSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &visibleCount);

			shader.use();
			// for each mesh
			for (unsigned int i = 0; i < pModel->meshes.size(); i++)
			{
				glBindVertexArray(pModel->meshes[i].VAO);
				glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(pModel->meshes[i].indices.size()), GL_UNSIGNED_INT, 0, visibleCount);
				glBindVertexArray(0);
			}

			cout << "all object size: " << objList.size() << ", draw size: " << visibleCount << endl;
		}


	}

	// initalize bounding box
	void initializeBVH() {
		if (BVHaccelerator) {
			delete BVHaccelerator;
			BVHaccelerator = nullptr;
		}
		BVHaccelerator = new BVHAccel(objList);
	}


	// always call it after scene adds all objects and before rendering loop
	void initialize() {
		initializeBVH();


		unordered_map<Model*, unsigned int> ids;
		// preallocate instance_ModelMatrices
		for (auto& [model, num] : modelAdded) {
			instanceModelMatrices[model].reserve(num);
			allInstanceModelMat[model].reserve(num);
			visibleIndices[model].reserve(num);

			ids[model] = 0;

#if SSBO == 0
			// instance Model Matrix VBO, it is for version_updating_model_matrix_every_frame 
			unsigned int instanceVBO;
			glGenBuffers(1, &instanceVBO);
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			// only allocate the space
			glBufferData(GL_ARRAY_BUFFER, num * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

			// set instance model's VAO data
			for (unsigned int i = 0; i < model->meshes.size(); i++)
			{
				unsigned int VAO = model->meshes[i].VAO;
				glBindVertexArray(VAO);

				// Here, the instanceVBO must be bound before setting vertex attrib pointers
				glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
				glEnableVertexAttribArray(4);
				glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
				glEnableVertexAttribArray(5);
				glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
				glEnableVertexAttribArray(6);
				glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

				glVertexAttribDivisor(3, 1);
				glVertexAttribDivisor(4, 1);
				glVertexAttribDivisor(5, 1);
				glVertexAttribDivisor(6, 1);

				glBindVertexArray(0);
			}

			instanceModelVBO[model] = instanceVBO;
		}

		for (auto& [pModel, matrices] : allInstanceModelMat) {
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, matrices.size() * sizeof(glm::mat4), matrices.data());
		}

#elif SSBO == 1

			// instance visible Index VBO, it is for ssbo instance drawing version
			unsigned int instanceVBO;
			glGenBuffers(1, &instanceVBO);
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			// only allocate the space
			glBufferData(GL_ARRAY_BUFFER, num * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

			// set instance visibility indeces data
			for (unsigned int i = 0; i < model->meshes.size(); i++) {
				unsigned int VAO = model->meshes[i].VAO;
				glBindVertexArray(VAO);

				// Here, the instanceVBO must be bound before setting vertex attrib pointers
				glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

				glEnableVertexAttribArray(3);
				// here need to use glVertexAttribIPointer for setting unsigned int data
				glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(unsigned int), (void*)0);
				glVertexAttribDivisor(3, 1);

				glBindVertexArray(0);
			}

			instanceModelVBO[model] = instanceVBO;
		}


		// set instance id and add its model matrix to allInstanceModelMat
		// so that allInstanceModelMat[model][i] will be the model matrix of the ith model
		for (Entity* entity : objList) {
			if (entity->isInstanced) {
				entity->id = ids[entity->pModel]++;
				allInstanceModelMat[entity->pModel].emplace_back(entity->transform.m_modelMatrix);
			}
		}

		// set SSBO
		for (auto& [model, mats] : allInstanceModelMat) {

			GLuint mMatSSBO = 0;
			glGenBuffers(1, &mMatSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mMatSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, allInstanceModelMat[model].size() * sizeof(glm::mat4), mats.data(), GL_DYNAMIC_DRAW);

			modelMatrixSSBOs[model] = mMatSSBO;
		}
#elif SSBO == 2
		}
		// set instance id and add its model matrix to allInstanceModelMat
		// so that allInstanceModelMat[model][i] will be the model matrix of the ith model
		for (Entity* entity : objList) {
			if (entity->isInstanced) {
				entity->id = ids[entity->pModel]++;
				allInstanceModelMat[entity->pModel].emplace_back(entity->transform.m_modelMatrix);
				AABB globalAABB = entity->getGlobalAABB();
				aabbs[entity->pModel].emplace_back(aabo_for_ssbo{ globalAABB.pMin, 0.0f, globalAABB.pMax, 0.0f});
			}
		}

		// set SSBO
		for (auto& [model, mats] : allInstanceModelMat) {
			// All model matrix SSBO
			GLuint mMatSSBO = 0;
			glGenBuffers(1, &mMatSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mMatSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, allInstanceModelMat[model].size() * sizeof(glm::mat4), mats.data(), GL_DYNAMIC_DRAW);
			cullingSSBOdatas[model].modelMatrixSSBO = mMatSSBO;

			// visible indeces SSBO
			GLuint VisSSBO = 0;
			glGenBuffers(1, &VisSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, VisSSBO);
			// assuming the wrost case, all instance are visible. Here we allocate the space only, drawCnt + num of instance
			glBufferData(GL_SHADER_STORAGE_BUFFER, (1 + modelAdded[model]) * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
			cullingSSBOdatas[model].visibleIndecesSSBO = VisSSBO;

			// AABB SSBO
			GLuint AABBSSBO = 0;
			glGenBuffers(1, &AABBSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, AABBSSBO);
			// vec3 + 8 bytes pading + vec3 + 8 bytes padding
			glBufferData(GL_SHADER_STORAGE_BUFFER, allInstanceModelMat[model].size() * 32, aabbs[model].data(), GL_DYNAMIC_DRAW);
			cullingSSBOdatas[model].aabbSSBO = AABBSSBO;
		}

#endif
	}


	// draw BVH of this scene
	void drawBoundingBox(Shader shader) {
		BVHaccelerator->drawBVH(shader, BVHaccelerator->getNode());
	}

	 
	~Scene() {
		delete BVHaccelerator;

		for (auto& [model, vbo] : instanceModelVBO)
			glDeleteBuffers(1, &vbo);
	}

};
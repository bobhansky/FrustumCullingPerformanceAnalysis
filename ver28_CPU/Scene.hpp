#pragma once

#include <unordered_map>
#include <vector>
#include <memory>	// smart pointer
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "BVH.hpp"
#include "version.h"

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
	std::unordered_map<Model*, GLuint> modelMatrixSSBOs;

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


	// with FRUSTUM_CULLING
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
			// visibily index VBO
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			// visibily index VBO, each frame it is updated for n * 4 bytes, instead of n * 64 bytes
			glBufferSubData(GL_ARRAY_BUFFER, 0, visIndex.size() * sizeof(unsigned int), visIndex.data());
			// binding point is 0, which is accessed in shader
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, modelMatrixSSBOs[pModel]);

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

		for (Entity* entity : objList) {
			if (entity->isInstanced) {
				entity->id = ids[entity->pModel]++;
				allInstanceModelMat[entity->pModel].emplace_back(entity->transform.m_modelMatrix);
			}
		}

		for (auto& [pModel, matrices] : allInstanceModelMat) {
			unsigned int instanceVBO = instanceModelVBO[pModel];
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, matrices.size() * sizeof(glm::mat4), matrices.data());
		}

#else // SSBO == 1

			// instance visible Index VBO, it is for ssbo instance drawing version
			unsigned int instanceVBO;
			glGenBuffers(1, &instanceVBO);
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
			// only allocate the space
			glBufferData(GL_ARRAY_BUFFER, num * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

			// set instance visibility indeces data
			for (unsigned int i = 0; i < model->meshes.size(); i++){
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
#pragma once
#include <list> //std::list
#include <array> //std::array
#include <memory> //std::unique_ptr
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <vector>
#include <algorithm>
#include <cassert>

#include "Camera.hpp"
#include "Model.hpp" 


using glm::vec3;
void renderCube();

// After applying T × R × S, the resulting 4×4 model matrix encodes your object’s:
// Right(local X axis in world space)
// Up(local Y axis in world space)
// Backward(local Z axis in world space — because OpenGL uses a negative Z forward convention)
// Position(world translation)
class Transform
{
public:
	// Local space information
	glm::vec3 m_trans = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_eulerRot = { 0.0f, 0.0f, 0.0f }; //In degrees
	glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

	// Global space information concatenate in matrix
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);

	//Dirty flag
	bool m_isDirty = true;

protected:
	// get the Model Matrix to the world space
	glm::mat4 getLocalModelMatrix()
	{
		const glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Y * X * Z
		const glm::mat4 rotationMatrix = rotateY * rotateX * rotateZ;

		// translation * rotation * scale (also know as TRS matrix)
		return glm::translate(glm::mat4(1.0f), m_trans) * rotationMatrix * glm::scale(glm::mat4(1.0f), m_scale);
	}
public:

	void computeModelMatrix()
	{
		m_modelMatrix = getLocalModelMatrix();
		//m_isDirty = false;
	}

	void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
	{
		m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
		//m_isDirty = false;
	}

	void setLocalPosition(const glm::vec3& newPosition)
	{
		m_trans = newPosition;
		m_isDirty = true;
	}

	void setLocalRotation(const glm::vec3& newRotation)
	{
		m_eulerRot = newRotation;
		m_isDirty = true;
	}

	void setLocalScale(const glm::vec3& newScale)
	{
		m_scale = newScale;
		m_isDirty = true;
	}

	// returns col vector
	const glm::vec3& getGlobalPosition() const
	{
		return m_modelMatrix[3];
	}

	const glm::vec3& getLocalPosition() const
	{
		return m_trans;
	}

	const glm::vec3& getLocalRotation() const
	{
		return m_eulerRot;
	}

	const glm::vec3& getLocalScale() const
	{
		return m_scale;
	}

	const glm::mat4& getModelMatrix() const
	{
		return m_modelMatrix;
	}

	// m_modelMatrix[0];
	glm::vec3 getRight() const
	{
		return m_modelMatrix[0];
	}

	// m_modelMatrix[1];
	glm::vec3 getUp() const
	{
		return m_modelMatrix[1];
	}

	// m_modelMatrix[2];
	glm::vec3 getBackward() const
	{
		return m_modelMatrix[2];
	}

	// -m_modelMatrix[2];
	glm::vec3 getForward() const
	{
		return -m_modelMatrix[2];
	}

	glm::vec3 getGlobalScale() const
	{
		return { glm::length(getRight()), glm::length(getUp()), glm::length(getBackward()) };
	}

	bool isDirty() const
	{
		return m_isDirty;
	}
};

struct Plane
{
	glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
	float     distance = 0.f;        // Distance with origin

	Plane() = default;

	// a plane can be built with a point and a normal.
	Plane(const glm::vec3& p1, const glm::vec3& norm)
		: normal(glm::normalize(norm)),
		distance(glm::dot(normal, p1)) {
	}

	// distance from point to plane
	float getSignedDistanceToPlane(const glm::vec3& point) const {
		// dot product of normal and point is the projection of point onto the plane normal
		// this gives how far the point is along the direction of the normal.
		// this offsets the result by the plane’s distance from the origin.
		return glm::dot(normal, point) - distance;
	}
};

struct Frustum
{
	Plane topFace;
	Plane bottomFace;

	Plane rightFace;
	Plane leftFace;

	Plane farFace;
	Plane nearFace;
};

struct BoundingVolume
{
	virtual bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const = 0;

	virtual bool isOnOrAbovePlane(const Plane& plane) const = 0;

	virtual bool isOnFrustum(const Frustum& camFrustum) const
	{
		return (isOnOrAbovePlane(camFrustum.leftFace) &&
			isOnOrAbovePlane(camFrustum.rightFace) &&
			isOnOrAbovePlane(camFrustum.topFace) &&
			isOnOrAbovePlane(camFrustum.bottomFace) &&
			isOnOrAbovePlane(camFrustum.nearFace) &&
			isOnOrAbovePlane(camFrustum.farFace));
	};
};


class AABB : public BoundingVolume
{
public:
	glm::vec3 center{ 0.f, 0.f, 0.f };
	// The half extension Ii, Ij, Ik
	glm::vec3 extents{ 0.f, 0.f, 0.f };

	vec3  pMin, pMax;		// two points specifiy the bound

	AABB(const glm::vec3& min, const glm::vec3& max)
		: BoundingVolume{}, center{ (max + min) * 0.5f }, extents{ max.x - center.x, max.y - center.y, max.z - center.z }
	{
		pMin = min;
		pMax = max;
	}

	AABB(const glm::vec3& inCenter, float iI, float iJ, float iK)
		: BoundingVolume{}, center{ inCenter }, extents{ iI, iJ, iK }
	{
		pMin = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
		pMax = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
	}

	AABB() {

	}

	std::array<glm::vec3, 8> getVertice() const
	{
		std::array<glm::vec3, 8> vertice;
		vertice[0] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
		vertice[1] = { center.x + extents.x, center.y - extents.y, center.z - extents.z };
		vertice[2] = { center.x - extents.x, center.y + extents.y, center.z - extents.z };
		vertice[3] = { center.x + extents.x, center.y + extents.y, center.z - extents.z };
		vertice[4] = { center.x - extents.x, center.y - extents.y, center.z + extents.z };
		vertice[5] = { center.x + extents.x, center.y - extents.y, center.z + extents.z };
		vertice[6] = { center.x - extents.x, center.y + extents.y, center.z + extents.z };
		vertice[7] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
		return vertice;
	}

	// return the diagonal of this bounding box
	vec3 Diagonal() const { return pMax - pMin; }

	// return the index of max elment in diagonal
	// which dimension is the longest one? x y z
	// helper function for dividing the bounding box
	int maxExtent() const
	{
		vec3 d = Diagonal();
		if (d.x > d.y && d.x > d.z)
			return 0;
		else if (d.y > d.z)
			return 1;
		else
			return 2;
	}

	// AABB Plane intersection,
	// if AABB intersects the plane
	//see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
	bool isOnOrAbovePlane(const Plane& plane) const final
	{
		// frustum's normal is inward facing direction,

		// projected radius of the AABB onto the plane normal,
		// the maximum reach the box has along the normal of the plane.
		// extents is always non negative,for plane normal, we always use the positive normal side
		const float r = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) +
			extents.z * std::abs(plane.normal.z);

		//AABB can reach ± r units from its center along the plane normal.
		return -r <= plane.getSignedDistanceToPlane(center);
	}

	bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const final
	{
		const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };
		// Scaled orientation
		// local X axis in world space * extent.x
		// actually modelMatrix * extent 
		const glm::vec3 right = transform.getRight() * extents.x;
		const glm::vec3 up = transform.getUp() * extents.y;
		const glm::vec3 forward = transform.getForward() * extents.z;

		// The half extension Ii, Ij, Ik for global AABB
		const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

		const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

		const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

		const AABB globalAABB(globalCenter, newIi, newIj, newIk);

		return (globalAABB.isOnOrAbovePlane(camFrustum.leftFace) &&
			globalAABB.isOnOrAbovePlane(camFrustum.rightFace) &&
			globalAABB.isOnOrAbovePlane(camFrustum.topFace) &&
			globalAABB.isOnOrAbovePlane(camFrustum.bottomFace) &&
			globalAABB.isOnOrAbovePlane(camFrustum.nearFace) &&
			globalAABB.isOnOrAbovePlane(camFrustum.farFace));
	};
};

Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar)
{
	Frustum     frustum;
	// half verical side
	const float halfVSide = zFar * tanf(fovY * .5f);
	const float halfHSide = halfVSide * aspect;
	const glm::vec3 frontMultFar = zFar * cam.Front;

	// a plane can be built with a point that the plane passes through and a normal.
	// all facing toward the inner frustum
	// facing from +y to -y:
	//     far face
	//  --------------
	//  \            /
	//   \          /		right face
	//    \        /
	//     \______/
	//     near face
	//     view fron here

	frustum.nearFace = { cam.Position + zNear * cam.Front, cam.Front };
	frustum.farFace = { cam.Position + frontMultFar, -cam.Front };
	frustum.leftFace = { cam.Position, glm::cross(frontMultFar - cam.Right * halfHSide, cam.Up) };
	frustum.rightFace = { cam.Position, glm::cross(cam.Up, frontMultFar + cam.Right * halfHSide) };
	frustum.topFace = { cam.Position, glm::cross(-cam.Right, frontMultFar + cam.Up * halfVSide) };
	frustum.bottomFace = { cam.Position, glm::cross(cam.Right, frontMultFar - cam.Up * halfVSide) };
	return frustum;
}

// per model AABB
AABB generateAABB(const Model& model)
{
	glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
	for (auto&& mesh : model.meshes)
	{
		for (auto&& vertex : mesh.vertices)
		{
			minAABB.x = std::min(minAABB.x, vertex.Position.x);
			minAABB.y = std::min(minAABB.y, vertex.Position.y);
			minAABB.z = std::min(minAABB.z, vertex.Position.z);

			maxAABB.x = std::max(maxAABB.x, vertex.Position.x);
			maxAABB.y = std::max(maxAABB.y, vertex.Position.y);
			maxAABB.z = std::max(maxAABB.z, vertex.Position.z);
		}
	}
	return AABB(minAABB, maxAABB);
}

// union two bounds
AABB Union(AABB& b1, AABB& b2) {
	vec3 min(fmin(b1.pMin.x, b2.pMin.x),
		fmin(b1.pMin.y, b2.pMin.y),
		fmin(b1.pMin.z, b2.pMin.z)
	);

	vec3 max(fmax(b1.pMax.x, b2.pMax.x),
		fmax(b1.pMax.y, b2.pMax.y),
		fmax(b1.pMax.z, b2.pMax.z)
	);

	return AABB(min, max);
}

class Entity
{
public:
	//Scene graph
	std::list<std::unique_ptr<Entity>> children;
	Entity* parent = nullptr;
	// used for intanced drawing
	bool isInstanced = false;

	//Space information
	Transform transform;

	Model* pModel = nullptr;
	// model space AABB
	std::unique_ptr<AABB> boundingVolume;
	std::unique_ptr<AABB> globalBoundingVolume;

	// id of this entity, used for instancing, updated by the scene
	unsigned int id = 0;	


	// constructor, expects a filepath to a 3D model.
	Entity(Model& model, bool isIns) : pModel{ &model }, isInstanced(isIns)
	{
		boundingVolume = std::make_unique<AABB>(generateAABB(model));
		globalBoundingVolume = std::make_unique<AABB>(getGlobalAABB());
	}

	// get the AABB in global space, after applying its model matrix
	AABB getGlobalAABB()
	{
		// global space center
		const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(boundingVolume->center, 1.f) };

		// Scaled orientation
		const glm::vec3 right = transform.getRight() * boundingVolume->extents.x;
		const glm::vec3 up = transform.getUp() * boundingVolume->extents.y;
		const glm::vec3 forward = transform.getForward() * boundingVolume->extents.z;

		const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

		const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

		const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
			std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

		return AABB(globalCenter, newIi, newIj, newIk);
	}

	//Add child. Argument input is argument of any constructor that you create. By default you can use the default constructor and don't put argument input.
	template<typename... TArgs>
	void addChild(TArgs&&... args)
	{
		// suppose Entity(std::string name, int id);
		// std::make_unique<Entity>("Player", 42);
		children.emplace_back(std::make_unique<Entity>(args...));
		children.back()->parent = this;
	}

	//Update transform if it was changed
	void updateSelfAndChild()
	{
		if (transform.isDirty()) {
			forceUpdateSelfAndChild();
			return;
		}

		// auto&& is a universal (or forwarding) reference 
		// If passed an lvalue	Deduces to T& (lvalue ref)
		// If passed an rvalue	Deduces to T && (rvalue ref)
		for (auto&& child : children)
		{
			child->updateSelfAndChild();
		}
	}

	//Force update of transform even if local space don't change
	void forceUpdateSelfAndChild()
	{
		if (parent) {
			transform.computeModelMatrix(parent->transform.getModelMatrix());
		}
		else {
			transform.computeModelMatrix();
		}
		if (transform.m_isDirty) {
			globalBoundingVolume = std::make_unique<AABB>(getGlobalAABB());
			transform.m_isDirty = false;
		}

		for (auto&& child : children)
		{
			child->forceUpdateSelfAndChild();
		}
	}


	void drawSelfAndChild(const Frustum& frustum, Shader& ourShader, unsigned int& display, unsigned int& total)
	{
#if FRUSTUM_CULLING == 1
		if (boundingVolume->isOnFrustum(frustum, transform))
		{
			ourShader.setMat4("model", transform.getModelMatrix());
			pModel->Draw(ourShader);
			display++;
		}
		total++;
#else
		ourShader.setMat4("model", transform.getModelMatrix());
		pModel->Draw(ourShader);
		display++;
		total++;
#endif
		for (auto&& child : children)
		{
			child->drawSelfAndChild(frustum, ourShader, display, total);
		}
	}

	void draw(Shader& ourShader)
	{
		ourShader.setMat4("model", transform.getModelMatrix());
		pModel->Draw(ourShader);
	}
};



// tree node, contains only ONE entity
class BVHNode {
public:
	Entity* obj;
	// world space AABB
	AABB bound;
	BVHNode* left = nullptr;
	BVHNode* right = nullptr;


	~BVHNode() {
	}

	// test if this node's AABB is on frustum
	bool isOnFrustum(const Frustum& camFrustum) const{

		return (bound.isOnOrAbovePlane(camFrustum.leftFace) &&
			bound.isOnOrAbovePlane(camFrustum.rightFace) &&
			bound.isOnOrAbovePlane(camFrustum.topFace) &&
			bound.isOnOrAbovePlane(camFrustum.bottomFace) &&
			bound.isOnOrAbovePlane(camFrustum.nearFace) &&
			bound.isOnOrAbovePlane(camFrustum.farFace));
	};
};


// BVH acceleration class
// contains a root BVHNode and algorithms to getIntersection with bounds
class BVHAccel {
public:
	// the scene passes in the objList
	BVHAccel(std::vector<Entity*> objList) : objects(objList) {
		
		root = recursiveBuild(objects);
	}


	~BVHAccel() {
		deleteBVHtree(root);
	}

	// build the BVH tree based on the objList
	// the Obj in the list has its own BoundBox
	// we use them to initialize the tree
	BVHNode* recursiveBuild(std::vector<Entity*> objList) {
		BVHNode* res = new BVHNode();

		// build BVH depending on the size of objList
		if (objList.size() == 0) return res;

		else if (objList.size() == 1) {
			res->bound = *objList.at(0)->globalBoundingVolume;
			res->left = nullptr;
			res->right = nullptr;
			res->obj = objList[0];
			return res;
		}

		else if (objList.size() == 2) {		
			res->left = recursiveBuild({ objList[0] });
			res->right = recursiveBuild({ objList[1] });

			res->bound = Union(res->left->bound, res->right->bound);
			return res;
		}

		else {	// multiple objects, then divide the box along the longest dimension
			// first union all the object
			// 5/6/2023: I loop and union objects here, which is very slow but the result 
			// seems to be correct.
			AABB unionBound = Union(*objList[0]->globalBoundingVolume, *objList[1]->globalBoundingVolume);
			for (int i = 2; i < objList.size(); i++) {
				unionBound = Union(unionBound, *objList[i]->globalBoundingVolume);
			}

			// first find the longest dimension
			// then sort objects by this dimension
			// find the middle object and divide the bound into two
			int longest = unionBound.maxExtent();
			switch (longest)
			{
			case 0: {	// x dimension is the longest
				std::sort(objList.begin(), objList.end(),
					[](Entity* o1, Entity* o2) -> bool {
						return o1->globalBoundingVolume->center.x < o2->globalBoundingVolume->center.x;
					});
				break;
			}
			case 1: {  // y dimension is the longest
				std::sort(objList.begin(), objList.end(),
					[](Entity* o1, Entity* o2) -> bool {
						return o1->globalBoundingVolume->center.y < o2->globalBoundingVolume->center.y;
					});
				break;
			}
			case 2: {  // z dimension is the longest
				std::sort(objList.begin(), objList.end(),
					[](Entity* o1, Entity* o2) -> bool {
						return o1->globalBoundingVolume->center.z < o2->globalBoundingVolume->center.z;
					});
				break;
			}
			}

			std::vector<Entity*>::iterator begin = objList.begin();
			auto middle = begin + (objList.size() / 2);
			auto end = objList.end();

			std::vector<Entity*> leftObjects(begin, middle);
			std::vector<Entity*> rightObjects(middle, end);

			assert(objList.size() == rightObjects.size() + leftObjects.size());

			res->left = recursiveBuild(leftObjects);
			res->right = recursiveBuild(rightObjects);

			res->bound = Union(res->left->bound, res->right->bound);
		}

		return res;
	}

	BVHNode* getNode() { return root; }

	// find visible object and add them to visList 
	void updateVisibleObject(BVHNode* node, const Frustum& frustum, std::vector<Entity*>& visList) {
		if (!node) return;

		if (!node->isOnFrustum(frustum)) return;

		// if the node is a leaf node
		// add the entity to list
		if (!node->left && !node->right) {
			visList.emplace_back(node->obj);
			return;
		}

		// test its sub nodes
		updateVisibleObject(node->left, frustum, visList);
		updateVisibleObject(node->right, frustum, visList);
	}

	void drawBVH(Shader shader, BVHNode* node) {
		if (!node) return;

		if (node->left) drawBVH(shader, node->left);
		if (node->right) drawBVH(shader, node->right);

		glm::mat4 model(1);
		model = glm::translate(model, node->bound.center);
		model = glm::scale(model, glm::vec3(node->bound.extents.x, node->bound.extents.y, node->bound.extents.z));
		shader.setMat4("model", model);
		renderCube();
	}
private:
	std::vector<Entity*> objects;
	BVHNode* root;					// root of the tree


	void deleteBVHtree(BVHNode* node) {
		if (!node) return;

		if (node->left) deleteBVHtree(node->left);
		if (node->right) deleteBVHtree(node->right);

		delete node;
	}
};

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


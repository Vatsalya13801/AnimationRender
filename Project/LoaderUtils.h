#pragma once

#include <iostream>
#include <directxmath.h>
#include <vector>

// FBX includes
#include <fbxsdk.h>
#include "MeshUtils.h"
#include <string>
#include "Anim.h"

FbxManager* gSdkManager;
//float scale = 1.0f;// / 40.0f;
FbxScene* fbxScene;
anim_clip_t finalClip;

// funtime random normal
//#define RAND_NORMAL XMFLOAT3(rand()/float(RAND_MAX),rand()/float(RAND_MAX),rand()/float(RAND_MAX))
const int MAX_INFLUENCES = 4;
struct influence
{
	int joint;
	float weight;
};

struct JoWe
{
	XMFLOAT4 weights;
	int jIndex[4];
};
std::vector<JoWe> JOIWEI;
using influence_set = std::array<influence, MAX_INFLUENCES>;

// Add FBX mesh process function declaration here
void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename,float scale);
void ProcessFBXMesh1(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename,float scale);
void ProcessFBXMesh2(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename,float scale);

void InitFBX()
{
	gSdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(gSdkManager, IOSROOT);
	gSdkManager->SetIOSettings(ios);
}

struct fbx_joint
{
	FbxNode* node;
	int parent;
};

using fbx_joint_set = std::vector<fbx_joint>;

FbxSkin* mesh_skin(FbxMesh& mesh)
{
	for (int i = 0; i < mesh.GetDeformerCount(); ++i)
	{
		auto deformer = mesh.GetDeformer(i);

		if (deformer->Is<FbxSkin>())
			return (FbxSkin*)deformer;
	}

	return nullptr;
}

fbx_joint_set get_bindpose(FbxMesh& mesh)
{
	FbxSkin* skin = mesh_skin(mesh);

	FbxNode* link = skin->GetCluster(0)->GetLink();

	FbxSkeleton* skeleton_root = link->GetSkeleton();

	assert(skeleton_root);

	while (!skeleton_root->IsSkeletonRoot())
	{
		link = link->GetParent();
		skeleton_root = link->GetSkeleton();
	}
	
	fbx_joint_set joints;

	joints.push_back(fbx_joint{ link, -1 });

	for (int i = 0; i < joints.size(); ++i)
	{
		int child_count = joints[i].node->GetChildCount();

		for (int c = 0; c < child_count; ++c)
		{
			FbxNode* child = joints[i].node->GetChild(c);

			if (child->GetSkeleton())
				joints.push_back(fbx_joint{ child, i });
		}
	}

	return std::move(joints);
	
}

anim_clip_t LoadAnimation(FbxScene* lScene, FbxMesh* mesh, SimpleMesh<SimpleVertex>& simpleMesh)
{
	
	fbx_joint_set bind_pose = get_bindpose(*mesh);

	FbxSkin* skin = mesh_skin(*mesh);

	std::vector<influence_set>control_point_influencs;

	control_point_influencs.resize(mesh->GetControlPointsCount());

	int n = skin->GetClusterCount();

	for (int i = 0; i < n; i++)
	{
		auto cluster = skin->GetCluster(i);

		int jIndex = -1;

		for (int j = 0; j < bind_pose.size(); j++)
		{
			if (bind_pose[j].node == cluster->GetLink())
			{
				jIndex = j;
				break;
			}
		}

		int cpiCount = cluster->GetControlPointIndicesCount();
		auto cpi = cluster->GetControlPointIndices();
		auto weights = cluster->GetControlPointWeights();

		for (int m = 0; m < cpiCount; m++)
		{
			int index = cpi[m];

			influence_set& influenceset = control_point_influencs[index];
			influence temp{jIndex,(float)weights[m] };
			for (auto& info : influenceset)
			{
				if (info.weight < temp.weight)
				{
					std::swap(temp, info);
					int nothing = 0;
				}
			}
		}

	}
	JOIWEI.resize(control_point_influencs.size());
	for (int j = 0; j < control_point_influencs.size(); j++)
	{
		JOIWEI[j].jIndex[0] = control_point_influencs[j][0].joint;
		JOIWEI[j].jIndex[1] = control_point_influencs[j][1].joint;
		JOIWEI[j].jIndex[2] = control_point_influencs[j][2].joint;
		JOIWEI[j].jIndex[3] = control_point_influencs[j][3].joint;
		float TW = control_point_influencs[j][0].weight + control_point_influencs[j][1].weight + control_point_influencs[j][2].weight + control_point_influencs[j][3].weight;
		control_point_influencs[j][0].weight /= TW;
		control_point_influencs[j][1].weight /= TW;
		control_point_influencs[j][2].weight /= TW;
		control_point_influencs[j][3].weight /= TW;
		JOIWEI[j].weights = { control_point_influencs[j][0].weight ,control_point_influencs[j][1].weight ,control_point_influencs[j][2].weight ,control_point_influencs[j][3].weight };
	}
	anim_clip_t anim_clip;
	//Load animation data
	bind_pose = get_bindpose(*mesh);
	
	int joint_count = (int)bind_pose.size();

	auto anim_stack = lScene->GetCurrentAnimationStack();
	FbxTimeSpan local_time_span = anim_stack->GetLocalTimeSpan();
	FbxTime timer = local_time_span.GetDuration();
	float duration = (float)timer.GetSecondDouble();
	int frame_count = (int)timer.GetFrameCount(FbxTime::eFrames24);
	anim_clip.duration = duration;

	for (int frame = 0; frame < frame_count; ++frame)
	{
		keyframe_t key_frame;

		key_frame.joints.resize(bind_pose.size());

		timer.SetFrame(frame, FbxTime::eFrames24);
		key_frame.time = (float)timer.GetSecondDouble();

		for (int j = 0; j < bind_pose.size(); ++j)
		{
			joint_t& joint = key_frame.joints[j];
			joint.parent = bind_pose[j].parent;
			FbxAMatrix mat = bind_pose[j].node->EvaluateGlobalTransform(timer);

			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					joint.transform[r][c] = (float)mat[r][c];
				}
			}
		}
		anim_clip.keyframes.push_back(std::move(key_frame));
	}
	return move(anim_clip);
}



void LoadFBX(const std::string& filename, SimpleMesh<SimpleVertex> &simpleMesh, std::vector<std::string>& textureFilename, float scale, anim_clip_t& animation)
{
	const char* ImportFileName = filename.c_str(); 
	
	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager, "");

	FbxImporter* lImporter = FbxImporter::Create(gSdkManager, "");

	// Initialize the importer by providing a filename.
	if (!lImporter->Initialize(ImportFileName, -1, gSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		//exit(-1);
	}

	// Import the scene.
	bool lStatus = lImporter->Import(lScene);

	// Destroy the importer
	lImporter->Destroy();

	fbxScene = lScene;
	//FbxNode* Node = lScene->GetRootNode();
	//while (true)
	//{
	//	int childrenCount = lScene->GetRootNode()->GetChildCount();
	//	for (int i = 0; i < childrenCount; i++)
	//	{
	//		FbxNode* childNode = Node->GetChild(i);
	//		FbxMesh* mesh = childNode->GetMesh();
	//
	//		if (mesh != NULL)
	//		{
	//			LoadAnimation(lScene, mesh);
	//		}
	//	}
	//	Node = Node.
	//}
	
	// Process the scene and build DirectX Arrays
	ProcessFBXMesh(lScene->GetRootNode(), simpleMesh, textureFilename[0],scale);
	ProcessFBXMesh1(lScene->GetRootNode(), simpleMesh, textureFilename[1], scale);
	ProcessFBXMesh2(lScene->GetRootNode(), simpleMesh, textureFilename[2], scale);
	// Optimize the mesh
	
	MeshUtils::Compactify(simpleMesh);
	animation = finalClip;
	
	// Destroy the (no longer needed) scene
	lScene->Destroy();
}

void LoadFBX1(const std::string& filename, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename, float scale)
{
	const char* ImportFileName = filename.c_str();

	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager, "");

	FbxImporter* lImporter = FbxImporter::Create(gSdkManager, "");

	// Initialize the importer by providing a filename.
	if (!lImporter->Initialize(ImportFileName, -1, gSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		//exit(-1);
	}

	// Import the scene.
	bool lStatus = lImporter->Import(lScene);

	// Destroy the importer
	lImporter->Destroy();

	// Process the scene and build DirectX Arrays
	ProcessFBXMesh1(lScene->GetRootNode(), simpleMesh, textureFilename, scale);

	// Optimize the mesh
	MeshUtils::Compactify(simpleMesh);

	// Destroy the (no longer needed) scene
	lScene->Destroy();
}

void LoadFBX2(const std::string& filename, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename, float scale)
{
	const char* ImportFileName = filename.c_str();

	// Create a scene
	FbxScene* lScene = FbxScene::Create(gSdkManager, "");

	FbxImporter* lImporter = FbxImporter::Create(gSdkManager, "");

	// Initialize the importer by providing a filename.
	if (!lImporter->Initialize(ImportFileName, -1, gSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		//exit(-1);
	}

	// Import the scene.
	bool lStatus = lImporter->Import(lScene);

	// Destroy the importer
	lImporter->Destroy();

	// Process the scene and build DirectX Arrays
	ProcessFBXMesh2(lScene->GetRootNode(), simpleMesh, textureFilename, scale);

	// Optimize the mesh
	MeshUtils::Compactify(simpleMesh);

	// Destroy the (no longer needed) scene
	lScene->Destroy();
}


string getFileName(const string& s)
{
	// look for '\\' first
	char sep = '/';

	size_t i = s.rfind(sep, s.length());
	if (i != string::npos) {
		return(s.substr(i + 1, s.length() - i));
	}
	else // try '/'
	{
		sep = '\\';
		i = s.rfind(sep, s.length());
		if (i != string::npos) {
			return(s.substr(i + 1, s.length() - i));
		}
	}
	return("");
}

// from C++ Cookbook by D. Ryan Stephens, Christopher Diggins, Jonathan Turkanis, Jeff Cogswell
// https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html
void replaceExt(string& s, const string& newExt) {

	string::size_type i = s.rfind('.', s.length());

	if (i != string::npos) {
		s.replace(i + 1, newExt.length(), newExt);
	}
}

void ProcessFBXMesh(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename,float scale)
{
	int childrenCount = Node->GetChildCount();

	cout << "\nName:" << Node->GetName();
	// check each child node for a FbxMesh
	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		// Found a mesh on this node
		if (mesh != NULL)
		{
			finalClip = LoadAnimation(fbxScene, mesh, simpleMesh);
			cout << "\nMesh:" << childNode->GetName();
			
			// Get index count from mesh
			int numVertices = mesh->GetControlPointsCount();
			cout << "\nVertex Count:" << numVertices;

			// Resize the vertex vector to size of this mesh
			simpleMesh.vertexList.resize(numVertices);

			//================= Process Vertices ===============
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0] * scale;
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1] * scale;
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2] * scale;
				// Generate random normal for first attempt at getting to render
				//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
			}
			
			int numIndices = mesh->GetPolygonVertexCount();
			cout << "\nIndice Count:" << numIndices;

			// No need to allocate int array, FBX does for us
			int* indices = mesh->GetPolygonVertices();

			// Fill indiceList
			simpleMesh.indicesList.resize(numIndices);
			memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));
		
			// Get the Normals array from the mesh
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			cout << "\nNormalVec Count:" << normalsVec.Size();

			//get all UV set names
			FbxStringList lUVSetNameList;
			mesh->GetUVSetNames(lUVSetNameList);
			const char* lUVSetName = lUVSetNameList.GetStringAt(0);
			const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);
			
			// Declare a new vector for the expanded vertex data
			// Note the size is numIndices not numVertices
			vector<SimpleVertex> vertexListExpanded;
			vertexListExpanded.resize(numIndices);

			// align (expand) vertex array and set the normals
			for (int j = 0; j < numIndices; j++)
			{
				// copy the original vertex position to the new vector
				// by using the index to look up the correct vertex
				// this is the "unindexing" step
				vertexListExpanded[j].Pos.x = simpleMesh.vertexList[indices[j]].Pos.x;
				vertexListExpanded[j].Pos.y = simpleMesh.vertexList[indices[j]].Pos.y;
				vertexListExpanded[j].Pos.z = simpleMesh.vertexList[indices[j]].Pos.z;

				vertexListExpanded[j].jIndex[0] = JOIWEI[indices[j]].jIndex[0];
				vertexListExpanded[j].jIndex[1] = JOIWEI[indices[j]].jIndex[1];
				vertexListExpanded[j].jIndex[2] = JOIWEI[indices[j]].jIndex[2];
				vertexListExpanded[j].jIndex[3] = JOIWEI[indices[j]].jIndex[3];
				vertexListExpanded[j].weights = JOIWEI[indices[j]].weights;
				// copy normal data directly, no need to unindex
				vertexListExpanded[j].Normal.x = (float)normalsVec.GetAt(j)[0];
				vertexListExpanded[j].Normal.y = (float)normalsVec.GetAt(j)[1];
				vertexListExpanded[j].Normal.z = (float)normalsVec.GetAt(j)[2];

				if (lUVElement->GetReferenceMode() == FbxLayerElement::eDirect)
				{
					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(indices[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
				else if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					auto& index_array = lUVElement->GetIndexArray();

					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(index_array[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
			}

			// make new indices to match the new vertexListExpanded
			vector<int> indicesList;
			indicesList.resize(numIndices);
			for (int j = 0; j < numIndices; j++)
			{
				indicesList[j] = j; //literally the index is the count
			}

			// copy working data to the global SimpleMesh
			simpleMesh.indicesList = indicesList;
			simpleMesh.vertexList = vertexListExpanded;

		//================= Texture ========================================

		int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
		//cout << "\nmaterial count: " << materialCount << std::endl;

		for (int index = 0; index < materialCount; index++)
		{
			FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
			//cout << "\nmaterial: " << material << std::endl;

			if (material != NULL)
			{
				//cout << "\nmaterial: " << material->GetName() << std::endl;
				// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
				FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

				// Check if it's layeredtextures
				int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();

				if (layeredTextureCount > 0)
				{
					for (int j = 0; j < layeredTextureCount; j++)
					{
						FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
						int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();

						for (int k = 0; k < lcount; k++)
						{
							FbxFileTexture* texture = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
							// Then, you can get all the properties of the texture, include its name
							const char* textureName = texture->GetFileName();
							//cout << textureName;
						}
					}
				}
				else
				{
					// Directly get textures
					int textureCount = prop.GetSrcObjectCount<FbxTexture>();
					for (int j = 0; j < textureCount; j++)
					{
						FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
						// Then, you can get all the properties of the texture, include its name
						const char* textureName = texture->GetFileName();
						//cout << "\nTexture Filename " << textureName;
						textureFilename = textureName;
						FbxProperty p = texture->RootProperty.Find("Filename");
						//cout << p.Get<FbxString>() << std::endl;

					}
				}

				// strip out the path and change the file extension
				textureFilename = getFileName(textureFilename);
				replaceExt(textureFilename, "dds");
				//cout << "\nTexture Filename " << textureFilename << endl;

			}
		}
	}
	// did not find a mesh here so recurse
		else
		ProcessFBXMesh(childNode, simpleMesh, textureFilename,scale);
	}
}

// Grabbing Animation Data
//void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex);
//
//void ProcessSkeletonHierarchy(FbxNode* inRootNode)
//{
//	for (int childIndex = 0; childIndex < inRootNode->GetChildCount(); ++childIndex)
//	{
//		FbxNode* currNode = inRootNode->GetChild(childIndex);
//		ProcessSkeletonHierarchyRecursively(currNode, 0, 0, -1);
//	}
//}
//void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int inDepth, int myIndex, int inParentIndex)
//{
//	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
//	{
//		Joint currJoint;
//		currJoint.mParentIndex = inParentIndex;
//		currJoint.mName = inNode->GetName();
//		mSkeleton.joints.push_back(currJoint);
//	}
//
//	for (int i = 0; i < inNode->GetChildCount(); i++)
//	{
//		ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), inDepth + 1, mSkeleton.joints.size(), myIndex);
//	}
//}
//
//unsigned FindJoint(std::string name)
//{
//	for (int i = 0; i < mSkeleton.joints.size(); i++)
//	{
//		if (mSkeleton.joints[i].mName == name)
//		{
//			return i;
//		}
//	}
//}
//
//void ProcessJointsandAnimations(FbxNode* inNode)
//{
//	FbxMesh* currMesh = inNode->GetMesh();
//	unsigned Deformers = currMesh->GetDeformerCount();
//
//	FbxAMatrix geometryTransform = FbxAMatrix();
//
//	for (unsigned  dIndex = 0; dIndex < Deformers; dIndex++)
//	{
//		
//		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(currMesh->GetDeformer(dIndex, FbxDeformer::eSkin));
//
//		if (!currSkin)
//			continue;
//		unsigned noofClusters = currSkin->GetClusterCount();
//		for (unsigned clusterIndex = 0; clusterIndex < noofClusters; clusterIndex++)
//		{
//			FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
//			std::string currJointName = currCluster->GetLink()->GetName();
//			unsigned currJointIndex = FindJoint(currJointName);
//			FbxAMatrix transformM;
//			FbxAMatrix transformLM;
//			FbxAMatrix globalBindPoseIM;
//			currCluster->GetTransformMatrix(transformM);
//			currCluster->GetTransformLinkMatrix(transformLM);
//			globalBindPoseIM = transformLM.Inverse() * transformM * geometryTransform;
//
//			mSkeleton.joints[currJointIndex].mGlobalBindPoseIM = globalBindPoseIM;
//			mSkeleton.joints[currJointIndex].mNode = currCluster->GetLink();
//			
//			unsigned noofIndices = currCluster->GetControlPointIndicesCount();
//			for (unsigned i = 0; i < noofIndices; i++)
//			{
//				WeightIndex weightI;
//				weightI.Weight = *currCluster->GetControlPointWeights();
//				weightI.index = currJointIndex;
//				wi.push_back(weightI);
//			}
//			
//
//			FbxAnimStack* currAnimStack;
//			currAnimStack->AddMember(mFbxScene->GetSrcObject(0));
//			FbxString animStackName = currAnimStack->GetName();
//			mAnimationName = animStackName.Buffer();
//			FbxTakeInfo* takeinfo = mFbxScene->GetTakeInfo(animStackName);
//			FbxTime start = takeinfo->mLocalTimeSpan.GetStart();
//			FbxTime end = takeinfo->mLocalTimeSpan.GetStop();
//			mAnimationLength = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;
//			mSkeleton.joints[currJointIndex].;
//		}
//	}
//}





void ProcessFBXMesh1(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename, float scale)
{
	int childrenCount = Node->GetChildCount();
	cout << "\nName:" << Node->GetName();
	// check each child node for a FbxMesh
	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		// Found a mesh on this node
		if (mesh != NULL)
		{
			cout << "\nMesh:" << childNode->GetName();

			// Get index count from mesh
			int numVertices = mesh->GetControlPointsCount();
			cout << "\nVertex Count:" << numVertices;

			// Resize the vertex vector to size of this mesh
			simpleMesh.vertexList.resize(numVertices);

			//================= Process Vertices ===============
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0] * scale;
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1] * scale;
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2] * scale;
				// Generate random normal for first attempt at getting to render
				//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
			}

			int numIndices = mesh->GetPolygonVertexCount();
			cout << "\nIndice Count:" << numIndices;

			// No need to allocate int array, FBX does for us
			int* indices = mesh->GetPolygonVertices();

			// Fill indiceList
			simpleMesh.indicesList.resize(numIndices);
			memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));

			// Get the Normals array from the mesh
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			cout << "\nNormalVec Count:" << normalsVec.Size();

			//get all UV set names
			FbxStringList lUVSetNameList;
			mesh->GetUVSetNames(lUVSetNameList);
			const char* lUVSetName = lUVSetNameList.GetStringAt(0);
			const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);

			// Declare a new vector for the expanded vertex data
			// Note the size is numIndices not numVertices
			vector<SimpleVertex> vertexListExpanded;
			vertexListExpanded.resize(numIndices);

			// align (expand) vertex array and set the normals
			for (int j = 0; j < numIndices; j++)
			{
				// copy the original vertex position to the new vector
				// by using the index to look up the correct vertex
				// this is the "unindexing" step
				vertexListExpanded[j].Pos.x = simpleMesh.vertexList[indices[j]].Pos.x;
				vertexListExpanded[j].Pos.y = simpleMesh.vertexList[indices[j]].Pos.y;
				vertexListExpanded[j].Pos.z = simpleMesh.vertexList[indices[j]].Pos.z;

				vertexListExpanded[j].jIndex[0] = JOIWEI[indices[j]].jIndex[0];
				vertexListExpanded[j].jIndex[1] = JOIWEI[indices[j]].jIndex[1];
				vertexListExpanded[j].jIndex[2] = JOIWEI[indices[j]].jIndex[2];
				vertexListExpanded[j].jIndex[3] = JOIWEI[indices[j]].jIndex[3];
				vertexListExpanded[j].weights = JOIWEI[indices[j]].weights;
				// copy normal data directly, no need to unindex
				vertexListExpanded[j].Normal.x = (float)normalsVec.GetAt(j)[0];
				vertexListExpanded[j].Normal.y = (float)normalsVec.GetAt(j)[1];
				vertexListExpanded[j].Normal.z = (float)normalsVec.GetAt(j)[2];

				if (lUVElement->GetReferenceMode() == FbxLayerElement::eDirect)
				{
					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(indices[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
				else if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					auto& index_array = lUVElement->GetIndexArray();

					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(index_array[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
			}

			// make new indices to match the new vertexListExpanded
			vector<int> indicesList;
			indicesList.resize(numIndices);
			for (int j = 0; j < numIndices; j++)
			{
				indicesList[j] = j; //literally the index is the count
			}

			// copy working data to the global SimpleMesh
			simpleMesh.indicesList = indicesList;
			simpleMesh.vertexList = vertexListExpanded;

			//================= Texture ========================================

			int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
			//cout << "\nmaterial count: " << materialCount << std::endl;

			for (int index = 0; index < materialCount; index++)
			{
				FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
				//cout << "\nmaterial: " << material << std::endl;

				if (material != NULL)
				{
					//cout << "\nmaterial: " << material->GetName() << std::endl;
					// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
					FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sEmissive);

					// Check if it's layeredtextures
					int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();

					if (layeredTextureCount > 0)
					{
						for (int j = 0; j < layeredTextureCount; j++)
						{
							FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
							int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();

							for (int k = 0; k < lcount; k++)
							{
								FbxFileTexture* texture = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
								// Then, you can get all the properties of the texture, include its name
								const char* textureName = texture->GetFileName();
								//cout << textureName;
							}
						}
					}
					else
					{
						// Directly get textures
						int textureCount = prop.GetSrcObjectCount<FbxTexture>();
						for (int j = 0; j < textureCount; j++)
						{
							FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
							// Then, you can get all the properties of the texture, include its name
							const char* textureName = texture->GetFileName();
							//cout << "\nTexture Filename " << textureName;
							textureFilename = textureName;
							FbxProperty p = texture->RootProperty.Find("Filename");
							//cout << p.Get<FbxString>() << std::endl;

						}
					}

					// strip out the path and change the file extension
					textureFilename = getFileName(textureFilename);
					replaceExt(textureFilename, "dds");
					//cout << "\nTexture Filename " << textureFilename << endl;

				}
			}
		}
		// did not find a mesh here so recurse
		else
			ProcessFBXMesh1(childNode, simpleMesh, textureFilename, scale);
	}
}

void ProcessFBXMesh2(FbxNode* Node, SimpleMesh<SimpleVertex>& simpleMesh, std::string& textureFilename, float scale)
{
	int childrenCount = Node->GetChildCount();

	cout << "\nName:" << Node->GetName();
	// check each child node for a FbxMesh
	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();

		// Found a mesh on this node
		if (mesh != NULL)
		{
			cout << "\nMesh:" << childNode->GetName();

			// Get index count from mesh
			int numVertices = mesh->GetControlPointsCount();
			cout << "\nVertex Count:" << numVertices;

			// Resize the vertex vector to size of this mesh
			simpleMesh.vertexList.resize(numVertices);

			//================= Process Vertices ===============
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0] * scale;
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1] * scale;
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2] * scale;
				// Generate random normal for first attempt at getting to render
				//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
			}

			int numIndices = mesh->GetPolygonVertexCount();
			cout << "\nIndice Count:" << numIndices;

			// No need to allocate int array, FBX does for us
			int* indices = mesh->GetPolygonVertices();

			// Fill indiceList
			simpleMesh.indicesList.resize(numIndices);
			memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));

			// Get the Normals array from the mesh
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			cout << "\nNormalVec Count:" << normalsVec.Size();

			//get all UV set names
			FbxStringList lUVSetNameList;
			mesh->GetUVSetNames(lUVSetNameList);
			const char* lUVSetName = lUVSetNameList.GetStringAt(0);
			const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);

			// Declare a new vector for the expanded vertex data
			// Note the size is numIndices not numVertices
			vector<SimpleVertex> vertexListExpanded;
			vertexListExpanded.resize(numIndices);

			// align (expand) vertex array and set the normals
			for (int j = 0; j < numIndices; j++)
			{
				// copy the original vertex position to the new vector
				// by using the index to look up the correct vertex
				// this is the "unindexing" step
				vertexListExpanded[j].Pos.x = simpleMesh.vertexList[indices[j]].Pos.x;
				vertexListExpanded[j].Pos.y = simpleMesh.vertexList[indices[j]].Pos.y;
				vertexListExpanded[j].Pos.z = simpleMesh.vertexList[indices[j]].Pos.z;

				vertexListExpanded[j].jIndex[0] = JOIWEI[indices[j]].jIndex[0];
				vertexListExpanded[j].jIndex[1] = JOIWEI[indices[j]].jIndex[1];
				vertexListExpanded[j].jIndex[2] = JOIWEI[indices[j]].jIndex[2];
				vertexListExpanded[j].jIndex[3] = JOIWEI[indices[j]].jIndex[3];
				vertexListExpanded[j].weights = JOIWEI[indices[j]].weights;
				// copy normal data directly, no need to unindex
				vertexListExpanded[j].Normal.x = (float)normalsVec.GetAt(j)[0];
				vertexListExpanded[j].Normal.y = (float)normalsVec.GetAt(j)[1];
				vertexListExpanded[j].Normal.z = (float)normalsVec.GetAt(j)[2];

				if (lUVElement->GetReferenceMode() == FbxLayerElement::eDirect)
				{
					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(indices[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
				else if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					auto& index_array = lUVElement->GetIndexArray();

					FbxVector2 lUVValue = lUVElement->GetDirectArray().GetAt(index_array[j]);

					vertexListExpanded[j].Tex.x = (float)lUVValue[0];
					vertexListExpanded[j].Tex.y = 1.0f - (float)lUVValue[1];
				}
			}

			// make new indices to match the new vertexListExpanded
			vector<int> indicesList;
			indicesList.resize(numIndices);
			for (int j = 0; j < numIndices; j++)
			{
				indicesList[j] = j; //literally the index is the count
			}
			
			// copy working data to the global SimpleMesh
			simpleMesh.indicesList = indicesList;
			simpleMesh.vertexList = vertexListExpanded;

			//================= Texture ========================================

			int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();
			//cout << "\nmaterial count: " << materialCount << std::endl;

			for (int index = 0; index < materialCount; index++)
			{
				FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
				//cout << "\nmaterial: " << material << std::endl;

				if (material != NULL)
				{
					//cout << "\nmaterial: " << material->GetName() << std::endl;
					// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
					FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sSpecular);

					// Check if it's layeredtextures
					int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();

					if (layeredTextureCount > 0)
					{
						for (int j = 0; j < layeredTextureCount; j++)
						{
							FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
							int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();

							for (int k = 0; k < lcount; k++)
							{
								FbxFileTexture* texture = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
								// Then, you can get all the properties of the texture, include its name
								const char* textureName = texture->GetFileName();
								//cout << textureName;
							}
						}
					}
					else
					{
						// Directly get textures
						int textureCount = prop.GetSrcObjectCount<FbxTexture>();
						for (int j = 0; j < textureCount; j++)
						{
							FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
							// Then, you can get all the properties of the texture, include its name
							const char* textureName = texture->GetFileName();
							//cout << "\nTexture Filename " << textureName;
							textureFilename = textureName;
							FbxProperty p = texture->RootProperty.Find("Filename");
							//cout << p.Get<FbxString>() << std::endl;

						}
					}

					// strip out the path and change the file extension
					textureFilename = getFileName(textureFilename);
					replaceExt(textureFilename, "dds");
					//cout << "\nTexture Filename " << textureFilename << endl;

				}
			}
		}
		// did not find a mesh here so recurse
		else
			ProcessFBXMesh2(childNode, simpleMesh, textureFilename, scale);
	}
}
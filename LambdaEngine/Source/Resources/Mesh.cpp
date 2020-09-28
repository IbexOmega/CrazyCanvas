#include "Resources/Mesh.h"

#include "Containers/TUniquePtr.h"

#include <unordered_set>

namespace LambdaEngine
{
	Mesh* MeshFactory::CreateQuad()
	{
		Vertex vertices[4] = 
		{
			{ glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
			{ glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
			{ glm::vec3( 1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
			{ glm::vec3( 1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) }
		};

		uint32 indices[6] =
		{
			2, 0, 1,
			3, 0, 2
		};

		Mesh* pMesh = DBG_NEW Mesh();
		pMesh->Vertices.Resize(ARR_SIZE(vertices));
		memcpy(pMesh->Vertices.GetData(), vertices, sizeof(Vertex) * ARR_SIZE(vertices));

		pMesh->Indices.Resize(ARR_SIZE(indices));
		memcpy(pMesh->Indices.GetData(), indices, sizeof(uint32) * ARR_SIZE(indices));

		MeshFactory::GenerateMeshlets(pMesh);
		return pMesh;
	}

	/*
	* Generation of meshlets
	* Reference: https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12MeshShaders
	*/
	struct EdgeEntry
	{
		uint32 i0;
		uint32 i1;
		uint32 i2;

		uint32 Face;
		EdgeEntry* pNext;
	};

	struct InlineMeshlet
	{
		TArray<MeshIndexType> UniqueVertexIndices;
		TArray<PackedTriangle> PrimitiveIndices;
	};

	static glm::vec3 ComputeNormal(glm::vec3 positions[3])
	{
		glm::vec3 e0 = positions[0] - positions[1];
		glm::vec3 e1 = positions[1] - positions[2];
		return glm::normalize(glm::cross(e0, e1));
	}

	static void GenerateAdjecenyList(Mesh* pMesh, uint32* pAdjecency)
	{
		const uint32 indexCount = pMesh->Indices.GetSize();
		const uint32 vertexCount = pMesh->Vertices.GetSize();
		const uint32 triangleCount = (indexCount / 3);

		const MeshIndexType* pIndices = pMesh->Indices.GetData();
		const Vertex* pVertices = pMesh->Vertices.GetData();

		TArray<MeshIndexType> indexList(vertexCount);

		std::unordered_map<size_t, MeshIndexType> uniquePositions;
		uniquePositions.reserve(vertexCount);

		std::hash<glm::vec3> hasher;
		for (uint32 i = 0; i < vertexCount; i++)
		{
			size_t hash = hasher(pVertices[i].Position);

			auto it = uniquePositions.find(hash);
			if (it != uniquePositions.end())
			{
				indexList[i] = it->second;
			}
			else
			{
				uniquePositions.insert(std::make_pair(hash, static_cast<MeshIndexType>(i)));
				indexList[i] = static_cast<MeshIndexType>(i);
			}
		}

		const uint32 hashSize = vertexCount / 3;
		TUniquePtr<EdgeEntry[]> entries(DBG_NEW EdgeEntry[triangleCount * 3]);
		TUniquePtr<EdgeEntry* []> hashTable(DBG_NEW EdgeEntry * [hashSize]);
		ZERO_MEMORY(hashTable.Get(), sizeof(EdgeEntry*) * hashSize);

		uint32 entryIndex = 0;
		for (uint32 face = 0; face < triangleCount; face++)
		{
			uint32 index = face * 3;
			for (uint32 edge = 0; edge < 3; edge++)
			{
				MeshIndexType i0 = indexList[pIndices[index + ((edge + 0) % 3)]];
				MeshIndexType i1 = indexList[pIndices[index + ((edge + 1) % 3)]];
				MeshIndexType i2 = indexList[pIndices[index + ((edge + 2) % 3)]];

				EdgeEntry& entry = entries[entryIndex++];
				entry.i0 = i0;
				entry.i1 = i1;
				entry.i2 = i2;

				uint32 key = entry.i0 % hashSize;
				entry.pNext = hashTable[key];
				entry.Face = face;

				hashTable[key] = &entry;
			}
		}

		memset(pAdjecency, static_cast<uint32>(-1), indexCount * sizeof(uint32));

		for (uint32 face = 0; face < triangleCount; face++)
		{
			uint32 index = face * 3;
			for (uint32 point = 0; point < 3; point++)
			{
				if (pAdjecency[index + point] != static_cast<uint32>(-1))
				{
					continue;
				}

				MeshIndexType i0 = indexList[pIndices[index + ((point + 1) % 3)]];
				MeshIndexType i1 = indexList[pIndices[index + ((point + 0) % 3)]];
				MeshIndexType i2 = indexList[pIndices[index + ((point + 2) % 3)]];

				uint32 key = i0 % hashSize;

				EdgeEntry* pFound = nullptr;
				EdgeEntry* pFoundPrev = nullptr;

				{
					EdgeEntry* pPrev = nullptr;
					for (EdgeEntry* pCurrent = hashTable[key]; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
					{
						if (pCurrent->i1 == i1 && pCurrent->i0 == i0)
						{
							pFound = pCurrent;
							pFoundPrev = pPrev;
							break;
						}
					}
				}

				glm::vec3 n0;
				{
					glm::vec3 p0 = pVertices[i0].Position;
					glm::vec3 p1 = pVertices[i1].Position;
					glm::vec3 p2 = pVertices[i2].Position;
					glm::vec3 e0 = p0 - p1;
					glm::vec3 e1 = p1 - p2;
					n0 = glm::normalize(glm::cross(e0, e1));
				}

				float32 bestDot = -2.0f;
				{
					EdgeEntry* pPrev = pFoundPrev;
					for (EdgeEntry* pCurrent = pFound; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
					{
						glm::vec3 p0 = pVertices[pCurrent->i0].Position;
						glm::vec3 p1 = pVertices[pCurrent->i1].Position;
						glm::vec3 p2 = pVertices[pCurrent->i2].Position;
						glm::vec3 e0 = p0 - p1;
						glm::vec3 e1 = p1 - p2;
						glm::vec3 n1 = glm::normalize(glm::cross(e0, e1));

						float32 dot = glm::dot(n0, n1);
						if (dot > bestDot)
						{
							pFound = pCurrent;
							pFoundPrev = pPrev;
							bestDot = dot;
						}
					}
				}

				if (pFound)
				{
					if (pFound->Face != static_cast<uint32>(-1))
					{
						if (pFoundPrev != nullptr)
						{
							pFoundPrev->pNext = pFound->pNext;
						}
						else
						{
							hashTable[key] = pFound->pNext;
						}
					}

					pAdjecency[index + point] = pFound->Face;

					uint32 key2 = i1 % hashSize;
					{
						EdgeEntry* pPrev = nullptr;
						for (EdgeEntry* pCurrent = hashTable[key2]; pCurrent != nullptr; pPrev = pCurrent, pCurrent = pCurrent->pNext)
						{
							if (pCurrent->Face == face && pCurrent->i0 == i1 && pCurrent->i1 == i0)
							{
								if (pPrev != nullptr)
								{
									pPrev->pNext = pCurrent->pNext;
								}
								else
								{
									hashTable[key2] = pCurrent->pNext;
								}

								break;
							}
						}
					}

					bool linked = false;
					for (uint32 point2 = 0; point2 < point; point2++)
					{
						if (pFound->Face == pAdjecency[index + point2])
						{
							linked = true;
							pAdjecency[index + point] = static_cast<uint32>(-1);
							break;
						}
					}

					if (!linked)
					{
						uint32 edge2 = 0;
						for (; edge2 < 3; edge2++)
						{
							MeshIndexType k = pIndices[(pFound->Face * 3) + edge2];
							if (k == static_cast<uint32>(-1))
							{
								continue;
							}

							if (indexList[k] == i0)
							{
								break;
							}
						}

						if (edge2 < 3)
						{
							pAdjecency[(pFound->Face * 3) + edge2] = face;
						}
					}
				}
			}
		}
	}

	static bool AddToMeshlet(uint32 maxVerts, uint32 maxPrims, InlineMeshlet& meshlet, uint32(&tri)[3])
	{
		if (meshlet.UniqueVertexIndices.GetSize() == maxVerts)
		{
			return false;
		}

		if (meshlet.PrimitiveIndices.GetSize() == maxPrims)
		{
			return false;
		}

		static const uint32 undef = static_cast<uint32>(-1);
		uint32 indices[3] = { undef, undef, undef };
		uint32 newCount = 3;

		for (uint32 i = 0; i < meshlet.UniqueVertexIndices.GetSize(); i++)
		{
			for (uint32 j = 0; j < 3; j++)
			{
				if (meshlet.UniqueVertexIndices[i] == tri[j])
				{
					indices[j] = i;
					--newCount;
				}
			}
		}

		if (meshlet.UniqueVertexIndices.GetSize() + newCount > maxVerts)
		{
			return false;
		}

		for (uint32 i = 0; i < 3; i++)
		{
			if (indices[i] == undef)
			{
				indices[i] = static_cast<uint32>(meshlet.UniqueVertexIndices.GetSize());
				meshlet.UniqueVertexIndices.PushBack(tri[i]);
			}
		}

		PackedTriangle prim = { };
		prim.i0 = indices[0];
		prim.i1 = indices[1];
		prim.i2 = indices[2];
		meshlet.PrimitiveIndices.PushBack(prim);

		return true;
	}

	static bool IsMeshletFull(uint32_t maxVerts, uint32_t maxPrims, const InlineMeshlet& meshlet)
	{
		VALIDATE(meshlet.UniqueVertexIndices.GetSize() <= maxVerts);
		VALIDATE(meshlet.PrimitiveIndices.GetSize() <= maxPrims);

		return meshlet.UniqueVertexIndices.GetSize() == maxVerts || meshlet.PrimitiveIndices.GetSize() == maxPrims;
	}

	static uint32 ComputeMeshletReuse(InlineMeshlet& meshlet, uint32(&triIndices)[3])
	{
		uint32 count = 0;
		for (uint32 i = 0; i < static_cast<uint32_t>(meshlet.UniqueVertexIndices.GetSize()); i++)
		{
			for (uint32 j = 0; j < 3; j++)
			{
				if (meshlet.UniqueVertexIndices[i] == triIndices[j])
				{
					count++;
				}
			}
		}

		return count;
	}

	static float32 ComputeMeshletScore(InlineMeshlet& meshlet, glm::vec3 normal, uint32(&triIndices)[3], glm::vec3* pTriVerts)
	{
		const float32 reuseWeight = 0.5f;
		const float32 oriWeight = 0.5f;

		const uint32 reuse = ComputeMeshletReuse(meshlet, triIndices);
		float32 reuseScore = float32(reuse) / 3.0f;

		glm::vec3 n = ComputeNormal(pTriVerts);
		float32 dot = glm::dot(n, normal);
		float32 oriScore = (-dot + 1.0f) / 2.0f;

		return (reuseWeight * reuseScore) + (oriWeight * oriScore);
	}

	static void Meshletize(Mesh* pMesh, uint32 maxVerts, uint32 maxPrims, TArray<InlineMeshlet>& output)
	{
		MeshIndexType* pIndices = pMesh->Indices.GetData();
		Vertex* pVertices = pMesh->Vertices.GetData();

		const uint32 vertexCount = pMesh->Vertices.GetSize();
		const uint32 indexCount = pMesh->Indices.GetSize();
		const uint32 triangleCount = (indexCount / 3);

		TArray<uint32> adjecenyList(indexCount);
		GenerateAdjecenyList(pMesh, adjecenyList.GetData());
		adjecenyList.ShrinkToFit();

		output.Clear();
		output.EmplaceBack();
		InlineMeshlet* pCurr = &output.GetBack();

		// Using std::vector since it has a bool specialization that stores it in a bitrepresentation
		std::vector<bool> checklist;
		checklist.resize(triangleCount);

		TArray<glm::vec3> positions;
		TArray<glm::vec3> normals;
		TArray<std::pair<uint32, float32>> candidates;
		std::unordered_set<uint32> candidateCheck;
		glm::vec3 normal;

		uint32 triIndex = 0;
		candidates.EmplaceBack(std::make_pair(triIndex, 0.0f));
		candidateCheck.insert(triIndex);

		while (!candidates.IsEmpty())
		{
			uint32 index = candidates.GetBack().first;
			candidates.PopBack();

			MeshIndexType tri[3] =
			{
				pIndices[index * 3 + 0],
				pIndices[index * 3 + 1],
				pIndices[index * 3 + 2],
			};

			VALIDATE(tri[0] < vertexCount);
			VALIDATE(tri[1] < vertexCount);
			VALIDATE(tri[2] < vertexCount);

			if (AddToMeshlet(maxVerts, maxPrims, *pCurr, tri))
			{
				checklist[index] = true;

				glm::vec3 points[3] =
				{
					pVertices[tri[0]].Position,
					pVertices[tri[1]].Position,
					pVertices[tri[2]].Position,
				};

				positions.PushBack(points[0]);
				positions.PushBack(points[1]);
				positions.PushBack(points[2]);

				normal = ComputeNormal(points);
				normals.PushBack(normal);

				const uint32 adjIndex = index * 3;
				uint32 adj[3] =
				{
					adjecenyList[adjIndex + 0],
					adjecenyList[adjIndex + 1],
					adjecenyList[adjIndex + 2],
				};

				for (uint32 i = 0; i < 3; i++)
				{
					if (adj[i] == static_cast<uint32>(-1))
					{
						continue;
					}

					if (checklist[adj[i]])
					{
						continue;
					}

					if (candidateCheck.count(adj[i]))
					{
						continue;
					}

					candidates.PushBack(std::make_pair(adj[i], FLT_MAX));
					candidateCheck.insert(adj[i]);
				}

				for (uint32 i = 0; i < static_cast<uint32>(candidates.GetSize()); i++)
				{
					uint32 candidate = candidates[i].first;
					MeshIndexType triIndices[3] =
					{
						pIndices[(candidate * 3) + 0],
						pIndices[(candidate * 3) + 1],
						pIndices[(candidate * 3) + 2],
					};

					VALIDATE(triIndices[0] < vertexCount);
					VALIDATE(triIndices[1] < vertexCount);
					VALIDATE(triIndices[2] < vertexCount);

					glm::vec3 triVerts[3] =
					{
						pVertices[triIndices[0]].Position,
						pVertices[triIndices[1]].Position,
						pVertices[triIndices[2]].Position,
					};

					candidates[i].second = ComputeMeshletScore(*pCurr, normal, triIndices, triVerts);
				}

				if (IsMeshletFull(maxVerts, maxPrims, *pCurr))
				{
					positions.Clear();
					normals.Clear();

					candidateCheck.clear();

					if (!candidates.IsEmpty())
					{
						candidates[0] = candidates.GetBack();
						candidates.Resize(1);
						candidateCheck.insert(candidates[0].first);
					}

					output.EmplaceBack();
					pCurr = &output.GetBack();
				}
				else
				{
					std::sort(candidates.begin(), candidates.end(), [](const std::pair<uint32, float32>& a, const std::pair<uint32, float32>& b)
						{
							return a.second > b.second;
						});
				}
			}
			else
			{
				if (candidates.IsEmpty())
				{
					positions.Clear();
					normals.Clear();

					candidateCheck.clear();

					output.EmplaceBack();
					pCurr = &output.GetBack();
				}
			}

			if (candidates.IsEmpty())
			{
				while (triIndex < triangleCount && checklist[triIndex])
				{
					triIndex++;
				}

				if (triIndex == triangleCount)
				{
					break;
				}

				candidates.PushBack(std::make_pair(triIndex, 0.0f));
				candidateCheck.insert(triIndex);
			}
		}

		if (output.GetBack().PrimitiveIndices.IsEmpty())
		{
			output.PopBack();
		}
		else
		{
			output.GetBack().PrimitiveIndices.ShrinkToFit();
			output.GetBack().UniqueVertexIndices.ShrinkToFit();
		}
	}

	void MeshFactory::GenerateMeshlets(Mesh* pMesh, uint32 maxVerts, uint32 maxPrims)
	{
		TArray<InlineMeshlet> builtMeshlets;
		Meshletize(pMesh, maxVerts, maxPrims, builtMeshlets);

		uint32 uniqueVertexIndexCount	= 0;
		uint32 primitiveIndexCount		= 0;
		const uint32 meshletCount		= static_cast<uint32>(builtMeshlets.GetSize());

		pMesh->Meshlets.Resize(meshletCount);
		for (uint32 i = 0; i < meshletCount; i++)
		{
			pMesh->Meshlets[i].VertOffset	= uniqueVertexIndexCount;
			pMesh->Meshlets[i].VertCount	= static_cast<uint32>(builtMeshlets[i].UniqueVertexIndices.GetSize());
			uniqueVertexIndexCount += static_cast<uint32>(builtMeshlets[i].UniqueVertexIndices.GetSize());

			pMesh->Meshlets[i].PrimOffset	= primitiveIndexCount;
			pMesh->Meshlets[i].PrimCount	= static_cast<uint32>(builtMeshlets[i].PrimitiveIndices.GetSize());
			primitiveIndexCount += static_cast<uint32>(builtMeshlets[i].PrimitiveIndices.GetSize());
		}

		pMesh->PrimitiveIndices.Resize(primitiveIndexCount);
		pMesh->UniqueIndices.Resize(uniqueVertexIndexCount);

		MeshIndexType*	pUniqueIndices		= pMesh->UniqueIndices.GetData();
		PackedTriangle*	pPrimitiveIndices	= pMesh->PrimitiveIndices.GetData();
		for (uint32 i = 0; i < meshletCount; i++)
		{
			uint32 localPrimitiveIndexCount		= builtMeshlets[i].PrimitiveIndices.GetSize();
			uint32 localUniqueVertexIndexCount	= builtMeshlets[i].UniqueVertexIndices.GetSize();

			memcpy(pPrimitiveIndices, builtMeshlets[i].PrimitiveIndices.GetData(), sizeof(PackedTriangle) * localPrimitiveIndexCount);
			memcpy(pUniqueIndices, builtMeshlets[i].UniqueVertexIndices.GetData(), sizeof(MeshIndexType) * localUniqueVertexIndexCount);

			pPrimitiveIndices	+= localPrimitiveIndexCount;
			pUniqueIndices		+= localUniqueVertexIndexCount;
		}

		//LOG_INFO("--------------------------------------------");
		//LOG_INFO("UniqueIndexCount=%u", pMesh->UniqueIndices.GetSize());
		//for (uint32 index = 0; index < pMesh->UniqueIndices.GetSize(); index++)
		//{
		//	LOG_INFO("[%u]=%u", index, pMesh->UniqueIndices[index]);
		//}

		//LOG_INFO("IndexCount=%u", pMesh->Indices.GetSize());
		//for (uint32 index = 0; index < pMesh->Indices.GetSize(); index++)
		//{
		//	LOG_INFO("[%u]=%u", index, pMesh->Indices[index]);
		//}

		//LOG_INFO("PrimitiveIndexCount=%d", pMesh->PrimitiveIndices.GetSize());
		//for (uint32 index = 0; index < pMesh->PrimitiveIndices.GetSize(); index++)
		//{
		//	uint32 i0 = pMesh->PrimitiveIndices[index].i0;
		//	uint32 i1 = pMesh->PrimitiveIndices[index].i1;
		//	uint32 i2 = pMesh->PrimitiveIndices[index].i2;

		//	LOG_INFO("[%u]=[%u, %u, %u]", index, i0, i1, i2);
		//}

		//LOG_INFO("VertexCount=%u", pMesh->Vertices.GetSize());
		//LOG_INFO("MeshletCount=%u", pMesh->Meshlets.GetSize());
		//for (uint32 meshlet = 0; meshlet < pMesh->Meshlets.GetSize(); meshlet++)
		//{
		//	Meshlet& m = pMesh->Meshlets[meshlet];
			//LOG_INFO("Meshlet[%u]", meshlet);
			//LOG_INFO("PrimCount=%u", m.PrimCount);
			//LOG_INFO("PrimOffset=%u", m.PrimOffset);
			//LOG_INFO("VertCount=%u", m.VertCount);
			//LOG_INFO("VertOffset=%u", m.VertOffset);
			
		//	LOG_INFO("Primitive Indices: ", meshlet);
		//	for (uint32 prim = 0; prim < m.PrimCount; prim++)
		//	{
		//		uint32 i0 = pMesh->PrimitiveIndices[prim + m.PrimOffset].i0;
		//		uint32 i1 = pMesh->PrimitiveIndices[prim + m.PrimOffset].i1;
		//		uint32 i2 = pMesh->PrimitiveIndices[prim + m.PrimOffset].i2;
		//		LOG_INFO("[%u]=[%u, %u, %u]", prim, i0, i1, i2);
		//	}
		//}
		//LOG_INFO("--------------------------------------------");
	}
}

#include "Scatter.h"

namespace VVVV {
namespace Nodes {

	void ScatterNode::Evaluate(int SpreadMax)
	{
		if (FInputGeo->IsConnected)
		{
			this->RenderRequest(this, this->FHost);

			FInputGeo->Sync();
			this->Invalidate = (FInputGeo->Stream->IsChanged || FCount->IsChanged || FSeed->IsChanged);
		}
		
		if (this->Invalidate)
		{
			if (FInputGeo->Stream->IsChanged)
			{
				if (this->AssignedContext == nullptr)
				{
					((IOutStream^)FOutput)->Length = 0;
					return;
				}

				setUpVertex();
			}

			size_t Count = ((ISpread<int>^)FCount)[0];
			size_t Seed = ((ISpread<int>^)FSeed)[0];

			std::mt19937 mt(Seed);
			std::uniform_real_distribution<float> rand01{0.0f, 1.0f};

			((IOutStream^)FOutput)->Length = Count;
			auto _o = FOutput->GetWriter();
			_o->Reset();

			for (size_t i = 0; i < Count; ++i)
			{
				float areaRandom = areaSum * rand01(mt);

				auto ite = std::upper_bound(areas->begin(), areas->end(), areaRandom);

				size_t primNum = std::distance(areas->begin(), ite);
				primNum = std::min(primNum, areas->size() - 1);

				const Vector3& p0 = positions[indices[(primNum * 3) + 0]];
				const Vector3& p1 = positions[indices[(primNum * 3) + 1]];
				const Vector3& p2 = positions[indices[(primNum * 3) + 2]];

				Vector3D& p = randomPoint(p0, p1, p2, mt);

				_o->Write(p, 1);
			}

			this->Invalidate = false;
		}
	}

	void ScatterNode::setUpVertex()
	{
		SlimDX::Direct3D11::Device^ device = this->AssignedContext->Device;
		SlimDX::Direct3D11::DeviceContext^ deviceContext = this->AssignedContext->CurrentDeviceContext;

		delete indices;
		delete positions;
		delete areas;
		this->areaSum = 0;

		positions = gcnew cliext::vector<Vector3>();
		areas = new std::vector<float>();

		IDX11Geometry^ geo = FInputGeo[0][this->AssignedContext]->ShallowCopy();

		if (geo->GetType() == DX11IndexedGeometry::typeid)
		{
			DX11IndexedGeometry^ Igeo = (DX11IndexedGeometry^)geo;

			DX11IndexBuffer^ indexBuffer = Igeo->IndexBuffer;
			SlimDX::Direct3D11::Buffer^ iBuffer = indexBuffer->Buffer;
			SlimDX::Direct3D11::Buffer^ vBuffer = Igeo->VertexBuffer;

			const size_t iCount = indexBuffer->IndicesCount;
			const size_t vCount = Igeo->VerticesCount;
			const size_t vSize = Igeo->VertexSize;
			
			indices = gcnew Spread<int>(iCount);
			positions->reserve(vCount);

			auto indexStaging = gcnew DX11StagingStructuredBuffer(device, iCount, sizeof(int));
			auto vertexStaging = gcnew DX11StagingStructuredBuffer(device, vCount, vSize);
			
			deviceContext->CopyResource(iBuffer, indexStaging->Buffer);
			deviceContext->CopyResource(vBuffer, vertexStaging->Buffer);

			DataStream^ ids = indexStaging->MapForRead(deviceContext);
			DataStream^ vds = vertexStaging->MapForRead(deviceContext);

			try
			{
				ids->ReadRange<int>(indices->Stream->Buffer, 0, iCount);
			}
			catch (Exception ^ e)
			{
				FLogger->Log(LogType::Error, "Error in readback index: " + e->Message);
			}
			finally
			{
				indexStaging->UnMap(deviceContext);
				delete indexStaging;
			}

			try
			{
				for (size_t i = 0; i < vCount; ++i)
				{
					positions->push_back(vds->Read<Vector3>());
					vds->Seek(vSize - sizeof(Vector3), System::IO::SeekOrigin::Current);
				}
			}
			catch (Exception ^ e)
			{
				FLogger->Log(LogType::Error, "Error in readback vertex: " + e->Message);
			}
			finally
			{
				vertexStaging->UnMap(deviceContext);
				delete vertexStaging;
			}

			const size_t nPrims = iCount / 3;

			float area_sum = 0;

			for (size_t i = 0; i < nPrims; ++i)
			{
				const Vector3& p0 = positions[indices[i + 0]];
				const Vector3& p1 = positions[indices[i + 1]];
				const Vector3& p2 = positions[indices[i + 2]];

				float area = (Vector3::Cross(Vector3::Subtract(p0,p1), Vector3::Subtract(p0,p2))).Length() / 2;

				area_sum += area;
				areas->push_back(area_sum);
			}

			this->areaSum = area_sum;

			FLogger->Log(LogType::Debug, "areas size : " + areas->size().ToString());
			FLogger->Log(LogType::Debug, "positions size : " + positions->size().ToString());
			FLogger->Log(LogType::Debug, "indices size : " + indices->SliceCount.ToString());

		}
		else if (geo->GetType() == DX11VertexGeometry::typeid)
		{
			DX11VertexGeometry^ Vgeo = (DX11VertexGeometry^)geo;
		}
		else
		{
			FLogger->Log(LogType::Debug, "Invalid Type.");
			((IOutStream^)FOutput)->Length = 0;
		}
	}

	Vector3D ScatterNode::randomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, std::mt19937& mt)
	{
		std::uniform_real_distribution<float> rand01{ 0.0f, 1.0f };

		const float u = rand01(mt);
		const float v = rand01(mt);

		const float m1 = std::min(u, v);
		const float m2 = 1.0f - std::max(u, v);
		const float m3 = std::max(u, v) - std::min(u, v);

		const float x = m1*p0.X + m2*p1.X + m3*p2.X;
		const float y = m1*p0.Y + m2*p1.Y + m3*p2.Y;
		const float z = m1*p0.Z + m2*p1.Z + m3*p2.Z;

		return Vector3D(x, y, z);
	}


}
}
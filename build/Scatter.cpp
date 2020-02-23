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

		SlimDX::Direct3D11::Device^ device = this->AssignedContext->Device;
		SlimDX::Direct3D11::DeviceContext^ deviceContext = this->AssignedContext->CurrentDeviceContext;

		size_t Count = ((ISpread<int>^)FCount)[0];
		size_t Seed = ((ISpread<int>^)FSeed)[0];

		for (int i = 0; i < SpreadMax; i++)
			FOutput[i] = Vector3D(Count * 2);
		
		if (this->Invalidate)
		{
			if (this->AssignedContext == nullptr) return;

			FLogger->Log(LogType::Debug, "context exist");

			IDX11Geometry^ geo = FInputGeo[0][this->AssignedContext]->ShallowCopy();

			if (geo->GetType() == DX11IndexedGeometry::typeid)
			{
				delete indices;
				delete positions;

				DX11IndexedGeometry^ Igeo = (DX11IndexedGeometry^)geo;

				DX11IndexBuffer^ indexBuffer = Igeo->IndexBuffer;
				SlimDX::Direct3D11::Buffer^ iBuffer = indexBuffer->Buffer;
				SlimDX::Direct3D11::Buffer^ vBuffer = Igeo->VertexBuffer;

				size_t iCount = indexBuffer->IndicesCount;
				size_t vCount = Igeo->VerticesCount;
				size_t vSize = Igeo->VertexSize;

				auto indexStaging = gcnew DX11StagingStructuredBuffer(device, iCount, sizeof(int));
				indices = gcnew Spread<int>(iCount);

				auto vertexStaging = gcnew DX11StagingStructuredBuffer(device, vCount, vSize);
				positions = new std::vector<Vector3D>(vCount);

				deviceContext->CopyResource(iBuffer, indexStaging->Buffer);
				deviceContext->CopyResource(vBuffer, vertexStaging->Buffer);

				DataStream^ ids = indexStaging->MapForRead(deviceContext);
				DataStream^ vds = vertexStaging->MapForRead(deviceContext);

				try 
				{
					ids->ReadRange<int>(indices->Stream->Buffer, 0, iCount);
				}
				catch (Exception^ e) 
				{
					FLogger->Log(LogType::Error, "Error in readback index: " + e->Message);
				}
				finally 
				{
					indexStaging->UnMap(deviceContext);
					indexStaging->Dispose();
				}

				try
				{
					for (size_t i = 0; i < vCount; ++i)
					{
						positions->push_back(vds->Read<Vector3>());
						vds->Seek(vSize - sizeof(Vector3), System::IO::SeekOrigin::Current);
					}
				}
				catch (Exception^ e)
				{
					FLogger->Log(LogType::Error, "Error in readback vertex: " + e->Message);
				}
				finally
				{
					vertexStaging->UnMap(deviceContext);
					vertexStaging->Dispose();
				}
			}
			else if (geo->GetType() == DX11VertexGeometry::typeid)
			{
				DX11VertexGeometry^ Vgeo = (DX11VertexGeometry^)geo;
			}
			else
			{
				FLogger->Log(LogType::Debug, "Invalid Type.");
				FOutput->SliceCount = 0;
			}

			this->Invalidate = false;
		}
	}

}
}
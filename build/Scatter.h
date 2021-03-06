﻿#pragma once

#include <typeinfo>
#include <cliext\vector>
#include <vector>
#include <random>
#include <cmath>

using namespace System;
using namespace System::ComponentModel::Composition;

using namespace VVVV::PluginInterfaces::V1;
using namespace VVVV::PluginInterfaces::V2;

using namespace VVVV::Utils::VMath;

using namespace VVVV::DX11;

using namespace SlimDX;
using namespace SlimDX::Direct3D11;

using namespace FeralTic::DX11;
using namespace FeralTic::DX11::Resources;
using namespace FeralTic::DX11::Geometry;

using namespace VVVV::Utils::Streams;

using namespace VVVV::Core::Logging;

using namespace System;

namespace VVVV
{
namespace Nodes
{

	[PluginInfo(Name = "Scatter", Category = "3d", Version = "", Tags = "")]
	public ref class ScatterNode : public IPluginEvaluate, IDX11ResourceDataRetriever
	{
	public:

		[Input("Count", DefaultValue = 200, MinValue = 1, Order = 2, IsSingle = true)]
		IDiffSpread<int>^ FCount;

		[Input("Seed", DefaultValue = 0, MinValue = 0, Order = 3, IsSingle = true)]
		IDiffSpread<int>^ FSeed;

		[Output("Output")]
		IOutStream<Vector3D>^ FOutput;

		[Import()]
		ILogger^ FLogger;

		virtual property DX11RenderContext^ AssignedContext;

		virtual event DX11RenderRequestDelegate^ RenderRequest;

		void Evaluate(int SpreadMax) override;

	protected:
		[Input("Geometry In", AutoValidate = false, CheckIfChanged = true, Order = 1, IsSingle = true)]
		Pin<DX11Resource<IDX11Geometry^>^>^ FInputGeo;

		[Import()]
		IPluginHost^ FHost;

	private:

		bool Invalidate = false;

		cliext::vector<Vector3>^ positions;
		ISpread<int>^ indices;

		float areaSum = 0;
		std::vector<float>* areas;

		void setUpVertex();

		Vector3D randomPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, std::mt19937& mt);
	};

}
}
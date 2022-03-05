#pragma once
#pragma once
#include "math_types.h"
#include <directxmath.h>
namespace end {
	struct positions
	{
		float3 LTF;
		float3 RTF;
		float3 LTB;
		float3 RTB;
		float3 LBF;
		float3 RBF;
		float3 LBB;
		float3 RBB;
	};
	struct AABB
	{
		float3 center;
		float3 extents;
	};
	struct AABBDetails
	{
		float3 pos;
		float width;
		float height;
		float depth;
	};
	struct MM
	{
		float3 min;
		float3 max;
	};
	class PersonalMath
	{
	private:
		float colorvalue = 0;
		float color[3];
		int colorIndex = 0;
		bool colorInc = true;

	public:
		float clamp(float value);
		double clamp(double value);
		float RandomFloat(int min, int max);
		float4 cycleColor(double delta_time);
		float4x4 Identity();
		float4x4 LookAt(float3 pos, float3 target, float3 up);
		float4 ConToF4(float3 X);
		float3 FindNextPos(float3 Curr, float3 dir, float scale);
		float4x4 TurnTo(float4x4 our, float4x4 target, float speed);
		float4x4 Orthonormalize(float4x4 mat);
		float degreeToRadian(float degree);
		float RadianToDegree(float radian);
		float4x4 RotationY(float degree);
		float4x4 RotationX(float degree);
		float4x4 MultiplyMat(float4x4 mat1, float4x4 mat2);
		positions AllPos(float3 centre, float height, float width, float depth);
		float3 FindingCenterPoint(float3 a, float3 b, float3 c, float3 d, float3 & second, float3 normal);
		AABBDetails converToAABB(float3 a, float3 b, float3 c);
		AABB createAABB(float3 centerA, float3 extentA, float3 centerB, float3 extentB);
		float4x4 Transpose(float4x4 mat);
		DirectX::XMFLOAT3 ConverttoXM(float3 data);
		DirectX::XMFLOAT4 ConverttoXM(float4 data);
		float3 ConverttoF(DirectX::XMFLOAT3 data);
		float4 ConverttoF(DirectX::XMFLOAT4 data);
	};

}
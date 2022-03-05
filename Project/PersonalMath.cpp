#include "PersonalMath.h"
#include<iostream>
#include "math_types.h"



namespace end {

	float PersonalMath::clamp(float value)
	{
		if (value > 1.0f)
		{
			value = 1;
		}
		else if (value < 0)
		{
			value = 0;
		}
		return value;
	}
	double PersonalMath::clamp(double value)
	{
		if (value > 1)
		{
			value = 1;
		}
		else if (value < 0)
		{
			value = 0;
		}
		return value;
	}
	float PersonalMath::RandomFloat(int min, int max)
	{
		float out = 0;
		int range = max - min;
		float random = (float)rand() / float(RAND_MAX);
		out = random * range;
		out += min;
		return out;
	}
	DirectX::XMFLOAT3 PersonalMath::ConverttoXM(float3 data)
	{
		DirectX::XMFLOAT3 ans;
		ans.x = data.x;
		ans.y = data.y;
		ans.z = data.z;
		return ans;
	}
	DirectX::XMFLOAT4 PersonalMath::ConverttoXM(float4 data)
	{
		DirectX::XMFLOAT4 ans;
		ans.x = data.x;
		ans.y = data.y;
		ans.z = data.z;
		ans.w = data.w;
		return ans;
	}
	float3 PersonalMath::ConverttoF(DirectX::XMFLOAT3 data)
	{
		float3 ans;
		ans.x = data.x;
		ans.y = data.y;
		ans.z = data.z;
		
		return ans;
	}
	float4 PersonalMath::ConverttoF(DirectX::XMFLOAT4 data)
	{
		float4 ans;
		ans.x = data.x;
		ans.y = data.y;
		ans.z = data.z;
		ans.w = data.w;
		return ans;
	}



	float4 PersonalMath::cycleColor(double delta_time)
	{
		if (colorInc)
		{
			if (colorvalue > 1)
			{
				colorvalue = 0;
				colorIndex++;
			}
			if (colorIndex > 2)
			{
				colorIndex = 2;
				colorvalue = 1;
				colorInc = false;
				return float4(color[0], color[1], color[2], 1);
			}
			colorvalue += delta_time;
			color[colorIndex] = colorvalue;
			return float4(color[0], color[1], color[2], 1);
		}
		else
		{
			if (colorvalue < 0)
			{
				colorvalue = 1;
				colorIndex--;
			}
			if (colorIndex < 0)
			{
				colorIndex = 0;
				colorvalue = 0;
				colorInc = true;
				return float4(color[0], color[1], color[2], 1);
			}

			colorvalue -= delta_time;
			color[colorIndex] = colorvalue;
			return float4(color[0], color[1], color[2], 1);
		}
	}

	float4x4 PersonalMath::Identity()
	{
		float4x4 identity = { float4 {1,0,0,0},
							 float4 {0,1,0,0},
							 float4 {0,0,1,0},
							 float4 {0,0,0,1}
		};
		return identity;
	}
	float3 PersonalMath::FindingCenterPoint(float3 a, float3 b, float3 c, float3 d, float3 & second, float3 normal)
	{
		float3 ans = (a + b + c + d) / 4.0f;
		second = ans + (normal * 1);
		return ans;
		
	}
	float4x4 PersonalMath::LookAt(float3 pos, float3 target, float3 up)
	{
		
		float3 Z = target - pos;
		Z = normalize(Z);
		float3 X = cross(up, Z);
		X = normalize(X);
		float3 Y = cross(Z, X);
		Y = normalize(Y);

		
		float4x4 ans = { ConToF4(X),
							 ConToF4(Y),
							 ConToF4(Z),
							 float4 {pos.x,pos.y,pos.z,1} };

		return ans;
	}

	float4x4 PersonalMath::Transpose(float4x4 mat)
	{
		float4x4 out;
		out[0].x = mat[0].x;
		out[1].x = mat[0].y;
		out[2].x = mat[0].z;
		out[3].x = mat[0].w;
		out[0].y = mat[1].x;
		out[1].y = mat[1].y;
		out[2].y = mat[1].z;
		out[3].y = mat[1].w;
		out[0].z = mat[2].x;
		out[1].z = mat[2].y;
		out[2].z = mat[2].z;
		out[3].z = mat[2].w;
		out[0].w = mat[3].x;
		out[1].w = mat[3].y;
		out[2].w = mat[3].z;
		out[3].w = mat[3].w;
		return out;
		
	}		   
	AABBDetails PersonalMath::converToAABB(float3 a, float3 b, float3 c)
	{
		AABBDetails ans;
		float3 min;
		min.x = std::min(a.x, std::min(b.x, c.x));
		min.y = std::min(a.y, std::min(b.y, c.y));
		min.z = std::min(a.z, std::min(b.z, c.z));

		float3 max;
		max.x = std::max(a.x, std::max(b.x, c.x));
		max.y = std::max(a.y, std::max(b.y, c.y));
		max.z = std::max(a.z, std::max(b.z, c.z));

		ans.pos = (max + min) / 2;
		ans.width = max.x - min.x;
		if (ans.width == 0.0f)
		{
			ans.width = 0.1f;
		}
		ans.height = max.y - min.y;
		if (ans.height == 0.0f)
		{
			ans.height = 0.1f;
		}
		ans.depth = max.z - min.z;
		if (ans.depth == 0.0f)
		{
			ans.depth = 0.1f;
		}
		return ans;
	}

	float4 PersonalMath::ConToF4(float3 X)
	{
		return float4(X.x, X.y, X.z, 0);
	}

	float3 PersonalMath::FindNextPos(float3 Curr,float3 dir,float scale)
	{
		dir = normalize(dir);
		return Curr + (dir * scale);
	}

	float PersonalMath::degreeToRadian(float degree)
	{
		float radian = degree * 3.14 / 180;
		return radian;
	}
	float PersonalMath::RadianToDegree(float radian)
	{
		return radian * (180/(22/7));
	}

	float4x4 PersonalMath::RotationY(float degree)
	{
		float Radian = degreeToRadian(degree);
		float Degree = RadianToDegree(degree);
		float4x4 RotatedMat = { float4{cosf(Radian),0,-sinf(Radian),0},
								float4{0,1,0,0},
								float4{sinf(Radian),0,cosf(Radian),0},
								float4 {0,0,0,1} };
		return RotatedMat;
	}

	positions PersonalMath::AllPos(float3 centre, float height, float width, float depth)
	{
		positions ans;
		float newWidth = width / 2;
		float newHeight = height / 2;
		float newDepth = depth / 2;
		ans.LTF = float3(centre.x - newWidth,centre.y + newHeight,centre.z - newDepth);
		ans.RTF = float3(centre.x + newWidth, centre.y + newHeight, centre.z - newDepth);
		ans.LTB = float3(centre.x - newWidth, centre.y + newHeight, centre.z + newDepth);
		ans.RTB = float3(centre.x + newWidth, centre.y + newHeight, centre.z + newDepth);
		ans.LBF = float3(centre.x - newWidth, centre.y - newHeight, centre.z - newDepth);
		ans.RBF = float3(centre.x + newWidth, centre.y - newHeight, centre.z - newDepth);
		ans.LBB = float3(centre.x - newWidth, centre.y - newHeight, centre.z + newDepth);
		ans.RBB = float3(centre.x + newWidth, centre.y - newHeight, centre.z + newDepth);
		return ans;
	}

	AABB PersonalMath::createAABB(float3 centerA, float3 extentA, float3 centerB, float3 extentB)
	{
		float3 minA = centerA - extentA;
		float3 minB = centerB - extentB;
		float3 maxA = centerA + extentA;
		float3 maxB = centerB + extentB;
		float3 min;
		float3 max;
		min.x = std::min(minA.x, minB.x);
		min.y = std::min(minA.y, minB.y);
		min.z = std::min(minA.z, minB.z);

		max.x = std::max(maxA.x, maxB.x);
		max.y = std::max(maxA.y, maxB.y);
		max.z = std::max(maxA.z, maxB.z);

		AABB ans;
		ans.center = (max + min) / 2;
		ans.extents =(max - min) / 2;
		
		return ans;
	}

	float4x4 PersonalMath::RotationX(float degree)
	{
		float Radian = degreeToRadian(degree);
		float Degree = RadianToDegree(degree);
		float4x4 RotatedMat = { float4{1,0,0,0},
								float4{0,cosf(Radian),sinf(Radian),0},
								float4{0,-sinf(Radian),cosf(Radian),0},
								float4 {0,0,0,1} };
		return RotatedMat;
	}

	float4x4 PersonalMath::MultiplyMat(float4x4 mat1, float4x4 mat2)
	{
		float4x4 ans;
		ans[0].x = mat1[0].x * mat2[0].x + mat1[0].y * mat2[1].x + mat1[0].z * mat2[2].x + mat1[0].w * mat2[3].x;
		ans[0].y = mat1[0].x * mat2[0].y + mat1[0].y * mat2[1].y + mat1[0].z * mat2[2].y + mat1[0].w * mat2[3].y;
		ans[0].z = mat1[0].x * mat2[0].z + mat1[0].y * mat2[1].z + mat1[0].z * mat2[2].z + mat1[0].w * mat2[3].z;
		ans[0].w = mat1[0].x * mat2[0].w + mat1[0].y * mat2[1].w + mat1[0].z * mat2[2].w + mat1[0].w * mat2[3].w;
		
		ans[1].x = mat1[1].x * mat2[0].x + mat1[1].y * mat2[1].x + mat1[1].z * mat2[2].x + mat1[1].w * mat2[3].x;
		ans[1].y = mat1[1].x * mat2[0].y + mat1[1].y * mat2[1].y + mat1[1].z * mat2[2].y + mat1[1].w * mat2[3].y;
		ans[1].z = mat1[1].x * mat2[0].z + mat1[1].y * mat2[1].z + mat1[1].z * mat2[2].z + mat1[1].w * mat2[3].z;
		ans[1].w = mat1[1].x * mat2[0].w + mat1[1].y * mat2[1].w + mat1[1].z * mat2[2].w + mat1[1].w * mat2[3].w;

		ans[2].x = mat1[2].x * mat2[0].x + mat1[2].y * mat2[1].x + mat1[2].z * mat2[2].x + mat1[2].w * mat2[3].x;
		ans[2].y = mat1[2].x * mat2[0].y + mat1[2].y * mat2[1].y + mat1[2].z * mat2[2].y + mat1[2].w * mat2[3].y;
		ans[2].z = mat1[2].x * mat2[0].z + mat1[2].y * mat2[1].z + mat1[2].z * mat2[2].z + mat1[2].w * mat2[3].z;
		ans[2].w = mat1[2].x * mat2[0].w + mat1[2].y * mat2[1].w + mat1[2].z * mat2[2].w + mat1[2].w * mat2[3].w;

		ans[3].x = mat1[3].x * mat2[0].x + mat1[3].y * mat2[1].x + mat1[3].z * mat2[2].x + mat1[3].w * mat2[3].x;
		ans[3].y = mat1[3].x * mat2[0].y + mat1[3].y * mat2[1].y + mat1[3].z * mat2[2].y + mat1[3].w * mat2[3].y;
		ans[3].z = mat1[3].x * mat2[0].z + mat1[3].y * mat2[1].z + mat1[3].z * mat2[2].z + mat1[3].w * mat2[3].z;
		ans[3].w = mat1[3].x * mat2[0].w + mat1[3].y * mat2[1].w + mat1[3].z * mat2[2].w + mat1[3].w * mat2[3].w;
		return ans;
	}

	float4x4 PersonalMath::TurnTo(float4x4 our, float4x4 target, float speed)
	{
		float4x4 ma1 = { float4{1,2,3,4},float4{5,6,7,8},float4{9,10,11,12},float4{13,14,15,16} };
		float4x4 ma2 = { float4{17,18,19,20},float4{21,22,23,24},float4{25,26,27,28},float4{29,30,31,32} };
		float4x4 ans = MultiplyMat(ma1, ma2);
		float3 sub = target[3].xyz - our[3].xyz;
		sub = normalize(sub);
		float checkingdir = dot(sub, our[0].xyz);
		float checkingdirX = dot(sub, our[1].xyz);
		float4x4 rotationMat = Identity();
		rotationMat = RotationY(speed * 50 * checkingdir);
		our = MultiplyMat(rotationMat,our);
		rotationMat = Identity();
		rotationMat = RotationX(speed * 50 * -checkingdirX);
		our = MultiplyMat(rotationMat, our);
		return Orthonormalize(our);
		
	}

	float4x4 PersonalMath::Orthonormalize(float4x4 mat)
	{
		float3 Z = mat[2].xyz;
		Z = normalize(Z);
		float3 X = cross(float3(0,1,0), Z);
		X = normalize(X);
		float3 Y = cross(Z, X);
		Y = normalize(Y);


		float4x4 ans = { ConToF4(X),
							 ConToF4(Y),
							 ConToF4(Z),
							 float4 {mat[3].x,mat[3].y,mat[3].z,1} };

		return ans;
	}
}
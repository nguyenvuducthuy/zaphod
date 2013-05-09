#include "Box.h"
#include "Intersection.h"

using namespace DirectX::SimpleMath;

Box::Box(Vector3 _pos, float _extendX, float _extendY, float _extendZ)
{
	SetPosition(_pos);
	SetExtendX(_extendX);
	SetExtendY(_extendY);
	SetExtendZ(_extendZ);
}

void Box::SetExtendX(float _x)
{
	m_Box.Extents.x = _x;
}

void Box::SetExtendY(float _y)
{
	m_Box.Extents.y = _y;
}

void Box::SetExtendZ(float _z)
{
	m_Box.Extents.z = _z;
}

void Box::SetPosition(Vector3 _pos)
{
	BaseObject::SetPosition(_pos);
	m_Box.Center = _pos;
}

bool Box::Intersect(const Ray& _ray, Intersection& _intersect)
{
	float dist;
	if(_ray.Intersects(m_Box, dist))
	{
		if(dist < 0.001f)
			return false;

		_intersect.position = _ray.position + dist * _ray.direction;
		Vector3 fromCenter = _intersect.position - Vector3(m_Box.Center.x, m_Box.Center.y, m_Box.Center.z);
		fromCenter.x /= m_Box.Extents.x;
		fromCenter.y /= m_Box.Extents.y;
		fromCenter.z /= m_Box.Extents.z;

		float dotX = fromCenter.Dot(Vector3(1,0,0));
		float dotY = fromCenter.Dot(Vector3(0,1,0));
		float dotZ = fromCenter.Dot(Vector3(0,0,1));

		float absX = abs(dotX);
		float absY = abs(dotY);
		float absZ = abs(dotZ);
		float largest = absX;

		if(absY > largest)
			largest = absY;
		if(absZ> largest)
			largest = absZ;
		
		if(largest == absX)
			_intersect.normal = dotX * Vector3(1,0,0);
		else if(largest == absY)
			_intersect.normal = dotY * Vector3(0,1,0);
		else if(largest == absZ)
			_intersect.normal = dotZ * Vector3(0,0,1);
		_intersect.normal.Normalize();

		_intersect.color = m_Diffuse;

		return true;
	}
	return false;
}

Box::~Box(void)
{
}

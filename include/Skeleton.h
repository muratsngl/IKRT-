#ifndef SKELETON.H
#define SKELETON.H

#include "vector"
struct Joint {
	int jointID;
	float position;
	int childID;
	
};
struct Bone {
	int boneID;
	//ASSUMPTION: one joint is associated with 1 bone;
	int associatedjointID[2];
};

class Skeleton
{
public:
	Skeleton();
	~Skeleton();

private:
	std::vector<Joint> joints;
	std::vector<Bone> bones;
};

Skeleton::Skeleton()
{
}

Skeleton::~Skeleton()
{
}


#endif


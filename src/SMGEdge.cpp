//
//  SMGEdge.cpp
//  
//
//  Created by NAOYAIWAMOTO on 25/10/2016.
//
//

#include "SMGEdge.h"

SMGEdge::SMGEdge() : mError(0.0f)
{
    
}

SMGEdge::~SMGEdge()
{
    
}

void SMGEdge::setStartNode(SMGNode *node)
{
    this->mNode1 = node;
}

void SMGEdge::setDestNode(SMGNode *node)
{
    this->mNode2 = node;
}

void SMGEdge::setError(const float &errorValue)
{
    this->mError = errorValue;
}

SMGNode *SMGEdge::getStartNode() const
{
    return this->mNode1;
}

SMGNode *SMGEdge::getDestNode() const
{
    return this->mNode2;
}
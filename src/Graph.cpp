//
//  Graph.cpp
//  
//
//  Created by NAOYAIWAMOTO on 09/10/2016.
//
//

#include "Graph.h"

using namespace Euclid;

Graph::Graph()
{
    this->mIndices = NULL;
    this->mNNodes = 0;
    this->mCurrentMotionIndex = 0;
    this->mCurrentFrame = 0;
}

Graph::~Graph()
{
    mNodes.clear();
    mMapLabelIndex.clear();
}

const int Graph::getNumNodes() const
{
    return this->mNNodes;
}

Node* Graph::getNode(const int node)
{
    if(node >= this->mNNodes || node < 0){
        std::cerr << "can't find the node index : " << node << "/" << this->mNNodes << std::endl;
        assert(node < this->mNNodes && node >= 0);
    }
    else
        return this->mNodes.at(node);
}

const bool Graph::hasBranch(const int motionIndex, const int frameId)
{
    this->mCurrentMotionIndex = motionIndex;
    this->mCurrentFrame = frameId;
    if(this->mIndices[motionIndex][frameId] > 0)    // (TBD) sometimes had been permited return the number such as 2910892
        return true;
    else
        return false;
}

const int Graph::getNodeindex(const int motionIndex, const int frameId)
{
    mCurrentMotionIndex = motionIndex;
    mCurrentFrame = frameId;
    return this->mIndices[motionIndex][frameId];
}

void Graph::constructGraph(const std::vector<Motion>& motions, const int nMotions, const float& threshold, const int nCoincidents)
{
    if(nMotions == 0) return;
    
    // initialize
    this->initIndices(motions, nMotions);
    
    Map *map;                  // ErrorMap

    map = new Map(nMotions);
    map->setThreshold(threshold);
    map->setNSteps(nCoincidents);
    map->constructMap(motions, nMotions); // also export each map file.
    
    for(int i=0 ; i<map->getNRelations(); i++){
        std::string label1 = map->getRelations(i, 0);
        std::string label2 = map->getRelations(i, 1);
        
        int index1 = -1, index2 = -1;
        
        int j=0;
        for(auto m : motions){
            if(m.getLabel() == label1) index1 = j;
            if(m.getLabel() == label2) index2 = j;
            j++;
        }
        
        int nRelations = 0; // number of similar poses
        
        if(index1 != -1 && index2 != -1) {
            std::vector<int> minFrame1, minFrame2;
            
            // Get Minumum
            nRelations = map->calcMinimums(i, minFrame1, minFrame2);
            
            for(j=0; j<nRelations; j++) {
                if(this->mIndices[index1][minFrame1[j]] < 0 || this->mIndices[index1][minFrame1[j]] > this->mNNodes){
                    Node *n = new Node();
                    this->mIndices[index1][minFrame1[j]] = this->addNode(n);
                }
                
                if(this->mIndices[index2][minFrame2[j]] < 0 || this->mIndices[index2][minFrame2[j]] > this->mNNodes){
                    Node *n = new Node();
                    this->mIndices[index2][minFrame2[j]] = this->addNode(n);
                }
                
                this->createTransitions(label1, this->mIndices[index1][minFrame1[j]], minFrame1[j], index1,
                                        label2, this->mIndices[index2][minFrame2[j]], minFrame2[j], index2,
                                        j, map->getNSteps(),
                                        motions.at(index1).getNFrames(), motions.at(index2).getNFrames() );
                
            }
        }
    }
}

bool Graph::loadGraph(const std::string& filename)
{
    cout << "Load Graph : " << filename << endl;
    
    ofFile file;
    std::string label = file.getAbsolutePath() + "/" + filename;
    
    std::ifstream inFile;
    inFile.open(label, std::ios_base::in);
    if(!inFile){
        cout << "counldn't find : " << label << endl;
        return false;
    }
    
    const int MAX_LINE_CHAR = 512;
    char szLine[MAX_LINE_CHAR] = "";
    char szLineTokens[MAX_LINE_CHAR] = "";
    
    inFile.getline(szLine, MAX_LINE_CHAR); //line of file type
    if( 0 != strcmp(szLine,"# Graph Version 0.1") ) //check the file type
    {
        printf("Graph file should begin with \"# Graph Version 0.1\"  \n");
        return false;
    }
    
    // loop process the file body
    while( true )
    {
        inFile.getline(szLine, MAX_LINE_CHAR);
        
        if( 0 == strlen(szLine) ) // end of file
            break;
        
        strcpy(szLineTokens, szLine);
        
        // split the line to tokens
        std::vector<char *> tokens;
        const char* szDelimit = " ,=:\"{}\t";
        char *token = strtok(szLineTokens, szDelimit);
        
        while(token)
        {
            tokens.push_back(token);
            token = strtok(NULL, szDelimit);
        }
        
        if( 0 == tokens.size() )
            continue;
        if( tokens[0][0] == '#') // comment line
            continue;
        
        if( 0 == strcmp("MotionNum", tokens[0]) )
        {
            int motionNum = std::atoi(tokens[1]);
            cout << "motionNum : " << motionNum << endl;
            this->mIndices = new int *[motionNum];
        }
        else if ( 0 == strcmp("MotionLabel", tokens[0]) )
        {
            int motionIndex = this->mMapLabelIndex.size();
            string motionLabel = tokens[1];
            this->mMapLabelIndex.insert( map<std::string, int>::value_type( motionLabel, motionIndex ) );
            
            cout << "motion label : " << motionLabel << endl;

            if ( 0 == strcmp("frameNum", tokens[2]) )
            {
                int numFrame = std::atoi(tokens[3]);
                this->mIndices[motionIndex] = new int [numFrame];
                
                // initialization
                for(int i=0; i<numFrame; i++) {
                    this->mIndices[motionIndex][i] = -1;
                }
            }
        }
        else if( 0 == strcmp("MotionName", tokens[0]) )
        {
            string motionName = tokens[1];
            if( 0 == strcmp("frame", tokens[2]) )
            {
                int frame = std::atoi(tokens[3]);
                auto index = mMapLabelIndex.find(motionName);
                Node *n = new Node;
                n->setFrameID(frame);
                n->setMotionID(index->second);
                n->setMotionLabel(motionName);
                this->mIndices[index->second][frame] = this->addNode(n);
            }
        }
        else if( 0 == strcmp("MotionLink", tokens[0]) )
        {
            int nodeIndex = std::atoi(tokens[1]);
            int numLinks = std::atoi(tokens[2]);
            
            if(numLinks > 0)
            {
                for(int i=0; i<numLinks; i++) {
                    int index = std::atoi(tokens[3+i]);
                    Edge *e;
                    e = new Edge(this->mNodes[index]);
                    this->mNodes[nodeIndex]->addEdge(e);
                }
            }
        }
    }
	return true;
}

void Graph::exportGraphFile(const string& filename, const std::vector<Motion>& motion)
{
    std::cout << "export graph file: " << endl;
    
    std::ofstream ofs(filename);
    if(!ofs) {
        std::cerr << "Failed open file" << std::endl;
        std::exit(1);
    }else{
        cout << "export... " << filename << endl;
    }

    int motionNum = motion.size();
    
    std::cout << "Numbert of Node" << this->getNumNodes() << endl;
    ofs << "# Graph Version 0.1" << endl;
    ofs << "MotionNum " << motionNum << endl;
    
    for(int i=0; i<motionNum; i++) {
        ofs <<  "MotionLabel "  << motion[i].getLabel()     << " " <<
                "frameNum "     << motion[i].getNFrames()   << std::endl;
    }

    for(int i=0; i<this->getNumNodes(); i++) {
        ofs << "MotionName " << this->getNode(i)->getMotionLabel() << " ";
        ofs << "frame "      << this->getNode(i)->getFrameID() << endl;
    }
    
    for(int i=0; i<this->getNumNodes(); i++) {
        ofs << "MotionLink " << this->getNode(i)->getNodeID() << " " << this->getNode(i)->getNumEdges() << " ";
        for(int j=0; j<this->getNode(i)->getNumEdges(); j++) {
            ofs << this->getNode(i)->getEdge(j)->getDestNode()->getNodeID() << " ";
        }
        ofs << endl;
    }
    ofs.close();
}

void Graph::draw(const float& wScale, const float& hScale)
{
    ofVec2f offset(10,10);
    int count = 0;
    for(int i=0; i<this->getNumNodes(); i++) {
        
        int index1, index2;
        int frame1, frame2;
        
        frame1 = this->getNode(i)->getFrameID();
        index1 = this->getNode(i)->getMotionID();
        ofDrawCircle(offset.x + frame1 * wScale, offset.y + index1 * hScale, 2.0);
        
        for(int j=0; j<this->getNode(i)->getNumEdges(); j++) {
            index2 = this->getNode(i)->getEdge(j)->getDestNode()->getMotionID();
            frame2 = this->getNode(i)->getEdge(j)->getDestNode()->getFrameID();
            
            if(index1 == index2){
                ofPoint p1(offset.x + frame1 * wScale, offset.y + index1 * hScale);
                ofPoint p2(offset.x + frame2 * wScale, offset.y + index2 * hScale);
                ofPoint midPoint( (p1 + p2)/2 );
                midPoint.set(midPoint.x, midPoint.y);
                ofDrawLine(p1, midPoint);
                ofDrawLine(midPoint, p2);
                
            }else{
                ofDrawLine(offset.x + frame1 * wScale, offset.y + index1 * hScale, offset.x + frame2 * wScale, offset.y + index2 * hScale);
            }
            ofDrawCircle(offset.x + frame2 * wScale, offset.y + index2 * hScale, 2.0);
        }
    }
    
    ofSetColor(255, 0, 0);
    ofDrawCircle(offset.x + mCurrentFrame * wScale, offset.y + mCurrentMotionIndex * hScale, 10.0);
}

const int Graph::addNode(Node *node)
{
    node->setNodeID(this->mNNodes);
    this->mNodes.push_back(node);
    this->mNNodes++;
    return node->getNodeID();
}

/*
 <------------->        <----> <----->
 *---------------*      *------*-------*
 */
void Graph::insertNode(Node *n)
{
    
    int count = 0;
    for(int i=0; i<this->mNodes.size(); i++) {
        if(this->mNodes[i]->getMotionID() == n->getMotionID()) {
            
            for(int j=0; j<this->mNodes[i]->getNumEdges(); j++) {
                if(this->mNodes[i]->getEdge(j)->getDestNode()->getMotionID() == n->getMotionID()) {
                    
                    if(this->mNodes[i]->getFrameID() < n->getFrameID() && n->getFrameID() < this->mNodes[i]->getEdge(j)->getDestNode()->getFrameID())
                    {
                        Edge *e = new Edge(this->mNodes[i]->getEdge(j)->getDestNode());
                        n->addEdge(e);
                        this->mNodes[i]->getEdge(j)->setDestNode(n);
                        //this->mNodes[i]->setEndFrameID(this->mNodes[i]->getEdge(j)->getDestination()->getFrameID());
                        count++;
                    }
                }
            }
        }
    }
}

void Graph::initIndices(const std::vector<Motion>& motions, const int nMotions)
{
    int maxFrames = -1;
    
    for(auto m : motions)
    {
        if(maxFrames < m.getNFrames())
            maxFrames = m.getNFrames();
    }
    
    cout << "nMotions : " << nMotions << endl;
    
    this->mIndices = new int *[nMotions];
    
    for(int i=0; i<nMotions; i++){
        this->mIndices[i] = new int [maxFrames];
    }
    
    for(int i=0; i<nMotions; i++){
        for(int j=0; j<maxFrames; j++){
            this->mIndices[i][j] = -1;
        }
    }
    
    // make graph structure with setting node and edge
    /*
            e1
     1-------------->2

     */
    int i=0;
    for(auto m : motions) {
        Node *n1 = new Node;
        Node *n2 = new Node;
        Edge *e = new Edge(n2);
        
        n1->addEdge(e);
        n1->setMotionID(i);
        n1->setMotionLabel(m.getLabel());
        n1->setFrameID(0);
        
        int lastPos = m.getNFrames()-1;
        n2->setMotionID(i);
        n2->setMotionLabel(m.getLabel());
        n2->setFrameID(lastPos);

        this->mIndices[i][0] = this->addNode(n1);
        this->mIndices[i][lastPos] = this->addNode(n2);
        i++;
    }
}

/*
        e1					   e3	   e4
 1-------------->2			1----->5------->2
                    ---->		    \ e7
                                     \
 3-------------->4			3-------->6---->4
        e2						e5		e6
 */
void Graph::createTransitions(std::string& m1, int node1, int frame1, int motionID1,
                              std::string& m2, int node2, int frame2, int motionID2,
                              int transiction, int range, int totalFrames1, int totalFrames2)
{
    std::stringstream ss;
    ss << m1 << "_" ;
    ss << m2 << "_" << transiction;
    std::string newName = ss.str();

    this->getNode(node1)->setFrameID(frame1);
    this->getNode(node2)->setFrameID(frame2);
    
    this->getNode(node1)->setMotionID(motionID1);
    this->getNode(node2)->setMotionID(motionID2);
    
    this->getNode(node1)->setMotionLabel(m1);
    this->getNode(node2)->setMotionLabel(m2);
    
    this->insertNode(this->getNode(node1));
    this->insertNode(this->getNode(node2));
    
    // TBD
    // not cycle in same motion
    if(motionID1 != motionID2){
        this->getNode(node1)->addEdge(new Edge(this->getNode(node2)));
    }
}

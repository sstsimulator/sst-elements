// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "emberunstructured.h"
#include "../../embercustommap.h"

using namespace SST::Ember;

EmberUnstructuredGenerator::EmberUnstructuredGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "Unstructured"), 
	m_loopIndex(0)
{
	graphFile  = params.find<std::string>("arg.graphfile", "-1");
	if(graphFile.compare("-1") == 0)
		fatal(CALL_INFO, -1, "Communication graph file must be specified for unstructured motifs!\n");

	p_size  = (uint32_t) params.find<uint32_t>("arg.p_size", 10000);
	items_per_cell = (uint32_t) params.find<uint32_t>("arg.fields_per_cell", 1);
	sizeof_cell = (uint32_t) params.find<uint32_t>("arg.datatype_width", 8);
	iterations = (uint32_t) params.find<uint32_t>("arg.iterations", 1);
	nsCompute  = (uint64_t) params.find<uint64_t>("arg.computetime", 0);

    jobId        = (int) params.find<int>("_jobId"); //NetworkSim
	configure();
}


void EmberUnstructuredGenerator::configure()
{
	//Check the type of the rankmapper: CustomMap or LinearMap
	bool use_CustomRankMap;
	EmberCustomRankMap* cm = dynamic_cast<EmberCustomRankMap*>(m_rankMap);
	if(cm == NULL)
		use_CustomRankMap = false;
	else
		use_CustomRankMap = true;

    if(0 == rank()) {
		output("Unstructured motif problem size: %" PRIu32 "\n", p_size);
		output("Unstructured motif compute time: %" PRIu32 " ns\n", nsCompute);
		output("Unstructured motif iterations: %" PRIu32 "\n", iterations);
		output("Unstructured motif iterms/cell: %" PRIu32 "\n", items_per_cell);
		output("Unstructured motif communication graph file: %s \n", graphFile.c_str());
		if(use_CustomRankMap)
			output("Unstructured motif rankmap: CustomRankMap\n");
		else
			output("Unstructured motif rankmap: LinearRankMap\n");			
	}
	
	//Read raw communication from the graph file
	rawCommMap = readCommFile(graphFile, size());

	if(0 == rank()){
		for(unsigned int i = 0; i < rawCommMap->size(); i++){
	        //std::cout << "rawCommMap(" << i << "):" << std::endl;

	        for(std::map<int, int>::iterator it = rawCommMap->at(i).begin(); it != rawCommMap->at(i).end(); it++){
	        	//std::cout << i << " communicates with " << it->first << std::endl;
			}
		}
	}	
	

	//Create the actual communication map based on the custom task mapping
	//Ex: TaskMapping on Node0 [3,5,7,120] -> CustomMap[0]=3, CustomMap[1]=5, CustomMap[2]=7, CustomMap[3]=120
	//	  If rawCommMap says Task0 communicates Task1 -> CommMap says Task3 communicates Task5

	//Create the actual communication map based on the custom task mapping
	//Ex: TaskMapping on Node0 [3,5,7,120] -> CustomMap[0]=3, CustomMap[1]=5, CustomMap[2]=7, CustomMap[3]=120
	//	  If rawCommMap says Task0 communicates Task1 -> CommMap says Task3 communicates Task5

	if (use_CustomRankMap){
        CommMap = new std::vector<std::map<int,int> >(size());
		int srcTask, destTask;
		for(unsigned int i = 0; i < CommMap->size(); i++){
	        srcTask = cm->CustomMap[i];
	        //if(0 == rank())
	        	//std::cout << "Rank(" << i << ") is in fact Rank(" << srcTask << ")"<< std::endl;

	        for(std::map<int, int>::iterator it = rawCommMap->at(i).begin(); it != rawCommMap->at(i).end(); it++){
	        	destTask = cm->CustomMap[it->first];
	        	CommMap->at(srcTask)[destTask] = 1; //1 could be changed to the weight (it->second) in the future
	        	//if(0 == rank()) 
	        		//std::cout << srcTask << " communicates with " << destTask << std::endl;
			}
		}
        delete rawCommMap;
	}
	//Actual communication map is the same as the raw communication map
	else{
		CommMap = rawCommMap;
	}

	/*
	for(unsigned int i = 0; i < rawCommMap->size(); i++){
	    delete [] rawCommMap->at(i);
	}
	delete rawCommMap;
	*/
	//output("My rank is: %" PRIu32 "\n", rank()); // NetworkSim
}

bool EmberUnstructuredGenerator::generate( std::queue<EmberEvent*>& evQ ) 
{
    verbose(CALL_INFO, 1, 0, "loop=%d\n", m_loopIndex );

        if ( 0 == m_loopIndex ) {
            m_startTime = getCurrentSimTimeMicro();
            //enQ_getTime( evQ, &m_startTime );
        }

        if ( m_loopIndex == iterations ) {
            //NetworkSim: report total running time
            //enQ_getTime( evQ, &m_stopTime );
            m_stopTime = getCurrentSimTimeMicro();
            if ( 0 == rank() ) {
                double latency = (double)(m_stopTime-m_startTime);
                double latency_per_iter = latency/(double)iterations;
                output("Motif Latency: JobNum:%d Total latency:%.3f us\n", jobId, latency );
                output("Motif Latency: JobNum:%d Per iteration latency:%.3f us\n", jobId, latency_per_iter );
                output("Job Finished: JobNum:%d Time:%" PRIu64 " us\n", jobId,  getCurrentSimTimeMicro());
            }//end->NetworkSim
            return true;
        }
        //end->NetworkSim

    	if(nsCompute > 0) {
    		enQ_compute( evQ, nsCompute);
    	}

		std::vector<MessageRequest*> requests;

        for(std::map<int, int>::iterator it = CommMap->at(rank()).begin(); it != CommMap->at(rank()).end(); it++){
			MessageRequest*  req  = new MessageRequest();
			requests.push_back(req);
        	
        	//std::cout << rank() << " communicates with " << it->first << std::endl;
			enQ_irecv( evQ, (int32_t)it->first, items_per_cell * sizeof_cell * p_size, 0, GroupWorld, req);
			enQ_send( evQ , (int32_t)it->first, items_per_cell * sizeof_cell * p_size, 0, GroupWorld);
		}

		for(uint32_t i = 0; i < requests.size(); ++i) {
			enQ_wait( evQ, requests[i]);
		}

		requests.clear();

		/*
		if(nsCopyTime > 0) {
			enQ_compute( evQ, nsCopyTime);
		}
		*/

    /*
    if ( ++m_loopIndex == iterations ) {
        //enQ_getTime( evQ, &m_stopTime );
        m_stopTime = getCurrentSimTimeMicro();
    	//NetworkSim: report total running time
    	if ( 0 == rank() ) {
            double latency = (double)(m_stopTime-m_startTime);
            double latency_per_iter = latency/(double)iterations;
            output("Motif Latency: JobNum:%d Total latency:%.3f us\n", jobId, latency );
            output("Motif Latency: JobNum:%d Per iteration latency:%.3f us\n", jobId, latency_per_iter );
            output("Job Finished: JobNum:%d Time:%" PRIu64 " us\n", jobId,  getCurrentSimTimeMicro());
    	}//end->NetworkSim

        return true;
    } else {
        return false;
    }
    */
    m_loopIndex++;
    return false;
    
}

std::vector<std::map<int,int> >* EmberUnstructuredGenerator::readCommFile(std::string fileName, int procsNeeded)
{
    //read matrix
	MatrixMarketReader2D<int> reader = MatrixMarketReader2D<int>();
    std::vector<int*>* dataVec = reader.readMatrix(fileName.c_str(), true); //current version ignores edge weights

    //NetworkSim
    if(dataVec == NULL)
        fatal(CALL_INFO, -1, "Could not read the matrix file properly\n");
    //end->NetworkSim

	//check size
	if(reader.xdim != reader.ydim){
		fatal(CALL_INFO, -1, "Given matrix in file %s is not a square matrix\n", fileName.c_str());
	} else if(reader.xdim != procsNeeded){
    	fatal(CALL_INFO, -1, "The size of the matrix in file %s does not match with the job size\n", fileName.c_str());
    }

	//convert matrix to adjacency & weight list
	//init vector
	std::vector<std::map<int,int> >* retVec = new std::vector<std::map<int,int> >;
	retVec->resize(procsNeeded);

	//fill with data
	for(unsigned int i = 0; i < dataVec->size(); i++){
	    retVec->at(dataVec->at(i)[0])[dataVec->at(i)[1]] = dataVec->at(i)[2];
	    delete [] dataVec->at(i);
	}
	delete dataVec;

	return retVec;
}

template <class T>
std::vector<T*>* MatrixMarketReader2D<T>::readMatrix(const char* fileName, bool ignoreValues)
{
    //TODO: make this function faster by reading the file all at once
    //TODO: print the errors inside this function instead of returning NULL

    //open file
    std::ifstream inputFile;
    inputFile.open( fileName, std::fstream::in );
    if(!inputFile.is_open()){
        return NULL;
        //fatal(CALL_INFO, -1, "Unable to open matrix market file: %s\n", fileName);
    }

    //read header
    std::string fType, object, format, field, symmetry;
    if( !(inputFile >> fType >> object >> format >> field >> symmetry) ){
        return NULL;
        //fatal(CALL_INFO, -1, "Cannot read matrix market file: %s\n", fileName);
    }

    //check compatibility
    if(fType.compare("%%MatrixMarket") != 0){
        return NULL;
        //fatal(CALL_INFO, -1, "File %s is not a Matrix Market file\n", fileName);
    }
    if(object.compare("matrix") != 0){
        return NULL;
        //fatal(CALL_INFO, -1, "Object type in file %s should be a matrix\n", fileName);
    }
    if(!ignoreValues){
        if(typeid(int) == typeid(T)){
            if(field.compare("integer") != 0){
                return NULL;
        		//fatal(CALL_INFO, -1, "In file %s : integer input type is expected\n", fileName);
            }
        } else if(typeid(double) == typeid(T)) {
            if(field.compare("real") != 0 && field.compare("double") != 0){
                return NULL;
        		//fatal(CALL_INFO, -1, "In file %s : double input type is expected\n", fileName);
            }
        }
    }

    //ignore comments
    while (inputFile.peek() == '%' || inputFile.peek() == '\n'){
        inputFile.ignore(2048, '\n');
    }

    //read the size line
    int numLines;
    if( !(inputFile >> xdim >> ydim) ) {
        return NULL;
        //fatal(CALL_INFO, -1, "Could not read matrix market input %s\n", fileName);
    }

    //read data
    std::vector<T*>* outVector = new std::vector<T*>();
    T* tempData;
    double ignoredDummy;
    std::string line;

    if(format.compare("coordinate") == 0){
        inputFile >> numLines;
        inputFile.ignore(2048, '\n');
        for(int i = 0; i < numLines; i++){
            tempData = new T[3];
            if(ignoreValues){
                getline(inputFile, line);
                std::stringstream is(line);
                is >> tempData[0] >> tempData[1] >> ignoredDummy;
                tempData[2] = 1;
            } else {
                if( !(inputFile >> tempData[0] >> tempData[1] >> tempData[2]) ){
                    return NULL;
        			//fatal(CALL_INFO, -1, "Could not read matrix market input %s\n", fileName);
                }
            }
            //convert numbering convention
            tempData[0] -= 1;
            tempData[1] -= 1;
            outVector->push_back(tempData);
        }
    } else if(format.compare("array") == 0) {
        //init outVector
        for(int i = 0; i < xdim; i++){
            outVector->push_back(new T[ydim]);
        }
        //fill outVector
        for(int j = 0; j < ydim; j++){
            for(int i = 0; i < xdim; i++){
                tempData = outVector->at(i);
                if( !(inputFile >> tempData[j]) ){
                    return NULL;
        			//fatal(CALL_INFO, -1, "Number of lines in file %s does not match with the matrix.\n", fileName);
                }
            }
        }
    } else {
        return NULL;
        //fatal(CALL_INFO, -1, "Unknown matrix file format in file %s\n", fileName);
    }

    inputFile.close();

    //apply symmetry if needed
    if(symmetry.compare("general") != 0){
        T* tempDataSym;
        int size = outVector->size();
        for(int i = 0; i < size; i++){
            tempData = outVector->at(i);
            if(tempData[0] != tempData[1] && tempData[2] != 0){
                tempDataSym = new T[3];
                tempDataSym[0] = tempData[1];
                tempDataSym[1] = tempData[0];
                tempDataSym[2] = tempData[2];
                outVector->push_back(tempDataSym);
            }
        }
    }

    return outVector;
}


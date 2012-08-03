#include "thermalmodel.h"

Define_Module(ThermalModel);

#ifdef ENABLE_HOTSPOT

flp_t* ThermalModel::flp;
map<string, ThermalUnit*> ThermalModel::functionalUnitList;
char* ThermalModel::init_file;
char* ThermalModel::steady_file;
char* ThermalModel::grid_steady_file;

RC_model_t* ThermalModel::model;
double* ThermalModel::temp;
double* ThermalModel::power;

double* ThermalModel::overall_power;
double* ThermalModel::steady_temp;


ThermalUnit::ThermalUnit()
{
	unit = NULL;
	power = 0;
	energy = 0;
}

ThermalModel::ThermalModel()
{
	init_file = NULLFILE;
	//steady_file = NULLFILE;
	steady_file = "temp_file";
	grid_steady_file = "gridtemp_file";
	thermalCycle = NULL;
	thermalCyclePeriod = 10e-6;
	thermalFileDumpCycle = NULL;
	thermalFileDumpPeriod = 100e-6;
	prevTime = 0;
	firstCall = true;
	useGridModel = true;
	samples = 0;
}


void ThermalModel::initialize(int stage)
{
	if(stage == 0)
	{
		thermalCycle = new cMessage("Thermal Model Timer");
		scheduleAt(simTime() + thermalCyclePeriod, thermalCycle);
		thermalFileDumpCycle = new cMessage ("Thermal File Dump Timer");
		scheduleAt(simTime() + thermalFileDumpPeriod, thermalFileDumpCycle);
		// other modules need to initialize in this stage
	}
	if(stage == 3)
	{
		generateFLP();
		saveFLP("file.flp");
	}
}
void ThermalModel::finish()
{
	double total_elapsed_time = simTime().dbl();
	int i, j, base;

	/* find the average power dissipated in the elapsed time */
	if (model->type == BLOCK_MODEL)
	{
		for (i = 0; i < flp->n_units; i++)
			overall_power[i] /= total_elapsed_time;
	}
	else
	{
		for(i=0, base=0; i < model->grid->n_layers; i++)
		{
			if(model->grid->layers[i].has_power)
				for(j=0; j < model->grid->layers[i].flp->n_units; j++)
					overall_power[base+j] /= total_elapsed_time;
			base += model->grid->layers[i].flp->n_units;
		}
	}

	/* get steady state temperatures */
	steady_state_temp(model, overall_power, steady_temp);

	/* dump temperatures if needed	*/
	if (model->type == BLOCK_MODEL && strcmp(model->config->steady_file, NULLFILE))
	{
		std::cout<<"Dumping Final block model results..."<<endl;
		dump_temp(model, steady_temp, model->config->steady_file);
	}

	/* for the grid model, optionally dump the internal
	 * temperatures of the grid cells
	 */
	if (model->type == GRID_MODEL && strcmp(model->config->grid_steady_file, NULLFILE))
	{
		std::cout<<"Thermal: Dumping Final grid model results."<<endl;
		dump_steady_temp_grid(model->grid, model->config->grid_steady_file);
	}

	/* cleanup */
	delete_RC_model(model);
	free_flp(flp, FALSE);
	free_dvector(temp);
	free_dvector(power);
	free_dvector(steady_temp);
	free_dvector(overall_power);

	if(thermalCycle->isScheduled())
	{
		cancelEvent(thermalCycle);
	}
	delete thermalCycle;
}
void ThermalModel::handleMessage(cMessage *msg)
{

	 if(msg == thermalCycle)
	  {
	  	calcThermal(msg);
		//cycleTemperature();
	  }

	 if(msg == thermalFileDumpCycle)
	 {
		dumpThermalFile(msg);
	 }
/*
	string file(model->config->grid_steady_file);
	samples ++;
	std::ostringstream sstream;
	sstream << samples;
	std::string varAsString = sstream.str();


	double *inter_total_power = hotspot_vector(model);

	for (int i = varAsString.length() ; i < 8 ; i++)
	{
		file.append("0");
	}


	file.append(varAsString);

	char *c_file = NULL;
	c_file = strdup(file.c_str());

	if(msg == thermalCycle)
	{
		cycleTemperature();

		if (model->type == GRID_MODEL && strcmp(model->config->grid_steady_file, NULLFILE))
		{
			std::cout<<"Thermal: Dumping grid model results."<<endl;

			int base=0;
			double total_elapsed_time = simTime().dbl();
			for(int i=0; i < model->grid->n_layers; i++)
			{
				if(model->grid->layers[i].has_power)
					for(int j=0; j < model->grid->layers[i].flp->n_units; j++)
						inter_total_power[base+j] = overall_power[base+j] / total_elapsed_time;
				base += model->grid->layers[i].flp->n_units;
			}
			//std::cout<<overall_power[0]<<endl;

			steady_state_temp(model, inter_total_power, steady_temp);
			dump_steady_temp_grid(model->grid, c_file);

			free_dvector(inter_total_power);

			//dump_temp(model, temp, c_file);
		}

		scheduleAt(simTime() + thermalCyclePeriod, thermalCycle);
	}
	else
	{
		throw cRuntimeError("ThermalModel error: Messages should not be sent to this module");
	}*/
}

void ThermalModel::calcThermal(cMessage *msg)
{
	string file(model->config->grid_steady_file);

		double *inter_total_power = hotspot_vector(model);

		if(msg == thermalCycle)
		{
			cycleTemperature();

			if (model->type == GRID_MODEL && strcmp(model->config->grid_steady_file, NULLFILE))
			{
				std::cout<<"Thermal: Calculating grid model results."<<endl;

				int base=0;
				double total_elapsed_time = simTime().dbl();
				for(int i=0; i < model->grid->n_layers; i++)
				{
					if(model->grid->layers[i].has_power)
						for(int j=0; j < model->grid->layers[i].flp->n_units; j++)
							inter_total_power[base+j] = overall_power[base+j] / total_elapsed_time;
					base += model->grid->layers[i].flp->n_units;
				}
				//std::cout<<overall_power[0]<<endl;

				steady_state_temp(model, inter_total_power, steady_temp);
				//dump_steady_temp_grid(model->grid, c_file);

				free_dvector(inter_total_power);

				//dump_temp(model, temp, c_file);
			}

			scheduleAt(simTime() + thermalCyclePeriod, thermalCycle);
		}
		else
		{
			throw cRuntimeError("ThermalModel error: Messages should not be sent to this module");
		}
}

void ThermalModel::dumpThermalFile(cMessage *msg)
{
		string file(model->config->grid_steady_file);
		samples ++;
		std::ostringstream sstream;
		sstream << samples;
		std::string varAsString = sstream.str();


		double *inter_total_power = hotspot_vector(model);

		for (int i = varAsString.length() ; i < 8 ; i++)
		{
			file.append("0");
		}


		file.append(varAsString);

		char *c_file = NULL;
		c_file = strdup(file.c_str());

		if(msg == thermalFileDumpCycle)
		{
			//cycleTemperature();

			if (model->type == GRID_MODEL && strcmp(model->config->grid_steady_file, NULLFILE))
			{
				std::cout<<"Thermal: Dumping grid model results."<<endl;

				int base=0;
				double total_elapsed_time = simTime().dbl();
				for(int i=0; i < model->grid->n_layers; i++)
				{
					if(model->grid->layers[i].has_power)
						for(int j=0; j < model->grid->layers[i].flp->n_units; j++)
							inter_total_power[base+j] = overall_power[base+j] / total_elapsed_time;
					base += model->grid->layers[i].flp->n_units;
				}
				//std::cout<<overall_power[0]<<endl;

				steady_state_temp(model, inter_total_power, steady_temp);
				dump_steady_temp_grid(model->grid, c_file);

				free_dvector(inter_total_power);

				//dump_temp(model, temp, c_file);
			}

			scheduleAt(simTime() + thermalFileDumpPeriod, thermalFileDumpCycle);
		}
		else
		{
			throw cRuntimeError("ThermalModel error: Messages should not be sent to this module");
		}
}

bool ThermalModel::registerThermalObject(const char *name, int id, double width, double height, double leftx, double bottomy)
{
	unit_t *newFunctionalUnit = new unit_t();
	std::string uniqueNameStr = createUniqueName(name,id);
	const char *uniqueName = uniqueNameStr.c_str();

	strcpy(newFunctionalUnit->name,uniqueName);
	newFunctionalUnit->width = width;
	newFunctionalUnit->height = height;
	newFunctionalUnit->leftx = leftx;
	newFunctionalUnit->bottomy = bottomy;

	ThermalUnit *tu = new ThermalUnit();
	tu->unit = newFunctionalUnit;
	functionalUnitList.insert(std::pair<string, ThermalUnit*>(uniqueNameStr, tu));

	return true;

}

void ThermalModel::generateFLP()
// replaces read_flp(); No longer reads from file, receives blocks from OMNeT
{

	std::cout<<"Thermal: Loading HotSpot, "<<functionalUnitList.size()<<" units created."<<endl;
	flp = flp_alloc_init_mem(functionalUnitList.size());

	map<string,ThermalUnit*>::iterator it;

	// Remember to convert from um (PhoenixSim length units) to m (HotSpot length units)

	int i = 0;
	for(it = functionalUnitList.begin(); it != functionalUnitList.end(); it++)
	{
		strcpy(flp->units[i].name, it->second->unit->name);
		flp->units[i].width = it->second->unit->width * 1e-6;
		flp->units[i].height = it->second->unit->height * 1e-6;
		flp->units[i].leftx = it->second->unit->leftx * 1e-6;
		flp->units[i].bottomy = it->second->unit->bottomy * 1e-6;
		i++;
	}
	//if(read_connects)
	//	flp_populate_connects(flp,fp);

	for(int i = 0 ; i < flp->n_units ; i++)
	{
		for(int j = 0 ; j < flp->n_units ; j++)
		{
			flp->wire_density[i][j] = 1.0;
		}
	}
	//print_flp(flp);

	// set origin to (0,0)
	flp_translate(flp,0,0);
	thermal_config_t config = default_thermal_config();
	strcpy(config.init_file, init_file);
	strcpy(config.steady_file, steady_file);
	strcpy(config.grid_steady_file, grid_steady_file);

	if(useGridModel)
	{
		strcpy(config.model_type, GRID_MODEL_STR);
	}

	config.grid_cols = 128;
	config.grid_rows = 128;


	model = alloc_RC_model(&config, flp);
	populate_R_model(model, flp);
	populate_C_model(model, flp);
	temp = hotspot_vector(model);
	power = hotspot_vector(model);
	steady_temp = hotspot_vector(model);
	overall_power = hotspot_vector(model);
	if(strcmp(model->config->init_file, NULLFILE))
	{
		if(!model->config->dtm_used)
		{
			read_temp(model, temp, model->config->init_file, FALSE);
		}
		else
		{
			read_temp(model, temp, model->config->init_file, TRUE);
		}
	}
	else
	{
		set_temp(model, temp, model->config->init_temp);
	}

}

void ThermalModel::cycleTemperature()
{

	int i, j, base, idx;


	map<string,ThermalUnit*>::iterator it;

	/* set the per cycle power values as returned by Wattch/power simulator	*/
	if (model->type == BLOCK_MODEL)
	{
		for(it = functionalUnitList.begin(); it != functionalUnitList.end(); it++)
		{
			power[get_blk_index(flp, (char*)it->first.c_str())] += it->second->power;
			power[get_blk_index(flp, (char*)it->first.c_str())] += it->second->energy / thermalCyclePeriod;

			it->second->energy = 0;
		}
	/* for the grid model, set the power numbers for all power dissipating layers	*/
	} else
		for(i=0, base=0; i < model->grid->n_layers; i++) {


			if(model->grid->layers[i].has_power) {

				for(it = functionalUnitList.begin(); it != functionalUnitList.end(); it++)
				{
					idx = get_blk_index(flp, (char*)it->first.c_str());
					power[base+idx] += it->second->power;
					power[base+idx] += it->second->energy / thermalCyclePeriod;
				}
			}
			base += model->grid->layers[i].flp->n_units;
		}


	double elapsed_time = simTime().dbl() - prevTime;
	prevTime = simTime().dbl();

	/* find the average power dissipated in the elapsed time */
	if (model->type == BLOCK_MODEL) {
		for (i = 0; i < flp->n_units; i++) {
			/* for steady state temperature calculation	*/
			overall_power[i] += power[i]*elapsed_time;
			/*
			 * 'power' array is an aggregate of per cycle numbers over
			 * the sampling_intvl. so, compute the average power
			 */

			// JC: This line is probably unnecessary since we are not doing a cycle-by-cycle calculation
			//power[i] /= (elapsed_time * model->config->base_proc_freq);
		}
	/* for the grid model, account for all the power dissipating layers	*/
	} else
		for(i=0, base=0; i < model->grid->n_layers; i++) {
			if(model->grid->layers[i].has_power)
				for(j=0; j < model->grid->layers[i].flp->n_units; j++) {
					/* for steady state temperature calculation	*/
					overall_power[base+j] += power[base+j]*elapsed_time;
					/* compute average power	*/
					//power[base+j] /= (elapsed_time * model->config->base_proc_freq);
				}
			base += model->grid->layers[i].flp->n_units;
		}

	/* calculate the current temp given the previous temp, time
	 * elapsed since then, and the average power dissipated during
	 * that interval. for the grid model, only the first call to
	 * compute_temp passes a non-null 'temp' array. if 'temp' is  NULL,
	 * compute_temp remembers it from the last non-null call.
	 * this is used to maintain the internal grid temperatures
	 * across multiple calls of compute_temp
	 */
	if (model->type == BLOCK_MODEL || firstCall)
	{
		compute_temp(model, power, temp, elapsed_time);
	}
	else
	{
		compute_temp(model, power, NULL, elapsed_time);
	}
	/* make sure to record the first call	*/
	firstCall = false;

	/* reset the power array */
	if (model->type == BLOCK_MODEL)
	{
		for (i = 0; i < flp->n_units; i++)
		{
			power[i] = 0;
		}
	}
	else
	{
		for(i=0, base=0; i < model->grid->n_layers; i++)
		{
			if(model->grid->layers[i].has_power)
				for(j=0; j < model->grid->layers[i].flp->n_units; j++)
					power[base+j] = 0;
			base += model->grid->layers[i].flp->n_units;
		}
	}

}

void ThermalModel::setThermalObjectPower(const char *name, int id, double power)
{
	map<string,ThermalUnit*>::iterator it;
	it = functionalUnitList.find(createUniqueName(name,id));

	if(it != functionalUnitList.end())
	{
		it->second->power = power;
	}
	else
	{
		throw cRuntimeError("ThermalModel error: Trying to add power to a object name that doesn't exist");
	}
}

void ThermalModel::addThermalObjectEnergy(const char *name, int id, double energy)
{
	map<string,ThermalUnit*>::iterator it;
	it = functionalUnitList.find(createUniqueName(name,id));

	if(it != functionalUnitList.end())
	{
		it->second->energy += energy;
	}
	else
	{
		throw cRuntimeError("ThermalModel error: Trying to add energy to a object name that doesn't exist");
	}
}

string ThermalModel::createUniqueName(const char *name, int id)
{
	std::stringstream ss;
	ss << name << "_" << id;
	return ss.str();
}

double ThermalModel::getCurrentTemperature(const char *name, int id, char unitType)
{
	char* thename = (char*)createUniqueName(name,id).c_str();


	double unitTemp = temp[get_blk_index(flp, thename)];
	if(unitType == 'K')	// Kelvin
	{
		return unitTemp;
	}
	else if(unitType == 'C')	// Celsius
	{
		return unitTemp - 273.15;
	}
	else if(unitType == 'F')	// Fahrenheit
	{
		return unitTemp * 1.8 - 459.67;
	}
	else if(unitType == 'R')	// Rankine
	{
		return unitTemp *1.8;
	}
	else
	{
		throw cRuntimeError("ThermalModel error: invalid temperature unit specified");
	}
}


void ThermalModel::saveFLP(const char *filename)
{
	std::ofstream fout;
	map<string,ThermalUnit*>::iterator it;

	fout.open(filename);
	if(fout.is_open())
	{
		for(it = functionalUnitList.begin(); it != functionalUnitList.end(); it++)
		{
			fout<<it->second->unit->name<<"\t";
			fout<<it->second->unit->width * 1e-6<<"\t";
			fout<<it->second->unit->height * 1e-6<<"\t";
			fout<<it->second->unit->leftx * 1e-6<<"\t";
			fout<<it->second->unit->bottomy * 1e-6<<"\t\n";
		}
		fout.close();
		std::cout<<"Thermal: Saving floorplan to file, "<<filename<<"."<<endl;
	}
}

#else
void ThermalModel::initialize()
{
}

void ThermalModel::handleMessage(cMessage *msg)
{
	// TODO - Generated method body
}

void ThermalModel::finish()
{
}
#endif

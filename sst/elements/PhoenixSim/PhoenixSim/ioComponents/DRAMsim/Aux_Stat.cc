#include "Aux_Stat.h"

Aux_Stat::Aux_Stat(DRAM_config* cfg){
	config = cfg;
	tran_queue_valid_stats = NULL;
	biu_slot_valid_stats = NULL;
	bundle_stats = NULL;
	biu_access_dist_stats = NULL;

	bus_stats = NULL;
	bank_hit_stats = NULL;

	bank_conflict_stats = NULL;
    cas_per_ras_stats = NULL;
}

Aux_Stat::~Aux_Stat(){
	free(filename);
}

void Aux_Stat::mem_gather_stat_init(int stat_type, int page_size, int range ){
  int i;

  if(stat_type == GATHER_TRAN_QUEUE_VALID_STAT) {
	tran_queue_valid_stats = (int *)calloc(config->get_dram_channel_count()*MAX_TRANSACTION_QUEUE_DEPTH + 1,sizeof(int));
  } else if(stat_type == GATHER_BUNDLE_STAT) {
	bundle_stats = (int *)calloc(BUNDLE_SIZE + 1,sizeof(int));
  } else if(stat_type == GATHER_BIU_SLOT_VALID_STAT) {
//	fprintf(stdout,"Allocating biu_slot_valid_stats\n");
  	biu_slot_valid_stats = (int *)calloc(MAX_BUS_QUEUE_DEPTH ,sizeof(int));
	if (biu_slot_valid_stats == NULL)
	  fprintf(stdout,"Failed To allocate biu_slot_valid_stats \n");
  } else if(stat_type == GATHER_BUS_STAT){
      bus_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      bus_stats->next_b = NULL;
  } else if(stat_type == GATHER_BANK_HIT_STAT){
      bank_hit_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      bank_hit_stats->next_b = NULL;
  } else if(stat_type == GATHER_BANK_CONFLICT_STAT){
      bank_conflict_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      bank_conflict_stats->next_b = NULL;
  } else if(stat_type == GATHER_CAS_PER_RAS_STAT){
      cas_per_ras_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      cas_per_ras_stats->next_b = NULL;
  } else if (stat_type == GATHER_REQUEST_LOCALITY_STAT){
	  /*page_size = log2(page_size);*/	/* convert page size to power_of_2 format */
	  page_size = -9999;
	  locality_range = MIN(range + 1,CMS_MAX_LOCALITY_RANGE);
	  valid_entries = 0;

	  for(i = 0; i< CMS_MAX_LOCALITY_RANGE;i++){
		locality_hit[i] = 0;
		previous_address_page[i] = 0;
	  }

  }else if(stat_type == GATHER_BIU_ACCESS_DIST_STAT) {
	  biu_access_dist_stats= (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
	  biu_access_dist_stats->next_b = NULL;
  }
}

/* We set up the common file for output */
void Aux_Stat::mem_stat_init_common_file(char *f) {
  filename = f;
  if (filename != NULL) {
	stats_fileptr = fopen(filename,"w");
	if (stats_fileptr == NULL) {
	  fprintf(stdout,"Error opening stats common output file %s\n",filename);
		exit(0);
	}
  }
}


/* simply add an accumulation of the bus latency statistic */

void Aux_Stat::mem_gather_stat(int stat_type, int delta_stat){			/* arrange smallest to largest */
  int found;
  STAT_BUCKET 	*this_b;
  STAT_BUCKET 	*temp_b;
  int i;
  unsigned int current_address;
  unsigned int current_page_index;
  if ((stat_type == GATHER_TRAN_QUEUE_VALID_STAT) && (tran_queue_valid_stats != NULL)) {
	tran_queue_valid_stats[delta_stat]++;
	return;
  }else if((stat_type == GATHER_BIU_SLOT_VALID_STAT) && (biu_slot_valid_stats != NULL)) {
	biu_slot_valid_stats[delta_stat]++;
	return;
  } else if((stat_type == GATHER_BUNDLE_STAT) && (bundle_stats != NULL)) {
     bundle_stats[delta_stat]++;
	return;
  }
  if(delta_stat <= 0){	/* squashed biu entries can return erroneous negative numbers, ignore. Also ignore zeros. */
    return;
  } else if((stat_type == GATHER_BIU_ACCESS_DIST_STAT ) && (biu_access_dist_stats != NULL)) {
    this_b = biu_access_dist_stats->next_b;
  } else if((stat_type == GATHER_BUS_STAT) && (bus_stats != NULL)){
    this_b = bus_stats->next_b;
  } else if((stat_type == GATHER_BANK_HIT_STAT) && (bank_hit_stats != NULL)){
    this_b = bank_hit_stats->next_b;
  } else if((stat_type == GATHER_BANK_CONFLICT_STAT) && (bank_conflict_stats != NULL)){
    this_b = bank_conflict_stats->next_b;
  } else if((stat_type == GATHER_CAS_PER_RAS_STAT) && (cas_per_ras_stats != NULL)){
    this_b = cas_per_ras_stats->next_b;
  } else if((stat_type == GATHER_REQUEST_LOCALITY_STAT) ){

	  if(locality_range >= CMS_MAX_LOCALITY_RANGE){
		  fprintf(stdout, "ERROR: DRAMsim: AuxStat: locality_range not set\n");
		  exit(1);
	  }
    current_address = (unsigned int) delta_stat;
    current_page_index = current_address >> page_size;  /* page size already in power_of_2 format */

    for(i = (locality_range-1);i>0;i--){
      previous_address_page[i] = previous_address_page[i-1];
      if(previous_address_page[i] == current_page_index){
	locality_hit[i]++;
      }
    }

    valid_entries++;
    previous_address_page[0] = current_page_index;
    return;
  } else {
    return;
  }
  found = FALSE;
  if((this_b == NULL) || (this_b->delta_stat > delta_stat)){     	/* Nothing in this type of */
                                                        		/* transaction. Add a bucket */
    temp_b = (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
    temp_b -> next_b = this_b;
    temp_b -> count = 1;
    temp_b -> delta_stat = delta_stat;
    if(stat_type == GATHER_BUS_STAT){
      bus_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BANK_HIT_STAT) {
      bank_hit_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BANK_CONFLICT_STAT) {
      bank_conflict_stats->next_b = temp_b;
    } else if (stat_type == GATHER_CAS_PER_RAS_STAT) {
      cas_per_ras_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BIU_ACCESS_DIST_STAT) {
      biu_access_dist_stats->next_b = temp_b;
    }
    found = TRUE;
  } else if(this_b->delta_stat == delta_stat) {		/* add a count to this bucket */
    this_b->count++;
    found = TRUE;
  }
  while(!found && (this_b -> next_b) != NULL){
    if(this_b->next_b->delta_stat == delta_stat) {
      this_b->next_b->count++;
      found = TRUE;
    } else if(this_b->next_b->delta_stat > delta_stat){
      temp_b = (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
      temp_b -> count = 1;
      temp_b -> delta_stat = delta_stat;
      temp_b -> next_b= this_b->next_b;
      this_b -> next_b= temp_b;
      found = TRUE;
    } else {
      this_b = this_b -> next_b;
    }
  }
  if(!found){                     /* add a new bucket to the end */
    this_b->next_b= (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
    this_b = this_b->next_b;
    this_b ->next_b = NULL;
    this_b ->count 	= 1;
    this_b ->delta_stat= delta_stat;
  }
}

void Aux_Stat::free_stats(STAT_BUCKET *start_b){
  STAT_BUCKET     *next_b;
  STAT_BUCKET     *this_b;
  this_b = start_b;
  while(this_b != NULL){
    next_b = this_b -> next_b;
    free(this_b);
    this_b = next_b;
  }
}

void Aux_Stat::mem_print_stat(int stat_type,bool common_output){
  STAT_BUCKET 	*this_b = NULL;
  char		delta_stat_string[1024];
  FILE		*fileout;
  int		i;
   fileout = stats_fileptr;
   if (fileout == NULL)
	 fileout = stdout;

  if((stat_type == GATHER_BUS_STAT) && (bus_stats != NULL)){
    this_b = bus_stats->next_b;
    sprintf(&delta_stat_string[0],"\tLatency");
     fprintf(fileout,"\n\nLatency Stats: \n");
  } else if((stat_type == GATHER_TRAN_QUEUE_VALID_STAT) && (tran_queue_valid_stats != NULL)){
     fprintf(fileout,"\n\nTransaction Queue Stats: \n");
    sprintf(&delta_stat_string[0],"\tTot Valid Transactions");
	for (i=0;i<config->get_dram_channel_count()*MAX_TRANSACTION_QUEUE_DEPTH + 1;i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,tran_queue_valid_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BIU_SLOT_VALID_STAT) && (biu_slot_valid_stats != NULL)){
    fprintf(fileout,"\n\nBIU Stats: \n");
    sprintf(&delta_stat_string[0],"\tTot Valid BIU Slots");
	for (i=0;i<config->get_biu_queue_depth();i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,biu_slot_valid_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BUNDLE_STAT) && (bundle_stats != NULL)){
      fprintf(fileout,"\n\nBundle Stats: \n");
    sprintf(&delta_stat_string[0],"\tBundle Size");
	for (i=0;i<BUNDLE_SIZE + 1;i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,bundle_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BIU_ACCESS_DIST_STAT) && (biu_access_dist_stats != NULL)){
    this_b = biu_access_dist_stats->next_b;
    fprintf(fileout,"\n\nBIU Access Dist Stats: \n");
    sprintf(&delta_stat_string[0],"\tBIU Access Distance");
  } else if((stat_type == GATHER_BANK_HIT_STAT) && (bank_hit_stats != NULL)){
    this_b = bank_hit_stats->next_b;
     fprintf(fileout,"\n\nBank Hit Stats: \n");
    sprintf(&delta_stat_string[0],"Hit");
  } else if((stat_type == GATHER_BANK_CONFLICT_STAT) && (bank_conflict_stats != NULL)){
    this_b = bank_conflict_stats->next_b;
     fprintf(fileout,"\n\nBank Conflict Stats: \n");
    sprintf(&delta_stat_string[0],"Conflict");
  } else if((stat_type == GATHER_CAS_PER_RAS_STAT) && (cas_per_ras_stats != NULL)){
    this_b = cas_per_ras_stats->next_b;
    fprintf(fileout,"\n\nCas Per Ras Stats: \n");
    sprintf(&delta_stat_string[0],"Hits");
  } else if((stat_type == GATHER_REQUEST_LOCALITY_STAT)){

     fprintf(fileout,"\n\nRequest Locality Stats: \n");
    for(i=1; i<=locality_range; i++){
      fprintf(fileout," %3d  %9.5lf\n",
	      i,
	      locality_hit[i]*100.0/(double)valid_entries);
    }
  }else {
	return;
  }
  {
	int i =0;
  while(this_b != NULL && fileout != NULL){
	i++;
    fprintf(fileout,"%s %8d %6d\n",delta_stat_string,this_b->delta_stat,this_b->count);
    this_b = this_b->next_b;
  }
  }
}

void Aux_Stat::mem_close_stat_fileptrs(){
}

void Aux_Stat::init_extra_bundle_stat_collector() {
	int i;
	for (i = 0; i < BUNDLE_SIZE; i++) {
		num_cas[i] = 0;
		num_cas_w_drive[i] = 0;
		num_cas_w_prec[i] = 0;
		num_cas_write[i] = 0;
		num_cas_write_w_prec[i] = 0;
		num_ras[i] = 0;
		num_ras_refresh[i] = 0;
		num_ras_all[i] = 0;
		num_prec[i] = 0;
		num_prec_refresh[i] = 0;
		num_prec_all[i] = 0;
		num_refresh[i] = 0;
		num_drive[i] = 0;
		num_data[i] = 0;
		num_refresh[i] = 0;
		num_refresh_all[i] = 0;
	}
}

void Aux_Stat::gather_extra_bundle_stats(Command *bundle[], int command_count) {
	int i;
	Command *this_c;
	for (i = 0; i < command_count; i++) {
		this_c = bundle[i];
		switch (this_c->command) {
			case (RAS):
			   if (this_c->refresh)
				num_ras_refresh[command_count-1]++;
			   else
				num_ras[command_count-1]++;
				break;
			case (CAS):
				num_cas[command_count-1]++;
				break;
			case (CAS_AND_PRECHARGE):
				num_cas_w_prec[command_count-1]++;
				break;
			case (CAS_WITH_DRIVE):
				num_cas_w_drive[command_count-1]++;
				break;
			case (CAS_WRITE):
				num_cas_write[command_count-1]++;
				break;
			case (CAS_WRITE_AND_PRECHARGE):
				num_cas_write_w_prec[command_count-1]++;
				break;
			case (PRECHARGE):
			   if (this_c->refresh)
				num_prec_refresh[command_count-1]++;
			   else
				num_prec[command_count-1]++;
				break;
			case (PRECHARGE_ALL):
				num_prec_all[command_count-1]++;
				break;
			case (RAS_ALL):
				num_ras_all[command_count-1]++;
				break;
			case (DATA):
				num_data[command_count-1]++;
				i++;
				break;
			case (DRIVE):
				num_drive[command_count-1]++;
				break;
			case (REFRESH):
				num_refresh[command_count-1]++;
				break;
			case (REFRESH_ALL):
				num_refresh_all[command_count-1]++;
				break;
			default:
				fprintf(stdout, "Unknown command: %d\n", this_c->command);
				break;
		}
	}
}

void Aux_Stat::print_extra_bundle_stats(bool common_output) {
  FILE *fileout;
  if (common_output == FALSE) {
	fileout = stdout;
  }else {
	if (stats_fileptr == NULL)
	  fileout = stdout;
	else
	  fileout = stats_fileptr;
  }
  fprintf(fileout, "Bundle Size       1    2   3\n");

  fprintf(fileout, "RAS               %d %d %d\n", num_ras[0], num_ras[1], num_ras[2]);
  fprintf(fileout, "CAS               %d %d %d\n", num_cas[0], num_cas[1], num_cas[2]);
  fprintf(fileout, "CAS w PREC        %d %d %d\n", num_cas_w_prec[0], num_cas_w_prec[1], num_cas_w_prec[2]);
  fprintf(fileout, "CAS w DRIVE       %d %d %d\n", num_cas_w_drive[0], num_cas_w_drive[1], num_cas_w_drive[2]);
  fprintf(fileout, "CAS WRITE         %d %d %d\n", num_cas_write[0], num_cas_write[1], num_cas_write[2]);
  fprintf(fileout, "CAS WRITE w PREC  %d %d %d\n", num_cas_write_w_prec[0], num_cas_write_w_prec[1], num_cas_write_w_prec[2]);
  fprintf(fileout, "PREC              %d %d %d\n", num_prec[0], num_prec[1], num_prec[2]);
  fprintf(fileout, "RAS ALL           %d %d %d\n", num_ras_all[0], num_ras_all[1], num_ras_all[2]);
  fprintf(fileout, "PREC ALL          %d %d %d\n", num_prec_all[0], num_prec_all[1], num_prec_all[2]);
  fprintf(fileout, "RAS REFRESH       %d %d %d\n", num_ras_refresh[0], num_ras_refresh[1], num_ras_refresh[2]);
  fprintf(fileout, "PREC REFRESH      %d %d %d\n", num_prec_refresh[0], num_prec_refresh[1], num_prec_refresh[2]);
  fprintf(fileout, "REFRESH           %d %d %d\n", num_refresh[0], num_refresh[1], num_refresh[2]);
  fprintf(fileout, "REFRESH_ALL       %d %d %d\n", num_refresh_all[0], num_refresh_all[1], num_refresh_all[2]);
  fprintf(fileout, "DATA              %d %d %d\n", num_data[0], num_data[1], num_data[2]);
  fprintf(fileout, "DRIVE             %d %d %d\n", num_drive[0], num_drive[1], num_drive[2]);
}

FILE *Aux_Stat::get_common_stat_file() {
  return stats_fileptr;
}

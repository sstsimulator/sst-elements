/******************************************************************************
  Copyright & Disclaimer of Liability

  Copyright 2015, Sandia Corporation

  This source code has been authored by a contractor of the U.S. Government
  under contract No. DE-AC04-94AL85000. Accordingly, the U.S. Government retains
  certain rights in this source code.

  COMMERCIAL USE OR APPLICATION OF THIS SOURCE CODE IS STRICTLY PROHIBITED.

  THIS COPYRIGHT NOTICE AND THE AUTHORSHIP BLOCK BELOW MUST NOT BE REMOVED OR
  MODIFIED FOR ANY REASON.
******************************************************************************/
///////////////////////////////////////////////////////////////////////////////
/////////////////////////// OFFICIAL USE ONLY /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/******************************************************************************

 Author     : Jonathon W. Donaldson

 Project    : Spiking Temporal Processing Unit (STPU)

 Description: STPU User API Library

 Special Req: 

 Notes      : All signals ending with _n are active low.

******************************************************************************/

#ifndef _STPU_LIB_H_ /* prevent circular inclusions */
#define _STPU_LIB_H_ /* by using protection macros  */

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// DATA TYPES ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace Neuron_Loader_Types {

  // Neuron Configuration Transmission (NCT) (Floating-Point Format)
  typedef struct {
    float NrnThr; // Neuron Firing Potential
    float NrnMin; // Neuron Minimum Allowed Potential
    float NrnLkg; // Neuron Leakage Value
  } T_NctFl;

} // Neuron_Loader_Types

namespace White_Matter_Types {

  // White Matter Entry (WME) Format
  // AFR: Changed to uint16
  typedef struct {
    uint16_t SynStr; // Synaptic Strength
    uint16_t TmpOff; // Temporal Offset
    uint16_t SubAdr; // Sub-Address
    uint16_t Valid;  // Valid Flag
  } T_Wme;

} // White_Matter_Types

namespace Ctx_Seq_Mem_Types {

  // Context Sequence Entry (CSE) Format
  typedef struct {
    int      CtxNum; // Context Number
    uint32_t LifCnt; // LIF Count (NOTE!!! When in non-neuronal/non-wave/streaming mode, this value represents the number of individual LIFs to perform. When in neuronal/wave/non-streaming mode this value represents the number of 32-LIF *rounds* to perform!)
  } T_Cse;

} // Ctx_Seq_Mem_Types

namespace Ctrl_And_Stat_Types {

  // Brain Wave Pulse (BWP) Format (Floating-Point Format)
  typedef struct {
    float InpValFl; // Input Value
    int   InpNrn; // Input Neuron Number
    int   TmpSft; // Temporal Shift
  } T_BwpFl;

  // Dump Tuple Format
  typedef struct {
    int NrnNum; // Neuron Number
    int TmpIdx; // Temporal Index
  } T_Dmp;

} // Ctrl_And_Stat_Types

///////////////////////////////////////////////////////////////////
///////////////////// FUNCTION PROTOTYPES /////////////////////////
///////////////////////////////////////////////////////////////////

// This task opens the STPU device, initializes it, calls NAGR_STPU_PCIE, calls
// STRS_STPU_PCIE to disable spike train logging, INIT_ALL_NRNS, and returns a
// Hardware Extension Vector (HEV) struct which is passed to all other functions.
//
// int skip_init:
// If set to 1 STPU_OPEN will open the STPU device and return
// a HEV populated only with the device file handle. _No_ initialization
// operations or communication with the STPU will be performed. This is mostly
// useful for debugging purposes.
// If set to 2 STPU_OPEN will open the STPU device and return
// a fully populated HEV. _No_ other initialization operations or communication
// with the STPU will be performed. This is mostly useful for debugging purposes.
void* STPU_OPEN (char* stpu_dev_str, int skip_init);

// Close device and cleanup memory
int STPU_CLOSE(void* stpu_hev_vp);

// This task prints out the contents of the entire Hardware Extension Vector (HEV) struct.
void DUMP_HEV_PCIE (void* stpu_hev_vp);

// Initializes all neuron parameters to have sane values via the LOAD_NCL_PCIE
// method. This prevents unused neurons from firing sporadically and cluttering
// the dump output. This function is automatically called one time by STPU_OPEN.
// Resulting global neuron configuration:
// NrnThr = Max Possible Value
// NrnMin = Min Possible Value
// NrnLkg = 0
int INIT_ALL_NRNS (void* stpu_hev_vp);

// Get number of contexts available in the White Matter Memory (WMM)
int GET_NUM_WMM_CTX (void* stpu_hev_vp);

// Get total number of neurons in the system
int GET_NUM_NRNS_SYS (void* stpu_hev_vp);

// Get depth of Context Sequence Memory (CSM)
int GET_CSM_DEPTH (void* stpu_hev_vp);

// Get physical depth of Temporal Stack
int GET_TMP_STK_PDP (void* stpu_hev_vp);

// Get number of Parallel Ingress Columns (PICOLS)
int GET_NUM_PICOLS (void* stpu_hev_vp);

// Get number of Output Neurons Per Ingress Column (ONPIC)
int GET_NUM_ONPIC (void* stpu_hev_vp);

// Get number of Lines Per Input Neuron (LPIN)
int GET_NUM_LPIN (void* stpu_hev_vp);

// This task sends the Neuronal Array Global Reset (NAGR) command to the STPU.
// CmdNum: Number of temporal stack locations to clear (usually this would be
// set to the depth of the temporal stack in order to perform a complete system
// reset).
int NAGR_STPU_PCIE (void* stpu_hev_vp, int CmdNum);

// This task sends the Spike Train Reporting Set (STRS) command to the STPU.
// CmdNum: 0=Disable spike train logging
//         1=Enable spike train logging
int STRS_STPU_PCIE (void* stpu_hev_vp, int CmdNum);

// This task sends the Wave Mode Set (WVMS) command to the STPU.
// CmdNum: 0=Disable wave mode
//         1=Enable wave mode
int WVMS_STPU_PCIE (void* stpu_hev_vp, int CmdNum);

// This task takes a Neuron Configuration List (NCL) in a pre-defined format
// and loads the contents into the STPU neuronal array using the STPU's external
// PCIE interface. It also calculates the 32-bit CRC for the entire NCL and sends
// it along with the NCVL command. The STPU response is checked to ensure that
// the STPU calculated the same CRC value.
// ncl_len = Length of Neuron Configuration List (NCL)
// nrn_num_off = Neuron Number Offset
// The specific neurons programmed as a result of calling this function will be
// neurons 'nrn_num_off' through 'nrn_num_off+ncl_len-1'.
int LOAD_NCL_PCIE (Neuron_Loader_Types::T_NctFl* ncl_arg, int ncl_len, int nrn_num_off, void* stpu_hev_vp);

// This task takes a Neuron Configuration List (NCL) file in a pre-defined format
// and loads the contents into the STPU neuronal array using the STPU's external
// PCIE interface. It also calculates the 32-bit CRC for the entire NCL and sends
// it along with the NCVL command. The STPU response is checked to ensure that
// the STPU calculated the same CRC value.
// nrn_num_off = Neuron Number Offset
// The specific neurons programmed as a result of calling this function will be
// neurons 'nrn_num_off' through 'nrn_num_off+<NCL_LENGTH>-1'.
int LOAD_NCL_FILE_PCIE (char* ncl_fname_arg, int nrn_num_off, void* stpu_hev_vp);

// This task takes an array of WMLs for a specified Context and Input Neuron in
// and loads the contents into the STPU's White Matter Memory (WMM) using the STPU's
// external PCIE interface.
int LOAD_WMLS_PCIE (White_Matter_Types::T_Wme** wmls, int CtxNum, int InpNrnNum, int NumLines, int LineOfs, void* stpu_hev_vp, int do_verify);

// This task returns an array of WMLs from the STPU's White Matter Memory (WMM)
// for a specified Context and Input Neuron using the STPU's PCIE interface.
//
// disp_dmp = set non-zero to also display dump to terminal (useful for debugging)
White_Matter_Types::T_Wme** DUMP_WMLS_PCIE (int CtxNum, int InpNrnNum, int NumLines, int LineOfs, int disp_dmp, void* stpu_hev_vp);

// This task takes a White Matter Content (WMC) file in a pre-defined format and
// loads the contents into the STPU's White Matter Memory (WMM) using the STPU's
// external PCIE interface.
int LOAD_WMC_FILE_PCIE (char* wmc_fname_arg, void* stpu_hev_vp);

// This task takes a White Matter Content (WMC) file in a pre-defined format and
// converts the contents into a wmc_load.h HDR file and a WMC file in the same
// format as the input which can be used with the stpu_loader application. The
// CI_* arguments specify the input WMC file format. The CO_* arguments specify
// the input WMC file format.
int CONV_WMC_FILE_PCIE (char* wmc_in_fname_arg, int CI_WMM_NUM_CTX, int CI_NUM_NRNS_SYS, int CI_WMM_LPIN, int CI_NUM_PICOLS,
char* wmc_out_fname_arg, char* hdr_out_fname_arg, int CO_WMM_NUM_CTX, int CO_NUM_NRNS_SYS, int CO_WMM_LPIN, int CO_NUM_PICOLS);

// This task takes a Context Sequence Program (CSP) in a pre-defined format
// and loads the contents into the STPU neuronal array using the STPU's external
// PCIE interface. It also calculates the 32-bit CRC for the entire CSP and sends
// it along with the CSPL command. The STPU response is checked to ensure that
// the STPU calculated the same CRC value.
int LOAD_CSP_PCIE (Ctx_Seq_Mem_Types::T_Cse* csp_arg, int csp_len, void* stpu_hev_vp);

// This task takes a Context Sequence Program (CSP) file in a pre-defined format
// and loads the contents into the STPU neuronal array using the STPU's external
// PCIE interface. It also calculates the 32-bit CRC for the entire CSP and sends
// it along with the CSPL command. The STPU response is checked to ensure that
// the STPU calculated the same CRC value.
int LOAD_CSP_FILE_PCIE (char* csp_fname_arg, void* stpu_hev_vp);

// This function takes a list/stream of BWPs and sends it to the STPU over PCIE.
int LOAD_BWPL_PCIE (Ctrl_And_Stat_Types::T_BwpFl* bwpl_arg, int CmdNum, void* stpu_hev_vp);

// This task takes a Brain Wave Stream (BWS) file in a pre-defined format
// and loads the contents into the STPU using the STPU's external PCIE
// interface.
int LOAD_BWS_FILE_PCIE (char* bws_fname_arg, char* dmp_fname_arg, void* stpu_hev_vp);

// This task sends the EXEC command to the STPU using the STPU's external PCIE
// interface and dumps any fired neurons to the T_Dmp* dmpl_act array.
// dmpl_exp: Expected dump list (may be NULL if CmdNum is -1 or 0)
// CmdNum: Any integer value >= -1 (see HAL for more information)
// dmpl_act_len: Length of dump list that is returned
// sth_list_out: Pointer to spike train history - will be NULL if spike train history is disabled
// sth_num_rnds: Number of 32-LIF rounds stored in sth_list_out
// Returns: Pointer to Actual Dump List (must be free'd by client application) - will be NULL if spike train history logging is enabled
Ctrl_And_Stat_Types::T_Dmp* LOAD_EXEC_PCIE (Ctrl_And_Stat_Types::T_Dmp* dmpl_exp, int CmdNum, int* dmpl_act_len,
  uint32_t** sth_list_out, int* sth_num_rnds, void* stpu_hev_vp);

// This task sends the EXEC command to the STPU using the STPU's external PCIE
// interface and dumps any fired neurons at the end of CSP to a file.
// dmpl_exp: Expected Dump List
// CmdNum: Any integer value >= -1 (see HAL for more information)
int LOAD_EXEC_FILE_PCIE (Ctrl_And_Stat_Types::T_Dmp* dmpl_exp, int CmdNum, char* dmp_fname_arg, void* stpu_hev_vp);

#endif // _STPU_LIB_H_

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// OFFICIAL USE ONLY /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

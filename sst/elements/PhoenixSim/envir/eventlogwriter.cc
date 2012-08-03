//=========================================================================
// EVENTLOGWRITER.CC - part of
//                  OMNeT++/OMNEST
//           Discrete System Simulation in C++
//
//  This is a generated file -- do not modify.
//
//=========================================================================


#include "eventlogwriter.h"
#include "common/stringutil.h"
#include "cconfigoption.h"
#include "csimulation.h"
#include "cmodule.h"
#include "cexception.h"

#ifdef CHECK
#undef CHECK
#endif
#define CHECK(fprintf)    if (fprintf<0) throw cRuntimeError("Cannot write event log file, disk full?");

#define LL    INT64_PRINTF_FORMAT

void EventLogWriter::recordLogLine(FILE *f, const char *s, int n)
{
    CHECK(fprintf(f, "- "));
    CHECK(fwrite(s, 1, n, f));
}

void EventLogWriter::recordSimulationBeginEntry_v_rid(FILE *f, int version, const char * runId)
{
    CHECK(fprintf(f, "SB v %d rid %s\n", version, QUOTE(runId)));
}

void EventLogWriter::recordSimulationEndEntry(FILE *f)
{
    CHECK(fprintf(f, "SE\n"));
}

void EventLogWriter::recordBubbleEntry_id_txt(FILE *f, int moduleId, const char * text)
{
    CHECK(fprintf(f, "BU id %d txt %s\n", moduleId, QUOTE(text)));
}

void EventLogWriter::recordModuleMethodBeginEntry_sm_tm_m(FILE *f, int fromModuleId, int toModuleId, const char * method)
{
    CHECK(fprintf(f, "MB sm %d tm %d m %s\n", fromModuleId, toModuleId, QUOTE(method)));
}

void EventLogWriter::recordModuleMethodEndEntry(FILE *f)
{
    CHECK(fprintf(f, "ME\n"));
}

void EventLogWriter::recordModuleCreatedEntry_id_c_t_n(FILE *f, int moduleId, const char * moduleClassName, const char * nedTypeName, const char * fullName)
{
    CHECK(fprintf(f, "MC id %d c %s t %s n %s\n", moduleId, QUOTE(moduleClassName), QUOTE(nedTypeName), QUOTE(fullName)));
}

void EventLogWriter::recordModuleCreatedEntry_id_c_t_pid_n_cm(FILE *f, int moduleId, const char * moduleClassName, const char * nedTypeName, int parentModuleId, const char * fullName, bool compoundModule)
{
    CHECK(fprintf(f, "MC id %d c %s t %s", moduleId, QUOTE(moduleClassName), QUOTE(nedTypeName)));
    if (parentModuleId!=-1)
        CHECK(fprintf(f, " pid %d", parentModuleId));
    CHECK(fprintf(f, " n %s", QUOTE(fullName)));
    if (compoundModule!=false)
        CHECK(fprintf(f, " cm %d", compoundModule));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordModuleDeletedEntry_id(FILE *f, int moduleId)
{
    CHECK(fprintf(f, "MD id %d\n", moduleId));
}

void EventLogWriter::recordModuleReparentedEntry_id_p(FILE *f, int moduleId, int newParentModuleId)
{
    CHECK(fprintf(f, "MR id %d p %d\n", moduleId, newParentModuleId));
}

void EventLogWriter::recordGateCreatedEntry_m_g_n_o(FILE *f, int moduleId, int gateId, const char * name, bool isOutput)
{
    CHECK(fprintf(f, "GC m %d g %d n %s o %d\n", moduleId, gateId, QUOTE(name), isOutput));
}

void EventLogWriter::recordGateCreatedEntry_m_g_n_i_o(FILE *f, int moduleId, int gateId, const char * name, int index, bool isOutput)
{
    CHECK(fprintf(f, "GC m %d g %d n %s", moduleId, gateId, QUOTE(name)));
    if (index!=-1)
        CHECK(fprintf(f, " i %d", index));
    CHECK(fprintf(f, " o %d\n", isOutput));
}

void EventLogWriter::recordGateDeletedEntry_m_g(FILE *f, int moduleId, int gateId)
{
    CHECK(fprintf(f, "GD m %d g %d\n", moduleId, gateId));
}

void EventLogWriter::recordConnectionCreatedEntry_sm_sg_dm_dg(FILE *f, int sourceModuleId, int sourceGateId, int destModuleId, int destGateId)
{
    CHECK(fprintf(f, "CC sm %d sg %d dm %d dg %d\n", sourceModuleId, sourceGateId, destModuleId, destGateId));
}

void EventLogWriter::recordConnectionDeletedEntry_sm_sg(FILE *f, int sourceModuleId, int sourceGateId)
{
    CHECK(fprintf(f, "CD sm %d sg %d\n", sourceModuleId, sourceGateId));
}

void EventLogWriter::recordConnectionDisplayStringChangedEntry_sm_sg_d(FILE *f, int sourceModuleId, int sourceGateId, const char * displayString)
{
    CHECK(fprintf(f, "CS sm %d sg %d d %s\n", sourceModuleId, sourceGateId, QUOTE(displayString)));
}

void EventLogWriter::recordModuleDisplayStringChangedEntry_id_d(FILE *f, int moduleId, const char * displayString)
{
    CHECK(fprintf(f, "MS id %d d %s\n", moduleId, QUOTE(displayString)));
}

void EventLogWriter::recordEventEntry_e_t_m_msg(FILE *f, eventnumber_t eventNumber, simtime_t simulationTime, int moduleId, long messageId)
{
    CHECK(fprintf(f, "\nE # %"LL"d t %s m %d msg %ld\n", eventNumber, SIMTIME_STR(simulationTime), moduleId, messageId));
}

void EventLogWriter::recordEventEntry_e_t_m_ce_msg(FILE *f, eventnumber_t eventNumber, simtime_t simulationTime, int moduleId, eventnumber_t causeEventNumber, long messageId)
{
    CHECK(fprintf(f, "\nE # %"LL"d t %s m %d", eventNumber, SIMTIME_STR(simulationTime), moduleId));
    if (causeEventNumber!=-1)
        CHECK(fprintf(f, " ce %"LL"d", causeEventNumber));
    CHECK(fprintf(f, " msg %ld\n", messageId));
}

void EventLogWriter::recordCancelEventEntry_id(FILE *f, long messageId)
{
    CHECK(fprintf(f, "CE id %ld\n", messageId));
}

void EventLogWriter::recordCancelEventEntry_id_pe(FILE *f, long messageId, eventnumber_t previousEventNumber)
{
    CHECK(fprintf(f, "CE id %ld", messageId));
    if (previousEventNumber!=-1)
        CHECK(fprintf(f, " pe %"LL"d", previousEventNumber));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordBeginSendEntry_id_tid_c_n(FILE *f, long messageId, long messageTreeId, const char * messageClassName, const char * messageFullName)
{
    CHECK(fprintf(f, "BS id %ld tid %ld c %s n %s\n", messageId, messageTreeId, QUOTE(messageClassName), QUOTE(messageFullName)));
}

void EventLogWriter::recordBeginSendEntry_id_tid_eid_etid_c_n_pe_k_p_l_er_d(FILE *f, long messageId, long messageTreeId, long messageEncapsulationId, long messageEncapsulationTreeId, const char * messageClassName, const char * messageFullName, eventnumber_t previousEventNumber, short messageKind, short messagePriority, int64 messageLength, bool hasBitError, const char * detail)
{
    CHECK(fprintf(f, "BS id %ld tid %ld", messageId, messageTreeId));
    if (messageEncapsulationId!=-1)
        CHECK(fprintf(f, " eid %ld", messageEncapsulationId));
    if (messageEncapsulationTreeId!=-1)
        CHECK(fprintf(f, " etid %ld", messageEncapsulationTreeId));
    CHECK(fprintf(f, " c %s n %s", QUOTE(messageClassName), QUOTE(messageFullName)));
    if (previousEventNumber!=-1)
        CHECK(fprintf(f, " pe %"LL"d", previousEventNumber));
    if (messageKind!=0)
        CHECK(fprintf(f, " k %d", messageKind));
    if (messagePriority!=0)
        CHECK(fprintf(f, " p %d", messagePriority));
    if (messageLength!=0)
        CHECK(fprintf(f, " l %"LL"d", messageLength));
    if (hasBitError!=false)
        CHECK(fprintf(f, " er %d", hasBitError));
    if (detail!=NULL)
        CHECK(fprintf(f, " d %s", QUOTE(detail)));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordEndSendEntry_t(FILE *f, simtime_t arrivalTime)
{
    CHECK(fprintf(f, "ES t %s\n", SIMTIME_STR(arrivalTime)));
}

void EventLogWriter::recordEndSendEntry_t_is(FILE *f, simtime_t arrivalTime, bool isReceptionStart)
{
    CHECK(fprintf(f, "ES t %s", SIMTIME_STR(arrivalTime)));
    if (isReceptionStart!=false)
        CHECK(fprintf(f, " is %d", isReceptionStart));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordSendDirectEntry_sm_dm_dg(FILE *f, int senderModuleId, int destModuleId, int destGateId)
{
    CHECK(fprintf(f, "SD sm %d dm %d dg %d\n", senderModuleId, destModuleId, destGateId));
}

void EventLogWriter::recordSendDirectEntry_sm_dm_dg_pd_td(FILE *f, int senderModuleId, int destModuleId, int destGateId, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    CHECK(fprintf(f, "SD sm %d dm %d dg %d", senderModuleId, destModuleId, destGateId));
    if (propagationDelay!=0)
        CHECK(fprintf(f, " pd %s", SIMTIME_STR(propagationDelay)));
    if (transmissionDelay!=0)
        CHECK(fprintf(f, " td %s", SIMTIME_STR(transmissionDelay)));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordSendHopEntry_sm_sg(FILE *f, int senderModuleId, int senderGateId)
{
    CHECK(fprintf(f, "SH sm %d sg %d\n", senderModuleId, senderGateId));
}

void EventLogWriter::recordSendHopEntry_sm_sg_pd_td(FILE *f, int senderModuleId, int senderGateId, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    CHECK(fprintf(f, "SH sm %d sg %d", senderModuleId, senderGateId));
    if (propagationDelay!=0)
        CHECK(fprintf(f, " pd %s", SIMTIME_STR(propagationDelay)));
    if (transmissionDelay!=0)
        CHECK(fprintf(f, " td %s", SIMTIME_STR(transmissionDelay)));
    CHECK(fprintf(f, "\n"));
}

void EventLogWriter::recordDeleteMessageEntry_id(FILE *f, int messageId)
{
    CHECK(fprintf(f, "DM id %d\n", messageId));
}

void EventLogWriter::recordDeleteMessageEntry_id_pe(FILE *f, int messageId, eventnumber_t previousEventNumber)
{
    CHECK(fprintf(f, "DM id %d", messageId));
    if (previousEventNumber!=-1)
        CHECK(fprintf(f, " pe %"LL"d", previousEventNumber));
    CHECK(fprintf(f, "\n"));
}


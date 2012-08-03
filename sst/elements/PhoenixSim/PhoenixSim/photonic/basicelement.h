///brief Base class for all photonic plane devices.
/**
 * The BasicElement pure virtual base class provides the fundamental functionality
 * for all photonic plane devices.
 */

#ifndef BASICELEMENT_H_
#define BASICELEMENT_H_

#include <omnetpp.h>
#include "messages_m.h"
#include "sim_includes.h"
#include "packetstat.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include "statistics.h"
#include "phylayer.h"

#ifdef ENABLE_HOTSPOT
#include "thermalmodel.h"
#endif

#define MAX_INSERTION_LOSS 2000;

using namespace std;
using namespace PhyLayer;

/**
 * \brief Port direction indicator.
 *
 * Used to declare the correct logical connection directions. This usage is possibly depricated.
 */
enum PortDir
{
	in, ///< inputs
	out, ///< outputs
	inout, ///< bidirectional
	none ///< no specification
};

class BasicElement : public cSimpleModule
{
	public:
		BasicElement();
		virtual ~BasicElement();

	protected:
		int numOfPorts;	///< The number of bi-directional ports on the device. (Should be specified in the NED file)
		vector<int> gateIdIn; ///< Vector for tracking OMNeT++ input gate IDs. (Should be specified in the NED file)
		vector<int> gateIdOut; ///< Vector for tracking OMNeT++ output gate IDs. (Should be specified in the NED file)
		vector<PortDir> portType; ///< Specifies the type of port directionality, possibly depricated. (Should be specified in the NED file)
		vector<int> routingTable; ///< Routing table for identifying the logical route an optical message should take.
		vector< map<double,simtime_t> > lastMessageTimestamp; ///< Tracks the last TXstart message that arrived in the device, according to inputgate and wavelength.
		vector< map<double,PacketStat*> > currPackets; ///< Local instantiations of PacketStat for messages that are currently propagating through the device.
		vector< map<double,PacketStat*> > currOutputPackets; ///copies of packetStats found in currPackets, organized according to destination

		string groupLabel;

		double SizeWidth;
        double SizeHeight;
		double PositionLeftX;
        double PositionBottomY;

		/// OMNeT++ initializer.
		virtual void initialize();

		/// OMNeT++ message handler.
		virtual void handleMessage(cMessage *msg);

		/// OMNeT++ deconstructor.
		virtual void finish();

		/// Initializes BasicElement specific member variables.
		void InitializeBasicElement();

		/// Device specific initialization. (Implemented by user)
		virtual void Setup() = 0;

		/// Returns the logically routed port
		int Route(int index);

		/// Sets the values in routingTable vector.
		virtual void SetRoutingTable() = 0;

		/// Function is called to determine routing.
		virtual int AccessRoutingTable(int index, PacketStat *ps = NULL);

		/// Checks for whether *msg points to a PhotonicMessage, if so, it gets passed to HandlePhotonicMessage.
		bool CheckAndHandlePhotonicMessage(cMessage *msg);

		/// Routes, applies physical-layer effects, and sends off *msg.
		void HandlePhotonicMessage(PhotonicMessage *msg, int index);

		/// Returns total insertion loss due to all loss types.
		/**
		 * Returned value is measured in units of dB.
		 */
		virtual double GetInsertionLoss(int indexIn, int indexOut, double wavelength = 0);

		/// Returns insertion loss due to propagation in waveguides.
		/**
		 * Returned value is measured in units of dB.
		 */
		virtual double GetPropagationLoss(int indexIn, int indexOut, double wavelength = 0);

		/// Returns insertion loss due to bends in waveguides.
		/**
		 * Returned value is measured in units of dB.
		 */
		virtual double GetBendingLoss(int indexIn, int indexOut, double wavelength = 0);

		/// Returns insertion loss due to waveguide crossings.
		/**
		 * Returned value is measured in units of dB.
		 */
		virtual double GetCrossingLoss(int indexIn, int indexOut, double wavelength = 0);

		/// Returns the gain of the device (negative loss).
		/**
		 * Postcondition: Returned value is equal to or less than zero.
		 */
		virtual double GetGain(int indexIn, int indexOut, double wavelength = 0);

		/// Returns the latency of the device.
		/**
		 * Returned value is measured in units of seconds.
		 */
		virtual double GetLatency(int indexIn, int indexOut);

		/// Applies insertion loss to the message's PacketStat pointed to by *localps.
		virtual void ApplyInsertionLoss(PacketStat *localps, int indexIn, int indexOut, double wavelength = 0);

		/// Applies crosstalk to the message's PacketStat pointed to by *localps.
		virtual void ApplyCrosstalk(PacketStat *localps, int indexIn, int indexOut, double wavelength = 0);

		/// Returns the name of the device.
		string GetName();
	private:
		PacketStat *currPacketStat;
		vector<bool> inNoise;
};

#endif /* BASICELEMENT_H_ */

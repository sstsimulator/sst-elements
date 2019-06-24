
#ifndef _H_SHOGUN_NIC
#define _H_SHOGUN_NIC

#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/link.h>
#include <sst/core/unitAlgebra.h>

#include "shogun_event.h"
#include "shogun_q.h"

using namespace SST;
using namespace SST::Interfaces;

namespace SST {
namespace Shogun {

    class ShogunNIC : public SST::Interfaces::SimpleNetwork {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT(
            ShogunNIC,
            "shogun",
            "ShogunNIC",
            SST_ELI_ELEMENT_VERSION(1, 0, 0),
            "Shogun X-Bar Interface for Memory Crossbars",
            "SST::Interfaces::SimpleNetwork");

        SST_ELI_DOCUMENT_PARAMS(
            { "verbose", "Level of output verbosity, higher is more output, 0 is	no output", 0 })

        ShogunNIC(SST::Component* component, Params& params);
        ~ShogunNIC();

        /** Second half of building the interface.
        Initialize network interface
        @param portName - Name of port to connect to
        @param link_bw - Bandwidth of the link
        @param vns - Number of virtual networks to be provided
        @param in_buf_size - Size of input buffers (from router)
        @param out_buf_size - Size of output buffers (to router)
     * @return true if the link was able to be configured.
     */
        virtual bool initialize(const std::string& portName, const UnitAlgebra& link_bw,
            int vns, const UnitAlgebra& in_buf_size,
            const UnitAlgebra& out_buf_size) override;

        /**
     		* Sends a network request during the init() phase
     	*/
        virtual void sendInitData(Request* req) override;

        /**
     		* Receive any data during the init() phase.
     		* @see SST::Link::recvInitData()
     	*/
        virtual Request* recvInitData() override;

        /**
     * Sends a network request during untimed phases (init() and
     * complete()).
     * @see SST::Link::sendUntimedData()
     *
     * For now, simply call sendInitData.  Once that call is
     * deprecated and removed, this will become a pure virtual
     * function.  This means that when classes implement
     * SimpleNetwork, they will need to overload both sendUntimedData
     * and sendInitData with identical functionality (or having the
     * Init version call the Untimed version) until sendInitData is
     * removed in SST 9.0.
     */
        virtual void sendUntimedData(Request* req) override
        {
            sendInitData(req);
        }

        /**
     * Receive any data during untimed phases (init() and complete()).
     * @see SST::Link::recvUntimedData()
     *
     * For now, simply call recvInitData.  Once that call is
     * deprecated and removed, this will become a pure virtual
     * function.  This means that when classes implement
     * SimpleNetwork, they will need to overload both recvUntimedData
     * and recvInitData with identical functionality (or having the
     * Init version call the Untimed version) until recvInitData is
     * removed in SST 9.0.
     */
        virtual Request* recvUntimedData() override
        {
            return recvInitData();
        }

        // /**
        //  * Returns a handle to the underlying SST::Link
        //  */
        // virtual Link* getLink(void) const = 0;

        /**
     * Send a Request to the network.
     */
        virtual bool send(Request* req, int vn) override;

        /**
     * Receive a Request from the network.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @param vn Virtual network to receive on
     * @return NULL if nothing is available.
     * @return Pointer to a Request response (that should be deleted)
     */
        virtual Request* recv(int vn) override;

        virtual void setup() override;
        virtual void init(unsigned int UNUSED(phase)) override;
        virtual void complete(unsigned int UNUSED(phase)) override;
        virtual void finish() override;

        /**
     * Checks if there is sufficient space to send on the specified
     * virtual network
     * @param vn Virtual network to check
     * @param num_bits Minimum size in bits required to have space
     * to send
     * @return true if there is space in the output, false otherwise
     */
        virtual bool spaceToSend(int vn, int num_bits) override;

        /**
     * Checks if there is a waiting network request request pending in
     * the specified virtual network.
     * @param vn Virtual network to check
     * @return true if a network request is pending in the specified
     * virtual network, false otherwise
     */
        virtual bool requestToReceive(int vn) override;

        /**
     * Registers a functor which will fire when a new request is
     * received from the network.  Note, the actual request that
     * was received is not passed into the functor, it is only a
     * notification that something is available.
     * @param functor Functor to call when request is received
     */
        virtual void setNotifyOnReceive(SimpleNetwork::HandlerBase* functor) override;

        /**
     * Registers a functor which will fire when a request is
     * sent to the network.  Note, this only tells you when data
     * is sent, it does not guarantee any specified amount of
     * available space.
     * @param functor Functor to call when request is sent
     */
        virtual void setNotifyOnSend(SimpleNetwork::HandlerBase* functor) override;

        /**
     * Check to see if network is initialized.  If network is not
     * initialized, then no other functions other than init() can
     * can be called on the interface.
     * @return true if network is initialized, false otherwise
     */
        virtual bool isNetworkInitialized() const override;

        /**
     * Returns the endpoint ID.  Cannot be called until after the
     * network is initialized.
     * @return Endpoint ID
     */
        virtual nid_t getEndpointID() const override;

        /**
     * Returns the final BW of the link managed by the simpleNetwork
     * instance.  Cannot be called until after the network is
     * initialized.
     * @return Link bandwidth of associated link
     */
        virtual const UnitAlgebra& getLinkBW() const override;

    private:
        SST::Output* output;
        SST::Link* link;
        nid_t netID;

        SimpleNetwork::HandlerBase* onSendFunctor;
        SimpleNetwork::HandlerBase* onRecvFunctor;

        ShogunQueue<Request*>* reqQ;
        int remote_input_slots;
        int port_count;

        void recvLinkEvent(SST::Event* ev);
        void reconfigureNIC(ShogunInitEvent* initEv);

        std::vector<Request*> initReqs;

        UnitAlgebra bw;
    };

}
}

#endif

#ifndef  IRIS_NIC_BUFFER_H
#define  IRIS_NIC_BUFFER_H

#include	"irisNicFlit.h"
#include	<deque>
#include	<vector>
#include	<assert.h>

class irisNicBuffer
{
    public:
    	irisNicBuffer (unsigned vcs);
        ~irisNicBuffer ();                             /* desstructor */
        void push( unsigned ch,  Flit* f );
        Flit* pull(unsigned ch);
        Flit* pull(unsigned ch, uint src);
        Flit* peek(unsigned ch);
        uint get_occupancy( uint ch ) const;
        bool is_channel_full( uint ch ) const;
        bool is_empty( uint ch ) const;

        std::string toString() const;

    protected:

    private:
        std::vector < std::deque<Flit*> > buffers;

}; /* -----  end of class irisNicBuffer  ----- */


#endif // IRIS_NIC_BUFFER_H

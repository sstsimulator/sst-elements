#include	"irisNicBuffer.h"

//! @param \c vcs  No. of virtual channels; internally vcs buffers are created.
//! @param \c size  Max. size of each buffer.
irisNicBuffer::irisNicBuffer (unsigned vcs) : buffers(vcs)
{
}		/* -----  end of function irisNicBuffer::irisNicBuffer  ----- */

irisNicBuffer::~irisNicBuffer ()
{
}

void
irisNicBuffer::push (unsigned ch, Flit* f )
{
    buffers[ch].push_back(f);
    assert(buffers[ch].size() > 0);
    return;
}		/* -----  end of function irisNicBuffer::push  ----- */

Flit*
irisNicBuffer::pull (unsigned ch)
{
    assert(ch < buffers.size() );
    assert(buffers[ch].size() != 0);

    Flit* f = buffers[ch].front();
    buffers[ch].pop_front();
    return f;
}		/* -----  end of function irisNicBuffer::pull  ----- */

Flit*
irisNicBuffer::pull (unsigned ch, uint src)
{
    assert(ch < buffers.size() );
    assert(buffers[ch].size() != 0);
    Flit * f;
    
    for(std::deque<Flit*>::iterator it = buffers[ch].begin();
        it != buffers[ch].end(); it++) {
        if((*it)->src_id == src) {
            f = *it;
            buffers[ch].erase(it);
            return f;
        }
    }
    return NULL;  
}

/*!
 * ===  FUNCTION  ======================================================================
 *         Name:  peek
 *  Description:  Uses the pull channel. Make sure to set it before peek is
 *  called. Normal usage is to get a pointer to the head of the buffer without
 *  actually pulling it out from the buffer
 * =====================================================================================
 */
Flit*
irisNicBuffer::peek (unsigned ch)
{
    if( ch > buffers.size() || buffers[ch].size() == 0)
    {
        fprintf( stdout, "\nERROR: **Invalid pull channel");
        exit(1);
    }
    Flit* f = buffers[ch].front();
    return f;
}		/* -----  end of function irisNicBuffer::pull  ----- */

uint
irisNicBuffer::get_occupancy ( uint ch ) const
{
    return buffers[ch].size();
}		/* -----  end of function irisNicBuffer::get_occupancy  ----- */



bool
irisNicBuffer::is_empty (uint ch ) const
{
    return buffers[ch].empty();
}		/* -----  end of function irisNicBuffer::empty  ----- */

std::string
irisNicBuffer::toString () const
{
    std::stringstream str;
    str << "irisNicBuffer"
        << "\t No of buffers: " << buffers.size() << "\n";
    for( uint i=0; i<buffers.size() && !buffers[i].empty(); i++)
        str << buffers[i].front()->toString();
    return str.str();
}		/* -----  end of function irisNicBuffer::toString  ----- */


/*
 * =====================================================================================
//       Filename:  genericVca.cc
 * =====================================================================================
 */
#ifndef  _GENERICVCA_CC_INC
#define  _GENERICVCA_CC_INC

#include	"genericVca.h"

GenericVca::GenericVca ()
{
    requesting_inputs.resize(r_param.ports);
    current_winners.resize(r_param.ports);
    last_winner.resize(r_param.ports);
    ovc_tokens.resize(r_param.ports);

    // Init the tokens to all available output r_param.vcs
    for ( std::vector< std::vector<uint16_t> >::iterator it=ovc_tokens.begin(); 
          it != ovc_tokens.end(); ++it)
        for( uint16_t token=0; token<r_param.vcs; ++token)
            it->push_back(token);

    for ( std::vector< std::vector< VCA_unit> >::iterator it=current_winners.begin(); 
          it!=current_winners.end(); ++it)
        it->resize(r_param.ports*r_param.vcs);

    for ( std::vector< std::vector< VCA_unit> >::iterator it=requesting_inputs.begin(); 
          it!=requesting_inputs.end(); ++it)
        it->resize(r_param.ports*r_param.vcs);

    for ( std::vector< std::vector< uint16_t> >::iterator it=last_winner.begin(); 
          it!=last_winner.end(); ++it)
        it->resize(r_param.vcs);

    return ;
}

// Head flits in the IB can request for an out vc every cycle till it is
// assigned an output channel by the arbiter. If there are multiple head flits
// queued in the input buffer and the one at the head of the queue is assigned a
// channel. The one next in line can request but will not be allowed to ask for
// the grant till the tail flit of the previous packet leaves the router. (The
// freeing of the request in the matrix can be done when the tail flit is pushed
// into the output buffer for output buffered switches)
// TODO: Modify for output buffered routers
// The function returns true if the requestor was allowed to set his request in
// the requesting matrix/ false otherwise to say sorry request next cycle.
bool
GenericVca::request ( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc )
{
    if ( requesting_inputs.at(op).at(ip+r_param.ports*ivc).is_valid == false ){
        requesting_inputs.at(op).at(ip+r_param.ports*ivc).is_valid = true;
        requesting_inputs.at(op).at(ip+r_param.ports*ivc).in_port = ip;
        requesting_inputs.at(op).at(ip+r_param.ports*ivc).in_vc = ivc;
        requesting_inputs.at(op).at(ip+r_param.ports*ivc).out_port = op;
        requesting_inputs.at(op).at(ip+r_param.ports*ivc).out_vc = ovc;
    }

    return requesting_inputs.at(op).at(ip+r_param.ports*ivc).is_valid;

}		/* -----  end of method GenericVca::request  ----- */

bool
GenericVca::is_empty ( void ) const
{
    for ( uint16_t p=0; p<r_param.ports; ++p )
        if (!is_empty(p))
            return false;

    return true;
}		/* -----  end of method GenericVca::is_empty  ----- */

bool
GenericVca::is_empty ( uint16_t port ) const
{
    for ( uint16_t i=0; i<r_param.ports*r_param.vcs; ++i )
        if (requesting_inputs.at(port).at(i).is_valid)
            return false;

    return true;
}		/* -----  end of method GenericVca::is_empty  ----- */

uint16_t
GenericVca::no_requestors ( uint16_t op )
{
    uint16_t no = 0;
    for ( uint16_t j=0; j<r_param.ports*r_param.vcs; j++)
        if ( requesting_inputs.at(op).at(j).is_valid )
            no++;

    return no;
}		/* -----  end of method GenericVca::no_requestors  ----- */

void
GenericVca::pick_winner ( void )
{
    if ( !is_empty() )
    {
        for ( uint16_t p=0; p<r_param.ports; ++p)
            for ( uint16_t v=0; v<r_param.vcs; ++v)
            {
                // check if this vc is available in the token list
                std::vector<uint16_t>::iterator it;
                it = find ( ovc_tokens.at(p).begin(), ovc_tokens.at(p).end(), v);
                if ( it != ovc_tokens.at(p).end() )
                {
                    /*  For DEBUG
                        printf("\nIter for port %d vc %d tokens:-",i,j);
                        for ( uint ss=0; ss<ovc_tokens.size(); ss++)
                        printf(" %d|",ovc_tokens[i][ss]);
                        printf(" reqs:-");
                        for ( uint ss=0; ss<r_param.ports*r_param.vcs; ss++)
                        printf(" %d-%d",(int)requesting_inputs[i][ss].is_valid, requesting_inputs[i][ss].out_vc);
                     */

                    uint16_t start_index=last_winner.at(p).at(v)+1;
                    bool found_winner = false;
                    for ( uint16_t k=start_index; k<r_param.ports*r_param.vcs; ++k)
                        if ( requesting_inputs.at(p).at(k).is_valid 
                             && requesting_inputs.at(p).at(k).out_vc == v )
                        {

                            last_winner.at(p).at(v) = k;
                            found_winner = true;
                            break;
                        }

                    if ( !found_winner )
                    {
                        for ( uint16_t k=0; k<start_index; ++k)
                            if ( requesting_inputs.at(p).at(k).is_valid 
                                 && requesting_inputs.at(p).at(k).out_vc == v )
                            {
                                last_winner.at(p).at(v) = k;
                                break;
                            }

                    }

                    // assign the channel and remove it from the token
                    // list
                    ovc_tokens.at(p).erase(it);

                    VCA_unit winner;
                    winner.out_vc = v;
                    winner.out_port = p;
                    winner.in_port = (int)(last_winner.at(p).at(v)%r_param.ports);
                    winner.in_vc= (int)(last_winner.at(p).at(v)/r_param.ports);

                    current_winners.at(p).push_back(winner);
                }
            }
    }
    return ;
}		/* -----  end of method GenericVca::pick_winner  ----- */

bool
GenericVca::is_requested ( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc ) const
{
    return requesting_inputs.at(op).at(ip+r_param.ports*ivc).is_valid ;
}		/* -----  end of method GenericVca::is_requested  ----- */

void
GenericVca::clear_winner ( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc )
{
    requesting_inputs.at(op).at(ip+r_param.ports*ivc).is_valid =false;

    // Erase the winner from the current winners list
    std::vector< std::vector<VCA_unit> >::iterator it;
    for( it=current_winners.begin(); it!=current_winners.end(); ++it)
    {
        std::vector<VCA_unit>::iterator it2;
        for( it2=it->begin(); it2!=it->end(); ++it2)
            if (it2->in_port == ip && it2->in_vc == ivc
                && it2->out_port == op && it2->out_vc == ovc )
                current_winners.at(op).erase(it2);
    }

    // Return the token to the pool
    ovc_tokens.at(op).push_back(ovc);
    return ;
}		/* -----  end of method GenericVca::clear_winner  ----- */

#endif   /* ----- #ifndef _GENERICVCA_CC_INC  ----- */


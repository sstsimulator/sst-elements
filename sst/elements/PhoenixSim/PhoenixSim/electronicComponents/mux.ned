//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package electronicComponents;

simple mux
{
 parameters:
     double O_frequency_cntrl;
        
    gates:
        input in[];

        output out;
}


simple demux
{
    parameters:
        int numOut;
    gates:
        input in;

        output out[numOut];
}


simple inoutConverter
{
 	
 	
 	gates:   
        input in;
        output out;
        
        inout io_out;
        inout io_in;
    
}
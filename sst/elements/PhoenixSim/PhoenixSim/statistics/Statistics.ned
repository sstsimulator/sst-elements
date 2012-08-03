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

package statistics;




simple Statistics
{
    parameters:
     
        string logDirectory;
        string networkName;
        string customInfo;
        
        string debug_filter;
        
        int numOfNodesX;
        
        double STATS_warmup;
        double STATS_cooldown;
        
        
        int O_technology_cntrl;
		int O_trans_type_cntrl;
		double O_voltage_cntrl;
		double O_frequency_cntrl;
		
		int O_technology_data;
		int O_trans_type_data;
		double O_voltage_data;
		double O_frequency_data;
		
		int O_technology_proc;
		int O_trans_type_proc;
		double O_voltage_proc;
		double O_frequency_proc;
     
}
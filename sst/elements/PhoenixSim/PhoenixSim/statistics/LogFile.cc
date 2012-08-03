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

#include "LogFile.h"

LogFile::LogFile(string fname) {
	if (!file.is_open()) {
		file.open(fname.c_str());
		if (!file.is_open()) {
			std::cout << "****** Log file name = " << fname << endl;
			opp_error("Could not open log file");
		}
	}

}

LogFile::~LogFile() {
	groups.clear();
}

void LogFile::close() {
	if (file.is_open()) {
		file.close();
	}
}

void LogFile::write() {
	list<StatGroup*>::iterator it;

	file << "Simulation Time:, " << simTime() << endl;

	for (it = groups.begin(); it != groups.end(); it++) {
		if (!(*it)->empty()) {
			file << (*it)->name << endl;
			(*it)->display(&file);
			file << "\n\n";
		}
	}
}

void LogFile::addGroup(StatGroup* sg) {
	groups.push_back(sg);
}

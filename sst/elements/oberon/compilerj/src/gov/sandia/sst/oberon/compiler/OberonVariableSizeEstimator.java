package gov.sandia.sst.oberon.compiler;

import gov.sandia.sst.oberon.compiler.exp.*;

public class OberonVariableSizeEstimator {

	public static int OberonVariableSizeEstimator(String fileName,
			int lineNo, int colNo, OberonVariableType vType) throws
		OberonIncompatibleTypeException {
		
		switch(vType) {
		case DOUBLE:
		case INTEGER:
			return 8;
			
		case BOOLEAN:
			return OberonCompilerOptions.getInstance().getBytesPerBoolean();
			
		case STRING:
			return 8;
			
		case VOID:
			throw new OberonIncompatibleTypeException(fileName, lineNo, colNo,
					"Cannot estimate size of void expression");
		}
		
		return 8;
	}

}

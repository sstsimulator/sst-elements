package gov.sandia.sst.oberon.compiler;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.TypeNamePair;

import java.util.*;

public class OberonNameMangler {

	public static String getMangleAdditionByTypes(
			String fileName, int lineNo, int colNo,
			Vector<TypeNamePair> typePairs) throws
		OberonIncompatibleTypeException {
		
		String mangle = "$";
		
		for(TypeNamePair nxtPair : typePairs) {
			switch(nxtPair.getType()) {
			case DOUBLE:
				mangle = mangle + "d";
				break;
			case INTEGER:
				mangle = mangle + "i";
				break;
			case BOOLEAN:
				mangle = mangle + "b";
				break;
			case STRING:
				mangle = mangle + "s";
				break;
			case VOID:
				throw new OberonIncompatibleTypeException(fileName, lineNo, colNo,
						"void expressions can not be mangled.");
			}
		}
		
		return mangle;
	}
	
	public static String getMangleAddition(String fileName,
			int lineNo, int colNo, Vector<OberonExpression> expressions) throws
		OberonIncompatibleTypeException {
		
		String mangle = "$";
		
		for(OberonExpression nextObExp : expressions) {
			switch(nextObExp.getExpressionType()) {
			case DOUBLE:
				mangle = mangle + "d";
				break;
			case INTEGER:
				mangle = mangle + "i";
				break;
			case BOOLEAN:
				mangle = mangle + "b";
				break;
			case STRING:
				mangle = mangle + "s";
				break;
			case VOID:
				throw new OberonIncompatibleTypeException(
					fileName,
					lineNo,
					colNo,
					"void expressions are not allowed as a mangle type.");
			}
		}
		
		return mangle;
	}

}

package gov.sandia.sst.oberon.compiler;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.*;
import java.util.*;

public final class OberonFunctionTable {

	private static OberonFunctionTable instance = null;
	
	public static OberonFunctionTable getInstance() {
		if(instance == null)
			instance = new OberonFunctionTable();
		
		return instance;
	}
	
	////////////////////////////////////////////////////////////
	
	private Hashtable<String, FunctionDefinition> functions;
	
	private OberonFunctionTable() {
		functions = new Hashtable<String, FunctionDefinition>();
	}
	
	public OberonVariableType getFunctionType(String fileName,
			int lineNo, int colNo, String mangledName) throws
		OberonIncompatibleTypeException {
		
		if(functions.containsKey(mangledName)) {
			return functions.get(mangledName).getFunctionType();
		} else {
			throw new OberonIncompatibleTypeException(
					fileName, lineNo, colNo,
					"Unable to locate function: " + mangledName
					+ " in function lookup table");
		}
	}
	
	public boolean containsFunctionByMangledName(String mangledName) {
		return functions.containsKey(mangledName);
	}
	
	public void addFunction(FunctionDefinition func) throws OberonIncompatibleTypeException {
		String mangled = func.getFunctionName() +
				OberonNameMangler.getMangleAdditionByTypes(func.getFileName(),
						func.getLineNumber(), func.getColumnNumber(), 
						func.getParameters());
	
		if(functions.containsKey(mangled)) {
			throw new OberonIncompatibleTypeException(func.getFileName(),
					func.getLineNumber(), func.getColumnNumber(),
					"Function: " + func.getFunctionName() + " already exists in table");
		} else {
			functions.put(mangled, func);
		}
	}

	public int countFunctions() {
		return functions.size();
	}
	
	public Vector<FunctionDefinition> getFunctionList() {
		Vector<FunctionDefinition> funcs = new Vector<FunctionDefinition>();
		funcs.addAll(functions.values());
		return funcs;
	}
}

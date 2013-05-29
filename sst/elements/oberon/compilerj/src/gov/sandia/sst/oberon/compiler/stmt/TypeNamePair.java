package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonVariableType;

public class TypeNamePair {

	protected OberonVariableType type;
	protected String name;
	
	public TypeNamePair(
			OberonVariableType type,
			String name) {
		this.name = name;
		this.type = type;
	}
	
	public OberonVariableType getType() {
		return type;
	}
	
	public String getName() {
		return name;
	}

}

package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.visitor.*;

public abstract class OberonStatement implements OberonVisitorTarget {

	protected int lineno;
	protected int colno;
	
	public OberonStatement(int lineno, int colno) {
		this.lineno = lineno;
		this.colno = colno;
	}
	
	public int getLineNumber() {
		return lineno;
	}
	
	public int getColumnNumber() {
		return colno;
	}
	
	public abstract int increaseAllocationByBytes();
	
}

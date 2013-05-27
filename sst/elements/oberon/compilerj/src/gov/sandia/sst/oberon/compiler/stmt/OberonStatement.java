package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.visitor.*;

public abstract class OberonStatement implements OberonVisitorTarget {

	protected String fileName;
	protected int lineno;
	protected int colno;
	
	public OberonStatement(String fileName,	int lineno, int colno) {
		this.fileName = fileName;
		this.lineno = lineno;
		this.colno = colno;
	}
	
	public String getFileName() {
		return fileName;
	}
	
	public int getLineNumber() {
		return lineno;
	}
	
	public int getColumnNumber() {
		return colno;
	}
	
	public abstract int increaseAllocationByBytes();
	
}

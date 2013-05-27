package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitorTarget;

public abstract class OberonExpression implements OberonExpressionVisitorTarget {

	protected String fileName;
	protected int lineNo;
	protected int colNo;
	
	public OberonExpression(String fileName, int lineNo, int colNo) {
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}
	
	public String getFileName() {
		return fileName;
	}
	
	public int getLineNumber() {
		return lineNo;
	}
	
	public int getColumnNumber() {
		return colNo;
	}
	
	public abstract OberonVariableType getExpressionType() throws
		OberonIncompatibleTypeException;
	
}

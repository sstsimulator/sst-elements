package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.exp.OberonVariableType;
import gov.sandia.sst.oberon.compiler.visitor.OberonStatementBodyVisitor;

import java.util.Vector;

public class FunctionDefinition implements StatementBlock {

	protected OberonVariableType funcType;
	protected String functionName;
	protected Vector<OberonStatement> statements;
	protected Vector<TypeNamePair> parameters;
	protected String fileName;
	protected int lineNo;
	protected int colNo;
	
	public FunctionDefinition(String fileName,
			int lineNo,
			int colNo,
			OberonVariableType type, String name) {
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
		
		funcType = type;
		functionName = name;
		statements = new Vector<OberonStatement>();
		parameters = new Vector<TypeNamePair>();
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
	
	public OberonVariableType getFunctionType() {
		return funcType;
	}
	
	public void addParameter(OberonVariableType type, String name) {
		parameters.add(new TypeNamePair(type, name));
	}
	
	public Vector<TypeNamePair> getParameters() {
		return parameters;
	}
	
	public void addStatement(OberonStatement stmt) {
		statements.add(stmt);
	}

	public Vector<OberonStatement> getStatements() {
		return statements;
	}

	public int getDeclaredVariableSize() {
		int allocationSize = 0;
		
		for(OberonStatement nxtStmt : statements) {
			allocationSize += nxtStmt.increaseAllocationByBytes();
		}
		
		return allocationSize;
	}
	
	public int countStatements() {
		return statements.size();
	}
	
	public String getFunctionName() {
		return functionName;
	}

	public void visit(OberonStatementBodyVisitor visit) throws 
	OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

}

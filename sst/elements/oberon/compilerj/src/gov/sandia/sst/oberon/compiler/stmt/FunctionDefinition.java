package gov.sandia.sst.oberon.compiler.stmt;

import java.util.Vector;

public class FunctionDefinition implements StatementBlock {

	protected String functionName;
	protected Vector<OberonStatement> statements;
	
	public FunctionDefinition(String name) {
		functionName = name;
		statements = new Vector<OberonStatement>();
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

}

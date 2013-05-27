package gov.sandia.sst.oberon.compiler.stmt;

import java.util.Vector;

import gov.sandia.sst.oberon.compiler.exp.*;

public abstract class ForStatement extends OberonStatement implements StatementBlock {

	protected OberonExpression loopCondition;
	protected AssignmentStatement incrStmt;
	protected Vector<OberonStatement> statements;

	public ForStatement(String fileName, int lineno, int colno,
			OberonExpression loopCondition,
			AssignmentStatement incrStmt) {
		super(fileName, lineno, colno);
		
		statements = new Vector<OberonStatement>();
		this.loopCondition = loopCondition;
		this.incrStmt = incrStmt;
	}
	
	public OberonExpression getLoopCondition() {
		return loopCondition;
	}
	
	public abstract ForStatementType getForStatementType();
	
	public AssignmentStatement getIncrementStatement() {
		return incrStmt;
	}
	
	public abstract int increaseAllocationByBytes();

	public StatementBlock getStatementBody() {
		return this;
	}
	
	public void addStatement(OberonStatement stmt) {
		statements.add(stmt);
	}

	public Vector<OberonStatement> getStatements() {
		return statements;
	}

	public abstract int getDeclaredVariableSize();

}

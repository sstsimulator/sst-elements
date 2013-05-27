package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonStatementBodyVisitor;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class ForWithAssignmentStatement extends ForStatement {

	protected AssignmentStatement assignStmt;
	
	public ForWithAssignmentStatement(String fileName, int lineno, int colno,
			AssignmentStatement assignStmt,
			OberonExpression loopCondition, AssignmentStatement incrStmt) {
		super(fileName, lineno, colno, loopCondition, incrStmt);
		
		this.assignStmt = assignStmt;
	}

	public AssignmentStatement getAssignmentStatement() {
		return assignStmt;
	}
	
	public ForStatementType getForStatementType() {
		return ForStatementType.ASSIGNMENT;
	}

	public int increaseAllocationByBytes() {
		int incSize = 0;
		
		for(OberonStatement stmt : statements) {
			incSize += stmt.increaseAllocationByBytes();
		}
		
		return incSize;
	}

	public int getDeclaredVariableSize() {
		return increaseAllocationByBytes();
	}

	public void processVisitor(OberonVisitor visit)
			throws OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

	public void visit(OberonStatementBodyVisitor visit)
			throws OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

}

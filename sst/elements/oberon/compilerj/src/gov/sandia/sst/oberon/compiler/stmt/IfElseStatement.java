package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class IfElseStatement extends OberonStatement {

	protected StatementBlock ifStatements;
	protected StatementBlock elseStatements;
	protected OberonExpression condition;
	
	public IfElseStatement(int lineno, int colno,
			OberonExpression condition) {
		super(lineno, colno);
		
		this.condition = condition;
	}

	public int increaseAllocationByBytes() {
		int totalAllocation = 0;
		
		totalAllocation += ifStatements.getDeclaredVariableSize();
		totalAllocation += elseStatements.getDeclaredVariableSize();
		
		return totalAllocation;
	}
	
	public StatementBlock getIfStatements() {
		return ifStatements;
	}
	
	public StatementBlock getElseStatements() {
		return elseStatements;
	}
	
	public OberonExpression getCondition() {
		return condition;
	}

	public void processVisitor(OberonVisitor visit)
			throws OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

}

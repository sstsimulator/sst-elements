package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class ForWithDeclarationStatement extends ForStatement {

	protected DeclarationStatement declStmt;
	
	public ForWithDeclarationStatement(int lineno, int colno,
			DeclarationStatement declStmt,
			OberonExpression loopCondition, AssignmentStatement incrStmt) {
		super(lineno, colno, loopCondition, incrStmt);
		
		this.declStmt = declStmt;
	}

	public DeclarationStatement getDeclarationStatement() {
		return declStmt;
	}
	
	public ForStatementType getForStatementType() {
		return ForStatementType.DECLARATION;
	}

	public int increaseAllocationByBytes() {
		int incSize = 0;
		
		incSize += declStmt.increaseAllocationByBytes();
		
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

}

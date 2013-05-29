package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class ReturnStatement extends OberonStatement {

	protected OberonExpression returnExpr;
	
	public ReturnStatement(String fileName, int lineno, int colno,
			OberonExpression expr) {
		super(fileName, lineno, colno);
		
		returnExpr = expr;
	}

	public void processVisitor(OberonVisitor visit) throws 
	OberonStatementException, OberonExpressionException{
		visit.visit(this);
	}

	public OberonExpression getReturnExpression() {
		return returnExpr;
	}
			
	public int increaseAllocationByBytes() {
		return 0;
	}

}

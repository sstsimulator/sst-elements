package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonBracketedExpression extends OberonUnaryExpression {

	public OberonBracketedExpression(String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit) throws 
	OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		return theExpr.getExpressionType();
	}

}

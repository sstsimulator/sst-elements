package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonBooleanLiteral extends OberonLiteralExpression {

	protected boolean value;
	
	public OberonBooleanLiteral(boolean val,
			String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
	
		value = val;
	}

	public boolean getBooleanValue() {
		return value;
	}
	
	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		return OberonVariableType.BOOLEAN;
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

}

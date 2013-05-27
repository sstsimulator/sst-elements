package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonDoubleLiteral extends OberonLiteralExpression {

	protected double literalValue;
	
	public OberonDoubleLiteral(double value,
			String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
		literalValue = value;
	}

	public double getDoubleValue() {
		return literalValue;
	}
	
	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		
		return OberonVariableType.DOUBLE;
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

}

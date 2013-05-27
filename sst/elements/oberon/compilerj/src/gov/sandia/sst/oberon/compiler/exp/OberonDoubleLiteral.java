package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonDoubleLiteral extends OberonLiteralExpression {

	protected double literalValue;
	
	public OberonDoubleLiteral(
			String fileName, int lineNo, int colNo,
			double value) {
		super(fileName, lineNo, colNo);
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

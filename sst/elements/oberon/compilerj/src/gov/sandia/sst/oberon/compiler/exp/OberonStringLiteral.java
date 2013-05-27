package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonStringLiteral extends OberonLiteralExpression {

	protected String value;
	
	public OberonStringLiteral(
			String fileName, int lineNo, int colNo,
			String val) {
		super(fileName, lineNo, colNo);
		
		value = val;
	}

	public String getStringValue() {
		return value;
	}
	
	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		return OberonVariableType.STRING;
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

}

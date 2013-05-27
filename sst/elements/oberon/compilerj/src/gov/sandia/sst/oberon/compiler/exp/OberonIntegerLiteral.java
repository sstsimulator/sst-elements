package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonIntegerLiteral extends OberonLiteralExpression {

	protected long value;
	
	public OberonIntegerLiteral(long value,
			String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
		
		this.value = value;
	}
	
	public long getIntegerValue() {
		return value;
	}

	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		return OberonVariableType.INTEGER;
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

}

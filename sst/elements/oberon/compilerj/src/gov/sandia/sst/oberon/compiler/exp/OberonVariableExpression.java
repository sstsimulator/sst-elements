package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonVariableExpression extends OberonUnaryExpression {

	protected OberonVariableType varType;
	protected String varName;
	
	public OberonVariableExpression(OberonVariableType vType,
			String varName,
			String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
		
		this.varType = vType;
		this.varName = varName;
	}

	public String getVariableName() {
		return varName;
	}
	
	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		return varType;
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

}

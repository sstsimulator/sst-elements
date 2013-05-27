package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonVariableExpression extends OberonExpression {

	protected OberonVariableType varType;
	protected String varName;
	
	public OberonVariableExpression(String fileName, int lineNo, int colNo,
			String varName,
			OberonVariableType vType) {
		super(fileName, lineNo, colNo);
		
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

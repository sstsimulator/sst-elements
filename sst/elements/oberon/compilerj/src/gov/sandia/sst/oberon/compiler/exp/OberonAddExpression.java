package gov.sandia.sst.oberon.compiler.exp;

import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonAddExpression extends
		OberonBinaryMathematicalExpression {

	public OberonAddExpression(
			String fileName,
			int lineNo, 
			int colNo,
			OberonExpression onLeft,
			OberonExpression onRight) {
		super(fileName, lineNo, colNo, onLeft, onRight);
	}

	public OberonVariableType getExpressionType() throws OberonIncompatibleTypeException {
		if(left.getExpressionType() == OberonVariableType.BOOLEAN) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform an addition on a boolean value");
		} else if(left.getExpressionType() == OberonVariableType.STRING) {
			return OberonVariableType.STRING;
		} else if (right.getExpressionType() == OberonVariableType.BOOLEAN) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform an addition on a boolean value");
		} else if (right.getExpressionType() == OberonVariableType.STRING) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform an addition on a string");
		} else if(left.getExpressionType() == OberonVariableType.DOUBLE) {
			return OberonVariableType.DOUBLE;
		} else if(right.getExpressionType() == OberonVariableType.DOUBLE) {
			return OberonVariableType.DOUBLE;
		} else {
			return OberonVariableType.INTEGER;
		}
	}

	public void processVisitorTarget(OberonExpressionVisitor expVisit) throws 
	OberonStatementException, OberonExpressionException{
		expVisit.visit(this);
	}

}
